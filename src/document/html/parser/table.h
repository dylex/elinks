
#ifndef EL__DOCUMENT_HTML_PARSER_TABLE_H
#define EL__DOCUMENT_HTML_PARSER_TABLE_H

#include "util/color.h"

#ifdef __cplusplus
extern "C" {
#endif

struct html_context;
struct part;

/* Fix namespace clash on MacOS. */
#define table table_elinks

#define ALIGN_TR	-1

#define VALIGN_TR	-1
#define VALIGN_TOP	0
#define VALIGN_MIDDLE	1
#define VALIGN_BOTTOM	2
#define VALIGN_BASELINE	VALIGN_TOP /* Not implemented. */

#define WIDTH_AUTO		-1
#define WIDTH_RELATIVE		-2

#define TABLE_FRAME_VOID	0
#define TABLE_FRAME_ABOVE	1
#define TABLE_FRAME_BELOW	2
#define TABLE_FRAME_HSIDES	(TABLE_FRAME_ABOVE | TABLE_FRAME_BELOW)
#define TABLE_FRAME_LHS		4
#define TABLE_FRAME_RHS		8
#define TABLE_FRAME_VSIDES	(TABLE_FRAME_LHS | TABLE_FRAME_RHS)
#define TABLE_FRAME_BOX		(TABLE_FRAME_HSIDES | TABLE_FRAME_VSIDES)

#define TABLE_RULE_NONE		0
#define TABLE_RULE_ROWS		1
#define TABLE_RULE_COLS		2
#define TABLE_RULE_ALL		3
#define TABLE_RULE_GROUPS	4

struct html_start_end {
	char *start, *end;
	void *start_node, *end_node;
};

struct table_cell {
	char *start;
	char *end;
	void *start_node;
	void *end_node;
	char *fragment_id;
	color_T bgcolor;
	int col, row;
	int align;
	int valign;
	int colspan;
	int rowspan;
	int min_width;
	int max_width;
	int width, height;
	int link_num;

	unsigned int is_used:1;
	unsigned int is_spanned:1;
	unsigned int is_header:1;
	unsigned int is_group:1;
};

struct table_column {
	int group;
	int align;
	int valign;
	int width;
};

struct table_colors {
	color_T background;
	color_T border;
};

struct table {
	struct part *part;
	struct table_cell *cells;
	char *fragment_id;
	struct table_colors color;
	int align;

	struct table_column *columns;
	int columns_count; /* Number of columns used. */
	int real_columns_count;	/* Number of columns really allocated. */

	int *min_cols_widths;
        int *max_cols_widths;
	int *cols_widths;
	int *cols_x;
	int cols_x_count;

	int *rows_heights;

	int cols, rows;	/* For number of cells used. */
	int real_cols, real_rows; /* For number of cells really allocated. */
	int border;
	int cellpadding;
	int vcellpadding;
	int cellspacing;
	int frame, rules, width;
	int real_width;
	int real_height;
	int min_width;
	int max_width;

	int link_num;

	unsigned int full_width:1;

	struct html_start_end caption;
	int caption_height;
	struct html_start_end *bad_html;
	int bad_html_size;
};

#define CELL(table, col, row) (&(table)->cells[(row) * (table)->real_cols + (col)])

struct table *
parse_table(char *html, char *eof, char **end,
	    char *attr, int sh, struct html_context *html_context);

struct table *new_table(void);
void free_table(struct table *table);
void new_columns(struct table *table, int span, int width, int align, int valign, int group);
struct table_cell *new_cell(struct table *table, int dest_col, int dest_row);
void set_td_width(struct table *table, int col, int width, int force);

#ifdef __cplusplus
}
#endif

#endif
