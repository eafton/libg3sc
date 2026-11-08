#include <stdio.h>
#include <glib-object.h>
#include <g3sc.h>

struct _G3SCStylePrivate {
	GScanner *scanner;
	GRegex *at_regex;
	GRegex *prop_regex;

	gchar *input;
	gsize input_sz;
};

G_DEFINE_TYPE(G3SCStyle, g3sc_style, G_TYPE_OBJECT)

static void _finalize(GObject *object) {
	G3SCStyle *style;
	
	style = G3SC_STYLE(object);
	g_free(style->priv->input);
	g_regex_unref(style->priv->at_regex);
	g_regex_unref(style->priv->prop_regex);
	g_scanner_destroy(style->priv->scanner);
	G_OBJECT_CLASS(g3sc_style_parent_class)->finalize(object);
}

static void g3sc_style_class_init(G3SCStyleClass *klass) {
	GObjectClass *g_class;

	g_class = G_OBJECT_CLASS(klass);
	g_class->finalize = _finalize;
	g_type_class_add_private(klass, sizeof(G3SCStylePrivate));
}	

static void g3sc_style_init(G3SCStyle *style) {
    GScannerConfig config;
    
	style->priv = G_TYPE_INSTANCE_GET_PRIVATE(style, G3SC_TYPE_STYLE, G3SCStylePrivate);
	style->priv->input = NULL;
    style->priv->input_sz = 0;
    
    config.cset_skip_characters = " \t\r\n";
    config.cset_identifier_first = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_.-#*@";
    config.cset_identifier_nth = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_.-0123456789:><";
    config.cpair_comment_single = NULL;
    config.case_sensitive = TRUE;
    config.skip_comment_multi = TRUE;
    config.skip_comment_single = TRUE;
    config.scan_comment_multi = TRUE;
    config.scan_identifier = TRUE;
    config.scan_identifier_1char = TRUE;
    config.scan_identifier_NULL = FALSE;
    config.scan_symbols = FALSE;
    config.scan_binary = FALSE;
    config.scan_octal = FALSE;
    config.scan_float = FALSE;
    config.scan_hex = FALSE;
    config.scan_hex_dollar = FALSE;
    config.scan_string_sq = TRUE;
    config.scan_string_dq = TRUE;
    config.numbers_2_int = FALSE;
    config.int_2_float = FALSE;
    config.identifier_2_string = FALSE;
    config.char_2_token = TRUE;
    config.symbol_2_token = FALSE;
    config.scope_0_fallback = FALSE;
    config.store_int64 = FALSE;
    style->priv->scanner = g_scanner_new(&config);
	
    style->priv->prop_regex = g_regex_new("([a-zA-Z0-9_-]+)\\s*:\\s*([^;]+);", 0, 0, NULL);
    style->priv->at_regex = g_regex_new("(\\d+)\\s+(\\.\\d+)", 0, 0, NULL);
}

