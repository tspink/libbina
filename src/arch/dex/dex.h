#ifndef __DEX_H__
#define __DEX_H__

typedef unsigned int file_offset_t;
typedef unsigned int file_size_t;
typedef unsigned int table_index_t;
typedef unsigned short table_short_index_t;
typedef unsigned int uleb128;
typedef signed int sleb128;

#define DEX_FILE_MAGIC			"dex\n035\0"
#define ENDIAN_CONSTANT			0x12345678
#define REVERSE_ENDIAN_CONSTANT	0x78563412
#define NO_INDEX				0xFFFFFFFF

struct dex_header {
	char magic[8];
	unsigned int checksum;
	char sha1[20];

	file_size_t file_size;
	file_size_t header_size;

	unsigned int endian_tag;
	
	file_size_t link_size;
	file_offset_t link_off;
	
	file_offset_t map_offset;

	file_size_t string_ids_size;
	file_offset_t string_ids_off;

	file_size_t type_ids_size;
	file_offset_t type_ides_off;

	file_size_t proto_ids_size;
	file_offset_t proto_ids_off;

	file_size_t field_ids_size;
	file_offset_t field_ids_off;

	file_size_t method_ids_size;
	file_offset_t method_ids_off;
	
	file_size_t class_defs_size;
	file_offset_t class_defs_off;
	
	file_size_t data_size;
	file_offset_t data_off;
};

enum dex_access_flags {
	ACC_PUBLIC			= 0x1,
	ACC_PRIVATE			= 0x2,
	ACC_PROTECTED		= 0x4,
	ACC_STATIC			= 0x8,
	ACC_FINAL			= 0x10,
	ACC_SYNCHRONISED	= 0x20,
	ACC_VOLATILE		= 0x40,
	ACC_BRIDGE			= 0x40,
	ACC_TRANSIENT		= 0x80,
	ACC_VARARGS			= 0x80,
	ACC_NATIVE			= 0x100,
	ACC_INTERFACE		= 0x200,
	ACC_ABSTRACT		= 0x400,
	ACC_STRICT			= 0x800,
	ACC_SYNTHETIC		= 0x1000,
	ACC_ANNOTATION		= 0x2000,
	ACC_ENUM			= 0x4000,
	ACC_UNUSED			= 0x8000,
	ACC_CONSTRUCTOR		= 0x10000,
	ACC_DECL_SYNCH		= 0x20000,
};

struct dex_string_id_item {
	file_offset_t string_data_off;
};

struct dex_string_data_item {
	uleb128 utf16_size;
	char data[];
};

struct dex_type_id_item {
	table_index_t descriptor_idx;
};

struct dex_proto_id_item {
	table_index_t shorty_idx;
	table_index_t return_type_idx;
	file_offset_t parameters_off;
};

struct dex_field_id_item {
	table_short_index_t class_idx;
	table_short_index_t type_idx;
	table_index_t name_idx;
};

struct dex_method_id_item {
	table_short_index_t class_idx;
	table_short_index_t proto_idx;
	table_index_t name_idx;
};

struct dex_class_def_item {
	table_index_t class_idx;
	unsigned int access_flags;
	table_index_t superclass_idx;
	file_offset_t interfaces_off;
	table_index_t source_file_idx;
	file_offset_t annotations_off;
	file_offset_t class_data_off;
	file_offset_t static_values_off;
};

struct dex_class_data_item {
	uleb128 static_fields_size;
	uleb128 instance_fields_size;
	uleb128 direct_methods_size;
	uleb128 virtual_methods_size;
};

struct dex_encoded_field {
	uleb128 field_idx_diff;
	uleb128 access_flags;
};

struct dex_encoded_method {
	uleb128 method_idx_diff;
	uleb128 access_flags;
	uleb128 code_off;
};

struct dex_type_item {
	table_short_index_t type_idx;
};

struct dex_type_list {
	unsigned int size;
	struct dex_type_item list[];
};

struct dex_code_item {
	unsigned short registers_size;
	unsigned short ins_size;
	unsigned short outs_size;
	unsigned short tries_size;
	file_offset_t debug_info_off;
	unsigned int insns_size;
	unsigned short insns[];
};

/*
 * In-memory DEX file representation.
 */
enum dex_type_code {
	TC_VOID,
	TC_BOOLEAN,
	TC_BYTE,
	TC_SHORT,
	TC_CHAR,
	TC_INT,
	TC_LONG,
	TC_FLOAT,
	TC_DOUBLE,
	TC_CLASS,
	TC_ARRAY,
};

struct dex_type {
	char *descriptor;
	
	enum dex_type_code type_code;
	char *fqn;
};

struct dex_method {
	int virtual;
};

struct dex_class {
	struct dex_type *type;
	struct dex_type *superclass;
	struct dex_method *methods;
};

struct dex {
	struct dex_header *header;
	char *base;
	
	char **strings;
	unsigned int nr_strings;
	
	struct dex_type *types;
	unsigned int nr_types;

	struct dex_class *classes;
	unsigned int nr_classes;
};

extern struct dex *dex_load(struct bina_context *ctx);
extern void dex_unload(struct dex *dex);

#endif
