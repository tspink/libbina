#include <bina.h>
#include <malloc.h>
#include <string.h>
#include "dex.h"

static inline void *off_to_real(struct dex *dex, file_offset_t off)
{
	return (void *)(dex->base + off);
}

static unsigned int read_uleb128(char **p)
{
	unsigned int result = 0;
	int shift = 0;
	
	do {
		result |= (**p & 0x7F) << shift;
		shift += 7;
	} while (*((*p)++) & 0x80);
	
	return result;
}

char *dex_get_string(struct dex *dex, table_index_t idx)
{
	if (dex->strings[idx] == NULL) {
		struct dex_string_id_item *items = off_to_real(dex, dex->header->string_ids_off);
		file_offset_t off = items[idx].string_data_off;
		void *str_data = off_to_real(dex, off);
		unsigned int str_length = read_uleb128((char **)&str_data);

		dex->strings[idx] = calloc(1, str_length+1);
		memcpy(dex->strings[idx], str_data, str_length);
	}
	
	return dex->strings[idx];
}

static void read_encoded_fields(char **off, unsigned int count)
{
	int i;
	
	for (i = 0; i < count; i++) {
		unsigned int field_idx_diff, access_flags;
		
		field_idx_diff = read_uleb128(off);
		access_flags = read_uleb128(off);
		
		//printf("  field: 0x%x 0x%x\n", field_idx_diff, access_flags);
	}
}

static void disassemble_method(struct dex *dex, struct dex_method_id_item *item, file_offset_t code_off)
{
	struct dex_code_item *code_item = off_to_real(dex, code_off);
	int i;
	
	printf("method %s:\n", dex_get_string(dex, item->name_idx));
	
	for (i = 0; i < code_item->insns_size; i++) {
		printf("  0x%x\n", code_item->insns[i]);
	}
	exit(0);
}

static void read_encoded_methods(struct dex *dex, char **off, unsigned int count)
{
	struct dex_method_id_item *method_items = off_to_real(dex, dex->header->method_ids_off);
	unsigned int method_idx_diff = 0;
	int i;
	
	for (i = 0; i < count; i++) {
		unsigned int access_flags, code_off;
		
		method_idx_diff += read_uleb128(off);
		access_flags = read_uleb128(off);
		code_off = read_uleb128(off);

		printf("  method: 0x%x 0x%x 0x%x\n", method_idx_diff, access_flags, code_off);
		
		disassemble_method(dex, &method_items[method_idx_diff], code_off);
	}
}

static void load_class_methods(struct dex *dex, struct dex_class_def_item *def, struct dex_class *cls)
{
	char *class_data = off_to_real(dex, def->class_data_off);
	unsigned int static_fields_size, instance_fields_size, direct_methods_size, virtual_methods_size;
	
	if (class_data == NULL)
		return;
	
	static_fields_size = read_uleb128(&class_data);
	instance_fields_size = read_uleb128(&class_data);
	direct_methods_size = read_uleb128(&class_data);
	virtual_methods_size = read_uleb128(&class_data);
	
	//printf("static fields:\n");
	read_encoded_fields(&class_data, static_fields_size);

	//printf("instance fields:\n");
	read_encoded_fields(&class_data, instance_fields_size);
	
	//printf("direct methods:\n");
	read_encoded_methods(dex, &class_data, direct_methods_size);

	//printf("virtual methods:\n");
	read_encoded_methods(dex, &class_data, virtual_methods_size);
}

static int init(struct dex *dex)
{
	struct dex_type_id_item *type_id_items = off_to_real(dex, dex->header->type_ides_off);
	struct dex_class_def_item *class_def_items = off_to_real(dex, dex->header->class_defs_off);
	int i;
	
	dex->strings = calloc(dex->header->string_ids_size, sizeof(*dex->strings));
	dex->types = calloc(dex->header->type_ids_size, sizeof(*dex->types));
	dex->classes = calloc(dex->header->class_defs_size, sizeof(*dex->classes));
	
	for (i = 0; i < dex->header->type_ids_size; i++) {
		dex->types[i].descriptor = dex_get_string(dex, type_id_items[i].descriptor_idx);
	}
	
	for (i = 0; i < dex->header->class_defs_size; i++) {
		dex->classes[i].type = &dex->types[class_def_items[i].class_idx];
		dex->classes[i].superclass = &dex->types[class_def_items[i].superclass_idx];
		load_class_methods(dex, &class_def_items[i], &dex->classes[i]);
		
		//printf("class: %s : %s\n", dex->classes[i].type->descriptor, dex->classes[i].superclass->descriptor);
	}
	
	return 0;
}

struct dex *dex_load(struct bina_context *ctx)
{
	struct dex *dex;
	struct dex_header *header = (struct dex_header *)ctx->base;
	
	dex = calloc(1, sizeof(*dex));
	if (!dex)
		return NULL;
	
	dex->base = ctx->base;
	dex->header = header;
	
	if (strncmp(header->magic, "dex\n035\0", 8)) {
		printf("error: invalid dex magic: %s\n", dex->header->magic);
		free(dex);
		return NULL;
	}
	
	if (header->endian_tag != ENDIAN_CONSTANT) {
		printf("error: unsupported file endian\n");
		free(dex);
		return NULL;
	}
	
	if (header->header_size != sizeof(*header)) {
		printf("error: header size mismatch\n");
		free(dex);
		return NULL;
	}
	
	init(dex);
	return dex;
}

void dex_unload(struct dex *dex)
{
	free(dex);
}