void demo_parse(G3SCStyle *style) {
	const gchar *cptr;

	cptr = style->priv->input;
    while (g_scanner_peek_next_token(style->priv->scanner) != G_TOKEN_EOF) {
        GString *compound_selector;
        const gchar *block_start;
        const gchar *block_end;
        gchar *selector;
        GTokenType term_tok;
        gint depth;
        gboolean first_token;
        gboolean is_at;
        
        selector = NULL;
        compound_selector = g_string_new("");
        block_start = NULL;
        block_end = NULL;
        depth = 0;
        first_token = TRUE;
        is_at = FALSE;

        while (g_scanner_peek_next_token(style->priv->scanner) != '{' &&  g_scanner_peek_next_token(style->priv->scanner) != ';' && g_scanner_peek_next_token(style->priv->scanner) != G_TOKEN_EOF) {
            GTokenType tok;
            
            tok = g_scanner_get_next_token(style->priv->scanner);
            if (tok == G_TOKEN_IDENTIFIER) {
                if (!first_token && compound_selector->str[compound_selector->len - 1] != ' ') {
                    g_string_append_c(compound_selector, ' ');
                }
                
                if (style->priv->scanner->value.v_identifier[0] == '@') {
					is_at = TRUE;
				}
                
				g_string_append(compound_selector, style->priv->scanner->value.v_identifier);
                first_token = FALSE;
            } else if (tok == G_TOKEN_COMMA) {
                g_string_append(compound_selector, ", ");
            } else if (tok == '>') {
                g_string_append(compound_selector, " > ");
            } else if (tok == '<') {
                g_string_append(compound_selector, " < ");
            } else if (tok == G_TOKEN_LEFT_PAREN && is_at) {
                g_string_append(compound_selector, "(");
            } else if (tok == G_TOKEN_RIGHT_PAREN && is_at) {
                g_string_append(compound_selector, ")");
            } else if (tok == G_TOKEN_INT && is_at) {
                g_string_append_printf(compound_selector, "%d", style->priv->scanner->value.v_int);
            } else if (tok == G_TOKEN_STRING && is_at) {
                g_string_append_printf(compound_selector, "\"%s\"", style->priv->scanner->value.v_string);
			} else if (tok == G_TOKEN_CHAR) {
                g_string_append_c(compound_selector, (gchar)style->priv->scanner->value.v_char);
                first_token = FALSE;
            }
        }
        
        if (is_at) {
			gchar *result;
			
			result = g_regex_replace(style->priv->at_regex, compound_selector->str, -1, 0, "\\1\\2", 0, NULL);
			printf("A: %s\n", result);
			g_free(result);
		}
        
        term_tok = g_scanner_get_next_token(style->priv->scanner);
        if (term_tok == ';') {
            g_string_free(compound_selector, TRUE);
            
            block_end = strchr(cptr, ';');
            if (!block_end){ 
				break;
			}
            cptr = block_end + 1;
            g_scanner_sync_file_offset(style->priv->scanner);
            g_scanner_input_text(style->priv->scanner, cptr, style->priv->input_sz - (cptr - style->priv->input));
            continue;
        }

        if (term_tok != '{') {
            g_string_free(compound_selector, TRUE);
            break;
        }

        selector = g_string_free(compound_selector, FALSE);
        g_strstrip(selector);

        block_start = strchr(cptr, '{');
        if (!block_start) {
            g_free(selector);
            break;
        }
        
        block_start++; 

        block_end = block_start;
        depth = 1;
        while (*block_end != '\0' && depth > 0) {
            if (*block_end == '{') {
                depth++;
            } else if (*block_end == '}') {
                depth--;
                if (depth == 0) {
                    break;
                }
            }
            block_end++;
        }

        if (!depth && block_end >= block_start) {
            if (!is_at) {
				GMatchInfo *match_info;
				gchar *contents;
                
                contents = g_strndup(block_start, block_end - block_start);
                g_strstrip(contents);
         
				printf("\nS: %s\n", selector);
				g_regex_match(style->priv->prop_regex, contents, 0, &match_info);
				while (g_match_info_matches(match_info)) {
					gchar *key;
					gchar *val;
					
					key = g_match_info_fetch(match_info, 1);
					val = g_match_info_fetch(match_info, 2);

					if (key && val) {
						g_strstrip(key);
						g_strstrip(val);
						printf("\nK: %s\nV: %s\n", key, val);
					}

					g_free(key);
					g_free(val);
					g_match_info_next(match_info, NULL);
				}

				g_match_info_free(match_info);
                g_free(contents);
            } 
            
            cptr = block_end + 1;
            g_scanner_sync_file_offset(style->priv->scanner);
            g_scanner_input_text(style->priv->scanner, cptr, style->priv->input_sz - (cptr - style->priv->input));
        } else {
            g_free(selector);
            break;
        }

        g_free(selector);
    }
}

G3SCStyle *g3sc_style_new_from_file(const gchar *file) {
	G3SCStyle *style;
	
	style = G3SC_STYLE(g_object_new(G3SC_TYPE_STYLE, NULL));
	g_file_get_contents(file, &style->priv->input, &style->priv->input_sz, NULL);
    g_scanner_input_text(style->priv->scanner, style->priv->input, style->priv->input_sz);
    demo_parse(style);
	return style;
}

