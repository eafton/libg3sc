#include <stdio.h>
#include <fcntl.h>
#include <glib-object.h>
#include <g3sc.h>

typedef struct {
	G3SCStyle *style;
	GScanner *scanner;
	gchar *file;
	gchar *dirc;
	gchar *input;
	gsize sz;
} G3SCStyleSheet;

struct _G3SCStylePrivate {
    GScannerConfig config;
	GRegex *at_regex;
	GRegex *prop_regex;
	GRegex *import_regex;
	
	GArray *sheets;
};

G_DEFINE_TYPE(G3SCStyle, g3sc_style, G_TYPE_OBJECT)

static void new_sheet(G3SCStyle *style, const gchar *file);

static void _finalize(GObject *object) {
	G3SCStyle *style;
	
	style = G3SC_STYLE(object);
	g_regex_unref(style->priv->at_regex);
	g_regex_unref(style->priv->prop_regex);
	G_OBJECT_CLASS(g3sc_style_parent_class)->finalize(object);
}

static void g3sc_style_class_init(G3SCStyleClass *klass) {
	GObjectClass *g_class;

	g_class = G_OBJECT_CLASS(klass);
	g_class->finalize = _finalize;
	g_type_class_add_private(klass, sizeof(G3SCStylePrivate));
}	

static void g3sc_style_init(G3SCStyle *style) {
	style->priv = G_TYPE_INSTANCE_GET_PRIVATE(style, G3SC_TYPE_STYLE, G3SCStylePrivate);
    
    style->priv->config.cset_skip_characters = " \t\r\n";
    style->priv->config.cset_identifier_first = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_.-#*@";
    style->priv->config.cset_identifier_nth = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_.-0123456789:><";
    style->priv->config.cpair_comment_single = NULL;
    style->priv->config.case_sensitive = TRUE;
    style->priv->config.skip_comment_multi = TRUE;
    style->priv->config.skip_comment_single = TRUE;
    style->priv->config.scan_comment_multi = TRUE;
    style->priv->config.scan_identifier = TRUE;
    style->priv->config.scan_identifier_1char = TRUE;
    style->priv->config.scan_identifier_NULL = FALSE;
    style->priv->config.scan_symbols = FALSE;
    style->priv->config.scan_binary = FALSE;
    style->priv->config.scan_octal = FALSE;
    style->priv->config.scan_float = FALSE;
    style->priv->config.scan_hex = FALSE;
    style->priv->config.scan_hex_dollar = FALSE;
    style->priv->config.scan_string_sq = TRUE;
    style->priv->config.scan_string_dq = TRUE;
    style->priv->config.numbers_2_int = FALSE;
    style->priv->config.int_2_float = FALSE;
    style->priv->config.identifier_2_string = FALSE;
    style->priv->config.char_2_token = TRUE;
    style->priv->config.symbol_2_token = FALSE;
    style->priv->config.scope_0_fallback = FALSE;
    style->priv->config.store_int64 = FALSE;
	
    style->priv->prop_regex = g_regex_new("([a-zA-Z0-9_-]+)\\s*:\\s*([^;]+);", 0, 0, NULL);
    style->priv->at_regex = g_regex_new("(\\d+)\\s+(\\.\\d+)", 0, 0, NULL);
    style->priv->import_regex = g_regex_new("@import\\s+(?:url\\s*\\(\\s*[\"']?|['\"])([^'\")\\s]+)", G_REGEX_CASELESS, 0, NULL);

    
    style->priv->sheets = g_array_new(FALSE, FALSE, sizeof(G3SCStyleSheet));
}

static void demo_parse(G3SCStyleSheet *sheet) {
	const gchar *cptr;

	cptr = sheet->input;
    while (g_scanner_peek_next_token(sheet->scanner) != G_TOKEN_EOF) {
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

        while (g_scanner_peek_next_token(sheet->scanner) != '{' &&  g_scanner_peek_next_token(sheet->scanner) != ';' && g_scanner_peek_next_token(sheet->scanner) != G_TOKEN_EOF) {
            GTokenType tok;
            
            tok = g_scanner_get_next_token(sheet->scanner);
            if (tok == G_TOKEN_IDENTIFIER) {
                if (!first_token && compound_selector->str[compound_selector->len - 1] != ' ') {
                    g_string_append_c(compound_selector, ' ');
                }
                
                if (sheet->scanner->value.v_identifier[0] == '@') {
					is_at = TRUE;
				}
                
				g_string_append(compound_selector, sheet->scanner->value.v_identifier);
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
                g_string_append_printf(compound_selector, "%d", sheet->scanner->value.v_int);
            } else if (tok == G_TOKEN_STRING && is_at) {
                g_string_append_printf(compound_selector, "\"%s\"", sheet->scanner->value.v_string);
			} else if (tok == G_TOKEN_CHAR) {
                g_string_append_c(compound_selector, (gchar)sheet->scanner->value.v_char);
                first_token = FALSE;
            }
        }
        
        if (is_at) {
			GMatchInfo *match_info;
			gchar *result;
			gboolean matched;
			
			result = g_regex_replace(sheet->style->priv->at_regex, compound_selector->str, -1, 0, "\\1\\2", 0, NULL);

			match_info = NULL;
			matched = g_regex_match(sheet->style->priv->import_regex, result, 0, &match_info);
			if (matched) {
				gchar *url;
				gchar *path;
				
				url = g_match_info_fetch(match_info, 1);
				path = g_strconcat(sheet->dirc, G_DIR_SEPARATOR_S, url, NULL);
				new_sheet(sheet->style, path);
				g_free(url);
				g_free(path);
			}
			g_match_info_free(match_info);

			g_free(result);
		}
        
        term_tok = g_scanner_get_next_token(sheet->scanner);
        if (term_tok == ';') {
            g_string_free(compound_selector, TRUE);
            
            block_end = strchr(cptr, ';');
            if (!block_end){ 
				break;
			}
            cptr = block_end + 1;
            g_scanner_sync_file_offset(sheet->scanner);
            g_scanner_input_text(sheet->scanner, cptr, sheet->sz - (cptr - sheet->input));
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
         
				/*printf("\nS: %s\n", selector);*/
				g_regex_match(sheet->style->priv->prop_regex, contents, 0, &match_info);
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
            g_scanner_sync_file_offset(sheet->scanner);
            g_scanner_input_text(sheet->scanner, cptr, sheet->sz - (cptr - sheet->input));
        } else {
            g_free(selector);
            break;
        }

        g_free(selector);
    }
}

static void new_sheet(G3SCStyle *style, const gchar *file) {
	G3SCStyleSheet *sheet;
	
	sheet = g_new(G3SCStyleSheet, 1);
	style->priv->sheets = g_array_append_vals(style->priv->sheets, &sheet, 1);
	
	sheet->input = NULL;
	sheet->style = style;
	sheet->file = g_strdup(file);
	sheet->dirc = g_path_get_dirname(file);
	g_file_get_contents(file, &sheet->input, &sheet->sz, NULL);
	sheet->scanner = g_scanner_new(&style->priv->config);
    g_scanner_input_text(sheet->scanner, sheet->input, sheet->sz);
	demo_parse(sheet);
}


G3SCStyle *g3sc_style_new_from_file(const gchar *file) {
	G3SCStyle *style;
	
	style = G3SC_STYLE(g_object_new(G3SC_TYPE_STYLE, NULL));
    new_sheet(style, file);
	return style;
}

