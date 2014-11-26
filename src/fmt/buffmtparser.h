#ifndef __BUF_FMT_PARSER_H__
#define __BUF_FMT_PARSER_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "protocol.h"

typedef void (*fmt_available_callback)(void* parser, unsigned short cmd, FMT* fmt, void* userdata);

void* buffered_fmt_parser_new();
void buffered_fmt_parser_delete(void* parser);
void buffered_fmt_parser_push(void* parser,
                              const void* data, unsigned int len,
                              fmt_available_callback cb, void* userdata);
int buffered_fmt_parser_complete(void* parser);
void buffered_fmt_parser_reset(void* parser);

#ifdef FMT_HAS_V2
    void buffered_fmt_parser_set_key(void* parser, const char* key);
#endif

#ifdef __cplusplus
}
#endif

#endif //__BUF_FMT_PARSER_H__
