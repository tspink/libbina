#ifndef __DEX_H__
#define __DEX_H__

typedef unsigned int file_offset_t;
typedef unsigned int table_offset_t;
typedef unsigned int table_index_t;

struct dex_header {
	char magic[8];
	unsigned int checksum;
	char sha1[20];

	unsigned int file_length;
	unsigned int header_length;

	char padding[8];

	unsigned int nr_strings;
	table_offset_t stab_offset;
	unsigned int unknown;

	unsigned int nr_classes;
	table_offset_t ctab_offset;

	unsigned int nr_fields;
	table_offset_t ftab_offset;

	unsigned int nr_methods;
	table_offset_t mtab_offset;

	unsigned int nr_class_defs;
	table_offset_t dtab_offset;
};

struct dex_stab_entry {
	file_offset_t data_offset;
	unsigned int str_length;
};

struct dex_ctab_entry {
	table_index_t name_stab_index;
};

struct dex_ftab_entry {
	table_index_t class_ctab_index;
	table_index_t name_stab_index;
	table_index_t mtd_stab_index;
};

struct dex_mtab_entry {
	table_index_t class_ctab_index;
	table_index_t name_stab_index;
	table_index_t mtd_stab_index;
};

enum dex_access_flags {
	UNKNOWN
};

struct dex_dtab_entry {
	table_index_t class_ctab_index;
	enum dex_access_flags access_flags;
	table_index_t superclass_ctab_index;
	file_offset_t iface_list_offset;
	file_offset_t sfld_list_offset;
	file_offset_t ifld_list_offset;
	file_offset_t dmth_list_offset;
	file_offset_t vmth_list_offset;
};

struct dex_field_list_entry {
	unsigned long long idx;
};

struct dex_method_list_entry {
	table_index_t method_mtab_index;
	enum dex_access_flags access_flags;
	unsigned int unknown;
	file_offset_t code_header_offset;
};

struct dex_code_header {
	unsigned short nr_registers;
	unsigned short nr_in_params;
	unsigned short nr_out_params;
	unsigned short padding;
	table_index_t source_file_stab_index;
	file_offset_t code_offset;
	file_offset_t throwable_list_offset;
	file_offset_t debug_list_offset;
	file_offset_t loc_var_list_offset;
};

struct loc_var_list_entry {
	unsigned int var_start, var_end;
	table_index_t name_stab_index;
	table_index_t vtd_stab_index;
	unsigned int reg_index;
};

struct dex {
	struct dex_header *header;
	
	struct dex_stab_entry *string_table;
	unsigned int nr_strings;
};

#endif
