#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "buffmtparser.h"
#include "lib/automem.h"

// define in midserver.c
// void print_data(void* data, int len, const char* name);

struct BuffedFmtParser_s {
	FMTParserState* state;
	automem_t* mem;
	unsigned int dataIndex;
};
typedef struct BuffedFmtParser_s BuffedFmtParser;

void* buffered_fmt_parser_new(int parse_server_data) {
	BuffedFmtParser* parser = (BuffedFmtParser*) malloc(sizeof(*parser));
	assert(parser);
	parser->state = fmt_parser_new_state(parse_server_data ? 1 : 0);
	parser->mem = (automem_t*) malloc(sizeof(automem_t));
	automem_init(parser->mem, 1024);
	parser->dataIndex = 0;
	return parser;
}

void buffered_fmt_parser_delete(void* parser) {
	BuffedFmtParser* p = (BuffedFmtParser*) parser;
	fmt_parser_close_state(p->state);
	automem_uninit(p->mem);
	p->state = NULL;
	p->mem = NULL;
	p->dataIndex = 0;
	free(parser);
}

#define MAX_PREFIX_MEM 1024

void buffered_fmt_parser_push(void* parser, const void* data, unsigned int len,
							fmt_available_callback fmt_available, void* userdata)
{
	int used;
	BuffedFmtParser* p = (BuffedFmtParser*) parser;
	assert(p && p->state && p->mem);

	if(p->dataIndex == p->mem->size) {
		automem_reset(p->mem);
		p->dataIndex = 0;
	}
	automem_append_voidp(p->mem, (unsigned char*)data, len);

	//print_data(p->mem->pdata + p->dataIndex, p->mem->size - p->dataIndex, "before parse");
	
	used = fmt_parser_push(p->state, p->mem->pdata + p->dataIndex, p->mem->size - p->dataIndex);
	assert(used >= 0);
	p->dataIndex += (unsigned int)used;

	while(used > 0) {
		//printf("parser used: %d\n", used);
		//callback
		if(fmt_available && fmt_parser_complete(p->state)) {
            FMT* fmt = fmt_parser_take(p->state);
			fmt_available(parser, p->state->m_cmd, fmt, userdata);
            if(fmt)
                fmt_object_put(fmt);
        }
		
		used = fmt_parser_push(p->state, p->mem->pdata + p->dataIndex, p->mem->size - p->dataIndex);
		assert(used >= 0);
		p->dataIndex += (unsigned int)used;
	}

	if(p->dataIndex >= MAX_PREFIX_MEM) {
		//automem_erase(p->mem, p->dataIndex);
		automem_erase_ex(p->mem, p->dataIndex, 64*1024);
		p->dataIndex = 0;
	}

	//print_data(p->mem->pdata + p->dataIndex, p->mem->size - p->dataIndex, "after parse");
}

int buffered_fmt_parser_complete(void* parser) {
	return fmt_parser_complete(((BuffedFmtParser*)parser)->state);
}

void buffered_fmt_parser_reset(void* parser) {
	fmt_parser_reset(((BuffedFmtParser*)parser)->state);
}

#ifdef FMT_HAS_V2
void buffered_fmt_parser_set_key(void* parser, const char* key) {
	fmt_parser_set_key(((BuffedFmtParser*)parser)->state, (char*)key);
}
#endif
