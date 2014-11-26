#include <stdlib.h>
#include <stdint.h>
#include "lib/automem.h"
#include "protocol.h"

unsigned char fmt_get_type(FMT* fmt) {
	return fmt->t;
}

unsigned char fmt_get_byte(FMT* fmt) {
	return fmt->m_byte;
}

short fmt_get_short(FMT* fmt) {
	return fmt->m_short;
}

unsigned short fmt_get_ushort(FMT* fmt) {
	return fmt->m_ushort;
}

int fmt_get_int(FMT* fmt) {
	return fmt->m_int;
}

unsigned int fmt_get_uint(FMT* fmt) {
	return fmt->m_uint;
}

__int64 fmt_get_long(FMT* fmt) {
	return fmt->m_long;
}

__UInt64 fmt_get_ulong(FMT* fmt) {
	return fmt->m_ulong;
}

double fmt_get_double(FMT* fmt) {
	return fmt->m_double;
}

__int64 fmt_get_date(FMT* fmt) {
	return fmt->m_date;
}

unsigned char* fmt_get_string(FMT* fmt) {
	return fmt->m_str;
}

unsigned int fmt_get_string_len(FMT* fmt) {
	return fmt->m_slen;
}

// returns free function, which can be used to free `data` later.
void* fmt_packet(FMT* fmt, short cmd, char* key, unsigned char** data, unsigned int* len) {
	automem_t mem;
	automem_init(&mem, 32);
	if(key == NULL)
		fmt_packet_build(&mem, fmt, cmd);
	else
		fmt_packet_build_v2(&mem, fmt, cmd, key);
	*data = mem.pdata;
	*len  = mem.size;
	return mem._free;
}

void fmt_freemem(void* freefn, void* mem) {
	// __free defined in automem.h: `typedef void (*  __free)(void * ptr);`
	((__free)freefn)(mem);
}
