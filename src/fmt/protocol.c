#include <stdio.h>
#include <stdlib.h>
//#include <stdint.h>
#if !defined(_MSC_VER)
#include <assert.h>
#endif
#include <string.h>

#include "lib/arraylist.h"
#include "lib/linkhash.h"
#include "lib/automem.h"

#include "lib/xxtea.h"
#include "protocol.h"

/*
static union { char c[4]; unsigned long mylong; } _endian_test_ = {{ 'l', '?', '?', 'b' } };
*/


#define SWAP_16(x) \
	(short)((((short)(x) & 0x00ff) << 8) | \
	(((short)(x) & 0xff00) >> 8) \
	)

#define SWAP_32(x) \
	(int)((((int)(x) & 0xff000000) >> 24) | \
	(((int)(x) & 0x00ff0000) >> 8) | \
	(((int)(x) & 0x0000ff00) << 8) | \
	(((int)(x) & 0x000000ff) << 24) \
	)

#define SWAP_64(x) \
	(__int64)(\
	(__int64)(((__int64)(x) & 0xFF00000000000000) >> 56) | \
	(__int64)(((__int64)(x) & 0x00FF000000000000) >> 40) | \
	(__int64)(((__int64)(x) & 0x0000FF0000000000) >> 24) | \
	(__int64)(((__int64)(x) & 0x000000FF00000000) >> 8) | \
	(__int64)(((__int64)(x) & 0x00000000FF000000) << 8) | \
	(__int64)(((__int64)(x) & 0x0000000000FF0000) << 24) | \
	(__int64)(((__int64)(x) & 0x000000000000FF00) << 40) | \
	(__int64)(((__int64)(x) & 0x00000000000000FF) << 56)\
	)

#define SWAP_U16(x) \
	(unsigned short)((((unsigned short)(x) & 0x00ff) << 8) | \
	(((unsigned short)(x) & 0xff00) >> 8) \
	)
#define SWAP_U32(x) \
	(unsigned int)((((int)(x) & 0xff000000) >> 24) | \
	(((unsigned int)(x) & 0x00ff0000) >> 8) | \
	(((unsigned int)(x) & 0x0000ff00) << 8) | \
	(((unsigned int)(x) & 0x000000ff) << 24) \
	)

#define SWAP_U64(x) \
	(__UInt64)(\
	(__UInt64)(((__UInt64)(x) & 0xFF00000000000000) >> 56) | \
	(__UInt64)(((__UInt64)(x) & 0x00FF000000000000) >> 40) | \
	(__UInt64)(((__UInt64)(x) & 0x0000FF0000000000) >> 24) | \
	(__UInt64)(((__UInt64)(x) & 0x000000FF00000000) >> 8) | \
	(__UInt64)(((__UInt64)(x) & 0x00000000FF000000) << 8) | \
	(__UInt64)(((__UInt64)(x) & 0x0000000000FF0000) << 24) | \
	(__UInt64)(((__UInt64)(x) & 0x000000000000FF00) << 40) | \
	(__UInt64)(((__UInt64)(x) & 0x00000000000000FF) << 56)\
	)

/**  reads an integer in FMT format */
static int fmt_read_length(unsigned char * bytes,int len,int * used)
{
	unsigned char * cp = (unsigned char *)bytes;
	int acc,mask,r,tmp;
	if(len < 1){
		*used = 0;
		return -1; //长度不足1 读取失败
	}

	acc = *cp++;


	if(acc < 128)
	{
		*used = 1;
		return acc;
	}
	else
	{
		if(len < 2){
			*used = 0;
			return -1; //长度不足2 读取失败
		}

		acc = (acc & 0x7f) << 7;
		tmp = *cp++;

		*used = 2;

		if(tmp < 128)
		{
			acc = acc | tmp;
		}
		else
		{
			if(len < 3){
				*used = 0;
				return -1; //长度不足2 读取失败
			}

			acc = (acc | (tmp & 0x7f)) << 7;
			tmp = *cp++;

			if(tmp < 128)
			{
				*used = 3;
				acc = acc | tmp;
			}
			else
			{
				if(len < 4){
					*used = 0;
					return -1; //长度不足2 读取失败
				}
				acc = (acc | (tmp & 0x7f)) << 8;
				tmp = *cp++;
				acc = acc | tmp;

				* used = 4;
			}
		}
	}
	/* To sign extend a value from some number of bits to a greater number of bits just copy the sign bit into all the additional bits in the new format */
	/* convert/sign extend the 29bit two's complement number to 32 bi */
	mask = 1 << 28;  /*  mas */
	r = -(acc & mask) | acc;

	return r;
}

static int fmt_write_length(automem_t * pmem, int val)
{

	val &= 0x1FFFFFFF;
	if(val < 0x80)
	{
		automem_append_byte(pmem,(unsigned char)val);
		return 1;
	}
	else if(val < 0x4000)
	{
		automem_append_byte(pmem,(unsigned char)((val >> 7 & 0x7f) | 0x80));
		automem_append_byte(pmem,(unsigned char)(val & 0x7f));
		return 2;
	}
	else if(val < 0x200000)
	{
		automem_append_byte(pmem,(unsigned char)((val >> 14 & 0x7f) | 0x80));
		automem_append_byte(pmem,(unsigned char)((val >> 7 & 0x7f) | 0x80));
		automem_append_byte(pmem,(unsigned char)(val & 0x7f));
		return 3;
	}
	else
	{
		char tmp[4] = { (val >> 22 & 0x7f) | 0x80, (val >> 15 & 0x7f) | 0x80, (val >> 8 & 0x7f) | 0x80, val & 0xff };
		automem_append_voidp(pmem, tmp, 4);
	}

	return 4;
}

////


FMT * fmt_object_get(FMT * fmt)
{
	fmt->_ref++;
	return fmt;
}

void fmt_object_put(FMT * fmt)
{
	//printf("data ref = [%d]\n",fmt->_ref);
	fmt->_ref--;
//	_assert(fmt->_ref >= 0);
	if(fmt->_ref ==0)
	{
		fmt->_delete(fmt);
	}
}



static void fmt_base_delete(FMT *o)
{
	free(o);
}
#define fmt_string_delete	fmt_base_delete


static int fmt_string_serial(FMT * fmt,automem_t * pmem)
{
	int size=1;
	automem_append_byte(pmem,PDT_STRING);

	size += fmt_write_length(pmem,fmt->m_slen);

	if(0 < fmt->m_slen)
	{
		automem_append_voidp(pmem,fmt->m_str,fmt->m_slen);
		size+=fmt->m_slen;
	}
	return size;
}

FMT * fmt_new_string(const void* str, unsigned int len)
{
	FMT * fmt;
	if(0 == len)
	{
		if(NULL != str)
			len = strlen((const char*)str);
	}

	fmt = (FMT *)malloc(sizeof(FMT) + len + 1);

	fmt->t=PDT_STRING;
	fmt->_ref = 0;
	fmt->_delete = &fmt_string_delete;
	fmt->_serialize = &fmt_string_serial;

	fmt->m_str = (char *)(fmt + 1);
	if(0 != (fmt->m_slen = len))
	{
		fmt->m_str[len] = '\0';
		memcpy(fmt->m_str, str , len);
	}else{
		fmt->m_str = NULL;
	}

	return fmt;
}

static int fmt_byte_serialize(FMT * fmt,automem_t * pmem)
{
	automem_append_byte(pmem,PDT_BYTE);
	automem_append_byte(pmem,fmt->m_byte);
	return 2;
}

FMT * fmt_new_byte(unsigned char bt)
{
	FMT * fmt = (FMT *)malloc(sizeof(FMT));
	fmt->_ref = 0;
	fmt->t = PDT_BYTE;
	fmt->m_byte = bt;
	fmt->_delete = &fmt_base_delete;
	fmt->_serialize = &fmt_byte_serialize;

	return fmt;
}

static int fmt_short_serialize(FMT * fmt,automem_t * pmem)
{
#if defined(LITTLEENDIANNESS)
	short m_short = SWAP_16(fmt->m_short);
	automem_append_byte(pmem,PDT_SHORT);
	automem_append_voidp(pmem,&m_short,sizeof(short));
#else
	automem_append_byte(pmem,PDT_SHORT);
	automem_append_voidp(pmem,&fmt->m_short,sizeof(short));
#endif
	return sizeof(short)+1;
}


FMT * fmt_new_short(short val)
{
	FMT * fmt = (FMT *)malloc(sizeof(FMT));
	fmt->_ref = 0;
	fmt->t = PDT_SHORT;
	fmt->m_short = val;
	fmt->_delete = &fmt_base_delete;
	fmt->_serialize = &fmt_short_serialize;

	return fmt;
}

static int fmt_ushort_serialize(FMT * fmt,automem_t * pmem)
{
#if defined(LITTLEENDIANNESS)
	unsigned short m_ushort = SWAP_U16(fmt->m_ushort);
	automem_append_byte(pmem,PDT_USHORT);
	automem_append_voidp(pmem,&m_ushort,sizeof(unsigned short));
#else
	automem_append_byte(pmem,PDT_USHORT);
	automem_append_voidp(pmem,&fmt->m_ushort,sizeof(unsigned short));
#endif
	return sizeof(unsigned short)+1;
}

FMT * fmt_new_ushort(unsigned short val)
{
	FMT * fmt = (FMT *)malloc(sizeof(FMT));
	fmt->_ref = 0;
	fmt->t = PDT_USHORT;
	fmt->m_ushort = val;
	fmt->_delete = &fmt_base_delete;
	fmt->_serialize = &fmt_ushort_serialize;

	return fmt;
}


static int fmt_integer_serialize(FMT * fmt,automem_t * pmem)
{
#if defined(LITTLEENDIANNESS)
	int m_int= SWAP_32(fmt->m_int);
	automem_append_byte(pmem,PDT_INTEGER);
	automem_append_voidp(pmem,&m_int,sizeof(int));
#else
	automem_append_byte(pmem,PDT_INTEGER);
	automem_append_voidp(pmem,&fmt->m_int,sizeof(int));
#endif
	return sizeof(int)+1;
}


FMT * fmt_new_integer(int val)
{
	FMT * fmt = (FMT *)malloc(sizeof(FMT));
	fmt->_ref = 0;
	fmt->t = PDT_INTEGER;
	fmt->m_int = val;
	fmt->_delete = &fmt_base_delete;
	fmt->_serialize = &fmt_integer_serialize;
	return fmt;
}

static int fmt_uinteger_serialize(FMT * fmt,automem_t * pmem)
{
#if defined(LITTLEENDIANNESS)
	unsigned int m_uint= SWAP_U32(fmt->m_uint);
	automem_append_byte(pmem,PDT_UINTEGER);
	automem_append_voidp(pmem,&m_uint,sizeof(unsigned int));
#else
	automem_append_byte(pmem,PDT_UINTEGER);
	automem_append_voidp(pmem,&fmt->m_uint,sizeof(unsigned int));
#endif
	return sizeof(unsigned int)+1;
}

FMT * fmt_new_uinteger(unsigned int val)
{
	FMT * fmt = (FMT *)malloc(sizeof(FMT));
	fmt->_ref = 0;
	fmt->t = PDT_UINTEGER;
	fmt->m_uint = val;
	fmt->_delete = &fmt_base_delete;
	fmt->_serialize = &fmt_uinteger_serialize;
	return fmt;
}


static int fmt_long_serialize(FMT * fmt,automem_t * pmem)
{
#if defined(LITTLEENDIANNESS)
	__UInt64 m_long = SWAP_64(fmt->m_long);
	automem_append_byte(pmem,PDT_LONG);
	automem_append_voidp(pmem,&m_long,sizeof(__int64));
#else
	automem_append_byte(pmem,PDT_LONG);
	automem_append_voidp(pmem,&fmt->m_long,sizeof(__int64));
#endif

	return sizeof(__int64)+1;
}

FMT * fmt_new_long(__int64 val)
{
	FMT * fmt = (FMT *)malloc(sizeof(FMT));
	fmt->_ref = 0;
	fmt->t = PDT_LONG;
	fmt->m_long = val;
	fmt->_delete = &fmt_base_delete;
	fmt->_serialize = &fmt_long_serialize;

	return fmt;
}

static int fmt_ulong_serialize(FMT * fmt,automem_t * pmem)
{
#if defined(LITTLEENDIANNESS)
	__UInt64 m_ulong = SWAP_U64(fmt->m_ulong);
	automem_append_byte(pmem,PDT_ULONG);
	automem_append_voidp(pmem,&m_ulong,sizeof(__UInt64));
#else
	automem_append_byte(pmem,PDT_ULONG);
	automem_append_voidp(pmem,&fmt->m_ulong,sizeof(__UInt64));
#endif

	return sizeof(__UInt64)+1;
}

FMT * fmt_new_ulong(__UInt64 val)
{
	FMT * fmt = (FMT *)malloc(sizeof(FMT));
	fmt->_ref = 0;
	fmt->t = PDT_ULONG;
	fmt->m_ulong = val;
	fmt->_delete = &fmt_base_delete;
	fmt->_serialize = &fmt_ulong_serialize;

	return fmt;
}


static int fmt_double_serialize(FMT * fmt,automem_t * pmem)
{
	automem_append_byte(pmem,PDT_DOUBLE);
	automem_append_voidp(pmem,&fmt->m_double,sizeof(double));
	return sizeof(double)+1;
}


FMT * fmt_new_double(double val)
{
	FMT * fmt = (FMT *)malloc(sizeof(FMT));
	fmt->_ref = 0;
	fmt->t = PDT_DOUBLE;
	fmt->m_double = val;
	fmt->_delete = &fmt_base_delete;
	fmt->_serialize = &fmt_double_serialize;
	return fmt;
}


static int fmt_datetime_serialize(FMT * fmt,automem_t * pmem)
{
#if defined(LITTLEENDIANNESS)
	__int64 m_date = SWAP_64(fmt->m_date);
	automem_append_byte(pmem,PDT_DATETIME);
	automem_append_voidp(pmem,&m_date,sizeof(__int64));
#else
	automem_append_byte(pmem,PDT_DATETIME);
	automem_append_voidp(pmem,&fmt->m_date,sizeof(__int64));
#endif
	return (1 + sizeof(__int64));
}

static int fmt_boolean_serialize(FMT * fmt, automem_t * mem)
{
	unsigned char b = fmt->m_byte;
	automem_append_byte(mem,PDT_BOOL);
	automem_append_voidp(mem, &fmt->m_byte,sizeof(unsigned char));
	return 1 + sizeof(unsigned char);
}
static int fmt_null_serialize(FMT * fmt, automem_t * mem)
{
	automem_append_byte(mem,PDT_NULL);
	return 1;
}

FMT * fmt_new_datetime(__int64 val)
{
	FMT * fmt = (FMT *)malloc(sizeof(FMT));
	fmt->_ref = 0;
	fmt->t = PDT_DATETIME;
	fmt->m_date = val;
	fmt->_delete = &fmt_base_delete;
	fmt->_serialize = &fmt_datetime_serialize;

	return fmt;
}
FMT * fmt_new_boolean(int b)
{
	FMT * fmt = (FMT *)malloc(sizeof(FMT));
	fmt->_ref = 0;
	fmt->t = PDT_BOOL;
	fmt->m_byte = b?1:0;
	fmt->_delete = &fmt_base_delete;
	fmt->_serialize = &fmt_boolean_serialize;

	return fmt;
}

FMT * fmt_new_null(void){
	FMT * fmt = (FMT *)malloc(sizeof(FMT));
	fmt->_ref = 0;
	fmt->t = PDT_NULL;
	fmt->_delete = &fmt_base_delete;
	fmt->_serialize = &fmt_null_serialize;
	return fmt;
}

static void fmt_array_free(void * data)
{
	fmt_object_put((FMT *)data);
}

static void fmt_array_delete(FMT * o)
{
	array_list_free(o->m_array);
	free(o);
}

static int fmt_array_serialize(FMT * fmt,automem_t * pmem)
{
	int i,len = array_list_length(fmt->m_array),size=0;
	FMT * item;

	automem_append_byte(pmem,PDT_ARRAY); //写入类型识别码

	for(i=0;i<len;i++)
	{
		item = (FMT *)array_list_get_idx(fmt->m_array,i);
		if(item)
		{
			size+=item->_serialize(item,pmem);
		}
	}
	automem_append_byte(pmem,_PDT_END);

	return 2+size;
}

FMT * fmt_new_array()
{
	FMT * fmt = (FMT *)malloc(sizeof(FMT));
	fmt->_ref = 0;
	fmt->t = PDT_ARRAY;
	fmt->m_array = array_list_new(fmt_array_free);

	fmt->_delete = &fmt_array_delete;
	fmt->_serialize = &fmt_array_serialize;

	return fmt;
}

unsigned int fmt_array_length(FMT * fmt)
{
	return array_list_length(fmt->m_array);
}

void fmt_array_append(FMT * fmt,FMT * item)
{
	array_list_add(fmt->m_array, fmt_object_get(item));
}

void fmt_array_remove(FMT * fmt,unsigned int idx)
{
	array_list_del_idx(fmt->m_array,idx);
}

FMT * fmt_array_get_idx(FMT * fmt,unsigned int idx)
{
	return (FMT *)array_list_get_idx(fmt->m_array, idx);
}

static void fmt_object_delete(FMT * data)
{
	lh_table_free(data->m_object); //遍历释放成员
	free(data);//释放对象本身
}

static void fmt_object_lh_entry_free(struct lh_entry *ent)
{
	free(ent->k); //	释放键值
	fmt_object_put((FMT *)ent->v); //	成员引用数减少1
}

static int _fmt_write_string(automem_t * pmem,char * string)
{
	int size = 1,kLen = strlen(string);

	automem_append_byte(pmem,PDT_STRING);
	size += fmt_write_length(pmem, kLen);
	automem_append_voidp(pmem, string, kLen);

	return size+kLen;
}

static int fmt_object_serialize(FMT * fmt,automem_t * pmem)
{
	int size=0;
	struct lh_entry *ent;
	automem_append_byte(pmem,PDT_OBJECT); //写入类型识别码

	lh_foreach(fmt->m_object,ent)
	{
		//写入 Key
		size +=_fmt_write_string(pmem,(char*)ent->k);
		size +=((FMT *)ent->v)->_serialize((FMT *)ent->v,pmem);

	}
	automem_append_byte(pmem,_PDT_END); //写入结束标志
	return size+2;
}

#define FMT_OBJECT_DEF_HASH_ENTRIES 8 //哈希表初始大小


FMT * fmt_new_object()
{
	FMT * fmt = (FMT *)malloc(sizeof(FMT));
	fmt->_ref = 0;
	fmt->t = PDT_OBJECT;

	fmt->_delete = &fmt_object_delete;
	fmt->_serialize = &fmt_object_serialize;

	fmt->m_object = lh_kchar_table_new(FMT_OBJECT_DEF_HASH_ENTRIES,NULL,fmt_object_lh_entry_free);

	return fmt;
}

static int fmt_raw_serialize(FMT * fmt,automem_t * pmem)
{
	automem_append_voidp(pmem, fmt->m_str, fmt->m_slen);
	return fmt->m_slen;
}

#define SKIP_VER_HEADER		(sizeof(unsigned short) +1)

static int fmt_packet_build_header(automem_t * pmem, short cmd, unsigned int dataSize)
{
	short ver = FMT_PROTOCOL_VERSION_V1;

#if defined(LITTLEENDIANNESS)
	cmd = SWAP_U16(cmd);
#endif

	automem_append_byte(pmem, 0x03);
	automem_append_voidp(pmem, &ver,sizeof(short));
	automem_append_voidp(pmem, &cmd,sizeof(short));

	if(dataSize > 0)
	{
		fmt_write_length(pmem, dataSize);//写入整数
	}else{
		fmt_write_length(pmem, 0);	//提交的数据包并未带有主体数据.
	}

	return pmem->size;
}

int fmt_packet_build(automem_t * pmem, FMT * fmt, short cmd)
{
	if(NULL != fmt)
	{
		automem_t mem;
		int dataSize;
		automem_init(&mem, 128);
		dataSize = fmt->_serialize(fmt, &mem);
		fmt_packet_build_header(pmem, cmd,  dataSize);
		automem_append_voidp(pmem, mem.pdata, mem.size);
		automem_uninit(&mem);
	}
	else
	{
		fmt_packet_build_header(pmem, cmd,  0);
	}

	return pmem->size;
}

#if defined(FMT_HAS_V2)
int fmt_packet_build_v2(automem_t * pmem, FMT * fmt, short cmd, char * key)
{
	automem_t mem;
	unsigned short ver = FMT_PROTOCOL_VERSION_V2;
	char * encrypted = NULL;
	xxtea_long encrypted_size = 0;

	if(NULL == key){
		return 0;
	}

	automem_init(&mem,128);
	fmt_packet_build(&mem, fmt, cmd);
	encrypted = (char*)xxtea_encrypt(mem.pdata +SKIP_VER_HEADER, mem.size - SKIP_VER_HEADER, key, &encrypted_size);
	automem_uninit(&mem);

	automem_append_byte(pmem, 0x03);
	automem_append_voidp(pmem, &ver,sizeof(unsigned short)); //版本号
	fmt_write_length(pmem,encrypted_size);
	automem_append_voidp(pmem, encrypted, encrypted_size);

	free(encrypted);
	return pmem->size;
}
#endif


/*********************************************************
*	若 fmt对象中存在同名属性，则前一个会被自动减除引用计数.
*
*/
void fmt_object_add(FMT * fmt, const char* key,FMT * val)
{
#if defined(_MSC_VER)
	char * mKey = _strdup(key);
#else
	char * mKey = strdup(key);
#endif
	lh_table_insert(fmt->m_object,mKey,fmt_object_get(val));
}


void fmt_object_remove(FMT * fmt, const char * key)
{
	lh_table_delete(fmt->m_object,key);
}


FMT * fmt_object_lookup(FMT * fmt, const char * key)
{
	return (FMT *)lh_table_lookup(fmt->m_object, key);
}

/***********************************************************************
*	获取 FMT 对象的成员数.
*/
int fmt_object_total(FMT * fmt)
{
	return fmt->m_object->count;
}

enum{
	__PST_HEADER = 0,
	__PST_VERSION =1,
	__PST_COMMAND =2,
	__PST_DATASTART = 4,
	__PST_DATA = 5,

	__PST_ENCRYPTED_DATA_START,
	__PST_ENCRYPTED_DATA,

	__PST_FINISHED = 0xFF		// 解析完成数据对象.
};


FMTParserState * fmt_parser_new_state()
{
	FMTParserState * t = (FMTParserState*)malloc(sizeof(FMTParserState));
	t->t=__PST_HEADER;
	t->result = NULL;
	t->key = NULL;
	return t;
}



/* 关闭状态机 */
void fmt_parser_close_state(FMTParserState * st)
{
	if(NULL != st->result){
		fmt_object_put(st->result);
		st->result = NULL;
	}
	if(NULL !=st->key)
		free(st->key);
	free(st);
}

/* 成功返回 true 失败返回 false */
int fmt_parser_set_key(FMTParserState * t, const char * key)
{
	if(NULL != t->key)
		free(t->key);
	t->key = NULL;

	if(NULL != key){
		t->key = (char *)malloc(16);
		memcpy(t->key, key, 16);
	}
	return 1;
}
typedef struct _pdt_fmt_tok{
	unsigned char * bytes;
	unsigned int pos;
	unsigned int size;
}_pdt_fmt_tok;


static FMT * fmt_parse_array(struct _pdt_fmt_tok * tok);


static unsigned char * fmt_read_string_key(struct _pdt_fmt_tok * tok)
{
	int used;
	unsigned int slen = fmt_read_length(&tok->bytes[tok->pos],tok->size - tok->pos,&used);
	if(slen >= 0 && ((tok->pos+slen) <= tok->size))
	{
		unsigned char * key;
		tok->pos+=used;

		key = (unsigned char * )malloc(slen+1);
		key[slen]='\0';
		memcpy(key,(void*)&tok->bytes[tok->pos],slen);

		tok->pos += slen;

		return key;
	}
	return NULL;

}


static FMT * fmt_read_string(struct _pdt_fmt_tok * tok)
{
	int used;
	unsigned int slen = fmt_read_length(&tok->bytes[tok->pos],tok->size - tok->pos,&used);
	if(slen >= 0)
	{
		tok->pos+=used;

		if(tok->size >= (tok->pos + slen))
		{
			if(slen>0)
			{
				// string to len
				FMT * fmt = fmt_new_string(&tok->bytes[tok->pos],slen);
				tok->pos += slen;
				return fmt;
			}else{
				// null string
				FMT * fmt = fmt_new_string(NULL , 0);
				return fmt;
			}
		}
	}
	return NULL;
}


static FMT * fmt_parse_object(struct _pdt_fmt_tok * tok)
{
	FMT * arr = fmt_new_object();
	FMT *item = NULL;
	unsigned char * key = NULL;
	unsigned char t = tok->bytes[tok->pos++];

	while(_PDT_END != t)
	{
		//if(t == PDT_STRING)
		//{
		key = fmt_read_string_key(tok);
		//}
		item = NULL;
		t = tok->bytes[tok->pos++];
		switch(t)
		{
		case PDT_BYTE:
			if( (tok->size - tok->pos) > sizeof(unsigned char))
			{
				item = fmt_new_byte(tok->bytes[tok->pos++]);
			}
			break;
		case PDT_BOOL:
			if( (tok->size - tok->pos) >= sizeof(unsigned char)){
				item = fmt_new_boolean(tok->bytes[tok->pos++]);
			}
			break;
		case PDT_NULL:
			item = fmt_new_null();
			break;
		case PDT_SHORT:
			if( (tok->size - tok->pos) > sizeof(short))
			{
#if defined(LITTLEENDIANNESS)
				item = fmt_new_short(SWAP_16(*(short*)&tok->bytes[tok->pos]));
#else
				item= fmt_new_short(*(short*)&tok->bytes[tok->pos]);
#endif
				tok->pos += sizeof(short);
			}
			break;
		case PDT_USHORT:
			if( (tok->size - tok->pos) > sizeof(unsigned short))
			{
			#if defined(LITTLEENDIANNESS)
				item = fmt_new_ushort(SWAP_U16(*(unsigned short*)&tok->bytes[tok->pos]));
			#else
				item= fmt_new_ushort(*(unsigned short*)&tok->bytes[tok->pos]);
			#endif
				tok->pos += sizeof(unsigned short);
			}
			break;
		case PDT_INTEGER:
			if( (tok->size - tok->pos) > sizeof(int))
			{
#if defined(LITTLEENDIANNESS)
				item = fmt_new_integer(SWAP_32(*(int*)&tok->bytes[tok->pos]));
#else
				item= fmt_new_integer(*(int*)&tok->bytes[tok->pos]);
#endif

				tok->pos += sizeof(int);
			}
			break;
		case PDT_UINTEGER:
			if( (tok->size - tok->pos) > sizeof(unsigned int))
			{
			#if defined(LITTLEENDIANNESS)
				item = fmt_new_uinteger(SWAP_U32(*(unsigned int*)&tok->bytes[tok->pos]));
			#else
				item= fmt_new_uinteger(*(unsigned int*)&tok->bytes[tok->pos]);
			#endif

				tok->pos += sizeof(unsigned int);
			}
			break;
		case PDT_LONG:
			if( (tok->size - tok->pos) > sizeof(__int64))
			{
#if defined(LITTLEENDIANNESS)
				item = fmt_new_long(SWAP_64(*(__int64*)&tok->bytes[tok->pos]));
#else

				item= fmt_new_long(*(__int64*)&tok->bytes[tok->pos]);
#endif

				//				item = fmt_new_long(*(__int64*)&tok->bytes[tok->pos]);
				tok->pos += sizeof(__int64);
			}
			break;
		case PDT_ULONG:
			if( (tok->size - tok->pos) > sizeof(__int64))
			{
			#if defined(LITTLEENDIANNESS)
				item = fmt_new_ulong(SWAP_U64(*(__UInt64*)&tok->bytes[tok->pos]));
			#else
				item= fmt_new_ulong(*(__UInt64*)&tok->bytes[tok->pos]);
			#endif
				tok->pos += sizeof(__UInt64);
			}
			break;
		case PDT_DOUBLE:
			if( (tok->size - tok->pos) > sizeof(double))
			{
				item = fmt_new_double(*(double*)&tok->bytes[tok->pos]);
				tok->pos += sizeof(double);
			}

			break;
		case PDT_DATETIME:
			if( (tok->size - tok->pos) > sizeof(__int64))
			{
#if defined(LITTLEENDIANNESS)
				item = fmt_new_datetime(SWAP_64(*(__int64 *)&tok->bytes[tok->pos]));
#else
				item= fmt_new_datetime(*(__int64 *)&tok->bytes[tok->pos]);
#endif

				tok->pos += sizeof(__int64);
			}
			break;

		case PDT_STRING:
			item = fmt_read_string(tok);
			break;

		case PDT_ARRAY:
			item = fmt_parse_array(tok);
			break;
		case PDT_OBJECT:
			item = fmt_parse_object(tok);
			break;
		}


		if(NULL != item && key != NULL)//读取条目成功
		{
			lh_table_insert(arr->m_object, key, fmt_object_get(item));
		}
		else
		{
			//当条目解析失败 析构当前数组，并返回 NULL
			fmt_object_put(arr);
			if(NULL != key) free(key);
			return NULL;
		}

		t = tok->bytes[tok->pos++];
	}
	return arr;

}


static FMT * fmt_parse_array(struct _pdt_fmt_tok * tok)
{
	FMT * arr = fmt_new_array();
	FMT *item = NULL;
	unsigned char t = tok->bytes[tok->pos++];
	while(_PDT_END != t)
	{
		item = NULL;
		switch(t)
		{
		case PDT_BYTE:
			if( (tok->size - tok->pos) > sizeof(unsigned char))
			{
				item = fmt_new_byte(tok->bytes[tok->pos++]);
			}
			break;
		case PDT_BOOL:
			if( (tok->size - tok->pos) >= sizeof(unsigned char)){
				item = fmt_new_boolean(tok->bytes[tok->pos++]);
			}
			break;
		case PDT_NULL:
			item = fmt_new_null();
			break;
		case PDT_SHORT:
			if( (tok->size - tok->pos) > sizeof(short))
			{
#if defined(LITTLEENDIANNESS)
				item = fmt_new_short(SWAP_16(*(short*)&tok->bytes[tok->pos]));
#else
				item= fmt_new_short(*(short*)&tok->bytes[tok->pos]);
#endif
				//item = fmt_new_short(*(short*)&tok->bytes[tok->pos]);
				tok->pos += sizeof(short);
			}
			break;
		case PDT_USHORT:
			if( (tok->size - tok->pos) > sizeof(unsigned short))
			{
			#if defined(LITTLEENDIANNESS)
				item = fmt_new_ushort(SWAP_U16(*(unsigned short*)&tok->bytes[tok->pos]));
			#else
				item= fmt_new_ushort(*(unsigned short*)&tok->bytes[tok->pos]);
			#endif
				tok->pos += sizeof(unsigned short);
			}
			break;
		case PDT_INTEGER:
			if( (tok->size - tok->pos) > sizeof(int))
			{
#if defined(LITTLEENDIANNESS)
				item = fmt_new_integer(SWAP_32(*(int*)&tok->bytes[tok->pos]));
#else
				item= fmt_new_integer(*(int*)&tok->bytes[tok->pos]);
#endif
				//item = fmt_new_integer(*(int*)&tok->bytes[tok->pos]);
				tok->pos += sizeof(int);
			}
			break;
		case PDT_UINTEGER:
			if( (tok->size - tok->pos) > sizeof(unsigned int))
			{
			#if defined(LITTLEENDIANNESS)
				item = fmt_new_uinteger(SWAP_U32(*(unsigned int*)&tok->bytes[tok->pos]));
			#else
				item= fmt_new_uinteger(*(unsigned int*)&tok->bytes[tok->pos]);
			#endif

				tok->pos += sizeof(unsigned int);
			}
			break;
		case PDT_LONG:
			if( (tok->size - tok->pos) > sizeof(__int64))
			{
#if defined(LITTLEENDIANNESS)
				item = fmt_new_long(SWAP_64(*(__int64*)&tok->bytes[tok->pos]));
#else
				item= fmt_new_long(*(__int64*)&tok->bytes[tok->pos]);
#endif

				//item = fmt_new_long(*(__int64*)&tok->bytes[tok->pos]);
				tok->pos += sizeof(__int64);
			}
			break;
		case PDT_ULONG:
			if( (tok->size - tok->pos) > sizeof(__UInt64))
			{
			#if defined(LITTLEENDIANNESS)
				item = fmt_new_ulong(SWAP_U64(*(__UInt64*)&tok->bytes[tok->pos]));
			#else
				item= fmt_new_ulong(*(__UInt64*)&tok->bytes[tok->pos]);
			#endif
				tok->pos += sizeof(__UInt64);
			}
			break;
		case PDT_DOUBLE:
			if( (tok->size - tok->pos) > sizeof(double))
			{
				item = fmt_new_double(*(double*)&tok->bytes[tok->pos]);
				tok->pos += sizeof(double);
			}

			break;
		case PDT_DATETIME:
			if( (tok->size - tok->pos) > sizeof(__int64))
			{
#if defined(LITTLEENDIANNESS)
				item = fmt_new_datetime(SWAP_64(*(__int64 *)&tok->bytes[tok->pos]));
#else
				item= fmt_new_datetime(*(__int64 *)&tok->bytes[tok->pos]);
#endif
				//item = fmt_new_datetime(*(int*)&tok->bytes[tok->pos]);
				tok->pos += sizeof(__int64);
			}
			break;
		case PDT_STRING:
			item = fmt_read_string(tok);
			break;
		case PDT_ARRAY:
			item = fmt_parse_array(tok);
			break;
		case PDT_OBJECT:
			item = fmt_parse_object(tok);
			break;
		}


		if(NULL != item)//读取条目成功
		{
			array_list_add(arr->m_array, fmt_object_get(item));
		}
		else
		{
			//当条目解析失败 析构当前数组，并返回 NULL
			fmt_object_put(arr);
			return NULL;
		}

		t = tok->bytes[tok->pos++];

	}
	return arr;

}


static FMT * fmt_token_parse(struct _pdt_fmt_tok * tok)
{
	unsigned char t = tok->bytes[tok->pos++];
	FMT  * result = NULL;

	//printf("%s-%d\n","parse fmt type=",t);
	switch(t)
	{
	case PDT_BYTE:
		if( (tok->size - tok->pos) >= sizeof(unsigned char))
		{
			result = fmt_new_byte(tok->bytes[tok->pos++]);
		}
		break;
	case PDT_BOOL:
		if( (tok->size - tok->pos) >= sizeof(unsigned char)){
			result = fmt_new_boolean(tok->bytes[tok->pos++]);
		}
		break;
	case PDT_NULL:
		result = fmt_new_null();
		break;
	case PDT_SHORT:
		if( (tok->size - tok->pos) >= sizeof(short))
		{
#if defined(LITTLEENDIANNESS)
			result = fmt_new_short(SWAP_16(*(short*)&tok->bytes[tok->pos]));
#else
			result = fmt_new_short(*(short*)&tok->bytes[tok->pos]);
#endif

			tok->pos += sizeof(short);
		}
		break;
	case PDT_USHORT:
		if( (tok->size - tok->pos) >= sizeof(unsigned short))
		{
	#if defined(LITTLEENDIANNESS)
			result = fmt_new_short(SWAP_U16(*(unsigned short*)&tok->bytes[tok->pos]));
	#else
			result = fmt_new_short(*(unsigned short*)&tok->bytes[tok->pos]);
	#endif
			tok->pos += sizeof(unsigned short);
		}
		break;
	case PDT_INTEGER:
		if( (tok->size - tok->pos) >= sizeof(int))
		{
#if defined(LITTLEENDIANNESS)
			result = fmt_new_integer(SWAP_32(*(int*)&tok->bytes[tok->pos]));
#else
			result = fmt_new_integer(*(int*)&tok->bytes[tok->pos]);
#endif

			tok->pos += sizeof(int);
		}
		break;
	case PDT_UINTEGER:
		printf("uinteger");
		if( (tok->size - tok->pos) >= sizeof(unsigned int))
		{
	#if defined(LITTLEENDIANNESS)
			result = fmt_new_uinteger(SWAP_U32(*(unsigned int*)&tok->bytes[tok->pos]));
	#else
			result = fmt_new_uinteger(*(unsigned int*)&tok->bytes[tok->pos]);
	#endif
			tok->pos += sizeof(unsigned int);
		}
		break;
	case PDT_LONG:
		if( (tok->size - tok->pos) >= sizeof(__int64))
		{
#if defined(LITTLEENDIANNESS)
			result = fmt_new_long(SWAP_64(*(__int64*)&tok->bytes[tok->pos]));
#else
			result = fmt_new_long(*(__int64*)&tok->bytes[tok->pos]);
#endif

			tok->pos += sizeof(__int64);
		}
		break;
	case PDT_ULONG:
		if( (tok->size - tok->pos) >= sizeof(__UInt64))
		{
	#if defined(LITTLEENDIANNESS)
			result = fmt_new_ulong(SWAP_U64(*(__UInt64*)&tok->bytes[tok->pos]));
	#else
			result = fmt_new_ulong(*(__UInt64*)&tok->bytes[tok->pos]);
	#endif
			tok->pos += sizeof(__UInt64);
		}
		break;
	case PDT_DOUBLE:
		if( (tok->size - tok->pos) >= sizeof(double))
		{
			result = fmt_new_double(*(double*)&tok->bytes[tok->pos]);
			tok->pos += sizeof(double);
		}

		break;
	case PDT_DATETIME:
		if( (tok->size - tok->pos) >= sizeof(__int64))
		{
#if defined(LITTLEENDIANNESS)
			result = fmt_new_datetime(SWAP_64(*(__int64 *)&tok->bytes[tok->pos]));
#else
			result = fmt_new_datetime(*(__int64 *)&tok->bytes[tok->pos]);
#endif
			tok->pos += sizeof(__int64);
		}
		break;
	case PDT_STRING:
		result = fmt_read_string(tok);
		break;
	case PDT_ARRAY:
		result = fmt_parse_array(tok);
		break;
	case PDT_OBJECT:
		result = fmt_parse_object(tok);
		break;
	}

	return result;
}


int fmt_parser_push(FMTParserState* st, const void* buf, unsigned int size)
{
	unsigned int p = 0; /* 已用长度 */
	unsigned char * buffer = (unsigned char*) buf;

	while(p < size)
	{
		struct _pdt_fmt_tok tok;
		switch(st->t)
		{
		case __PST_HEADER:
			if(buffer[p++] == 0x03)
			{
				st->t = __PST_VERSION; //解析版本
			}
			break;
		case __PST_VERSION:
			if(size >= p+sizeof(short))
			{	//数据足够  读取版本号
				st->m_ver = *(unsigned short *)&buffer[p];
				p+= sizeof(unsigned short);
			}else{
				return p;
			}
			switch(st->m_ver)
			{
			case FMT_PROTOCOL_VERSION_V1:
				st->t = __PST_COMMAND;
				break;
#if defined(FMT_HAS_V2)
			case FMT_PROTOCOL_VERSION_V2: //加密传输协议.
				st->t = __PST_ENCRYPTED_DATA_START;
				break;
#endif
			default:
				st->m_cmd = PT_ERROR_DATA;
				st->t = __PST_FINISHED;
			}
			break;
		case __PST_COMMAND:
			if(size >= p+sizeof(short))
			{	//数据足够  读取命令代码
#ifdef  LITTLEENDIANNESS
				st->m_cmd = SWAP_16(*(unsigned short *)&buffer[p]);
#else
				st->m_cmd = *(unsigned short *)&buffer[p];
#endif
				p+= sizeof(unsigned short);
				st->t = __PST_DATASTART;

			}
			else
			{
				return p;
			}
			break;
#if defined(FMT_HAS_V2)
		case __PST_ENCRYPTED_DATA_START: //加密数据.
			{
				int used =0 , dbLen =0;
				if(0 <= (dbLen = fmt_read_length(&buffer[p],size - p,&used)))
				{
					p+=used;
					st->m_dLen = dbLen;
					if(dbLen > 0)
					{
						st->t = __PST_ENCRYPTED_DATA;
					}else{
						//没有数据体 直接返回成功..
						if(NULL != st->result)
						{
							fmt_object_put(st->result);
							st->result = NULL;
						}
						st->t = __PST_FINISHED;
						return p;
					}
				}
			}
			break;
#endif
		case __PST_DATASTART:
			//读取数据包长度
			{
				int used =0 , dbLen =0;
				if(0 <= (dbLen = fmt_read_length(&buffer[p],size - p,&used)))
				{
					p += used;
					st->m_dLen = dbLen;
					if(dbLen > 0)
					{
						st->t = __PST_DATA;
					}
					else
					{
						//没有数据体 直接返回成功..
						if(NULL != st->result)
						{
							fmt_object_put(st->result);
							st->result = NULL;
						}
						st->t = __PST_FINISHED;
						return p;
					}
				}
				else
				{
					return p;
				}
			}
			break;
#if defined(FMT_HAS_V2)
		case __PST_ENCRYPTED_DATA:
			if((size - p) < st->m_dLen)  //数据不够,等待数据满.
				return p;
			if(NULL == st->key){
				st->result = NULL;
				st->m_cmd = PT_ERROR_DATA;
				st->t = __PST_FINISHED;
				return p;
			}
			//
			{
				unsigned int rawlen = st->m_dLen;
				unsigned char * rawptr = &buffer[p];
				// 在这里加入解密接口.
				unsigned int dplen = 0; // 解密后数据长度
				unsigned char * dptr  = xxtea_decrypt(rawptr,rawlen, (unsigned char *)st->key, &dplen), * pbuff = dptr;
				// 解密后数据存储于 dptr

				if(NULL == dptr)
				{
					// 解密数据失败.
					st->result = NULL;
					st->m_cmd = PT_ERROR_DATA;
					p += rawlen;
					st->t = __PST_FINISHED;
					return p;
				}
#ifdef  LITTLEENDIANNESS
				st->m_cmd = SWAP_16(*(unsigned short *)&pbuff[0]);
#else
				st->m_cmd = *(unsigned short *)&pbuff[0];
#endif
				pbuff += sizeof(unsigned short);

				if( (dptr + dplen) > pbuff)
				{
					int used = 0,
						rolen = fmt_read_length(pbuff,(dptr + dplen) - pbuff, &used);
					if(rolen >0 && used >0)
					{
						FMT * result ;
						pbuff+=used;
						// 存在FMT对象
						result = fmt_quick_parse((char *)pbuff, (dptr + dplen) - pbuff);
						//原来的 st->result 只是一个壳。 这里解析出来的菜是最终结果.

						st->result = result?fmt_object_get(result):NULL;
					}
				}
				st->t = __PST_FINISHED;
				p += rawlen;
				return p;
			}			
			break;
#endif
		case __PST_DATA:

			if((size - p) < st->m_dLen)  //数据不够,等待数据满.
				return p;

			// 解析数据流生成对象.
			tok.bytes = &buffer[p];
			tok.pos = 0;
			tok.size = size - p;
			st->result = fmt_token_parse(&tok);
			st->t = __PST_FINISHED;
			if(NULL != st->result)
			{
				//数据包解析成功
				fmt_object_get(st->result);
				p += tok.pos;
				return p;
			}
			
			break;
		case __PST_FINISHED:
			//丢弃数据引用， 并接受下一个包.

			if(NULL != st->result)
			{
				fmt_object_put(st->result);
				st->result = NULL;
			}

			st->t = __PST_HEADER;
			break;
		default:
			break;
		}
	}

	return p;

}


FMT * fmt_quick_parse(char * data,int length)
{
	struct _pdt_fmt_tok tok;
	tok.bytes = (unsigned char *)data;
	tok.pos = 0;
	tok.size = length;
	return fmt_token_parse(&tok);
}
int fmt_quick_serial(automem_t * pmem,FMT * fmt)
{
	return fmt->_serialize(fmt,pmem);
}


/* 当前包是否解析完成? */

int fmt_parser_complete(FMTParserState * st)
{
	return __PST_FINISHED == st->t;
}



FMT * fmt_parser_take(FMTParserState * st)
{
	if(st->t == __PST_FINISHED && NULL != st->result)
		return fmt_object_get(st->result);
	return NULL;
}




void fmt_parser_reset(FMTParserState * st)
{
	if(st->t == __PST_FINISHED && NULL != st->result)
	{
		fmt_object_put(st->result);
	}
	st->t=__PST_HEADER;
	st->result = NULL;

}


static unsigned int crc32_tab[] =
{
	0x00000000, 0x77073096, 0xee0e612c, 0x990951ba, 0x076dc419, 0x706af48f,
	0xe963a535, 0x9e6495a3,	0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988,
	0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91, 0x1db71064, 0x6ab020f2,
	0xf3b97148, 0x84be41de,	0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
	0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec,	0x14015c4f, 0x63066cd9,
	0xfa0f3d63, 0x8d080df5,	0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172,
	0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b,	0x35b5a8fa, 0x42b2986c,
	0xdbbbc9d6, 0xacbcf940,	0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
	0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116, 0x21b4f4b5, 0x56b3c423,
	0xcfba9599, 0xb8bda50f, 0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924,
	0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d,	0x76dc4190, 0x01db7106,
	0x98d220bc, 0xefd5102a, 0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433,
	0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818, 0x7f6a0dbb, 0x086d3d2d,
	0x91646c97, 0xe6635c01, 0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e,
	0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457, 0x65b0d9c6, 0x12b7e950,
	0x8bbeb8ea, 0xfcb9887c, 0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65,
	0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2, 0x4adfa541, 0x3dd895d7,
	0xa4d1c46d, 0xd3d6f4fb, 0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0,
	0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9, 0x5005713c, 0x270241aa,
	0xbe0b1010, 0xc90c2086, 0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
	0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4, 0x59b33d17, 0x2eb40d81,
	0xb7bd5c3b, 0xc0ba6cad, 0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a,
	0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683, 0xe3630b12, 0x94643b84,
	0x0d6d6a3e, 0x7a6a5aa8, 0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1,
	0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe, 0xf762575d, 0x806567cb,
	0x196c3671, 0x6e6b06e7, 0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc,
	0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5, 0xd6d6a3e8, 0xa1d1937e,
	0x38d8c2c4, 0x4fdff252, 0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b,
	0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60, 0xdf60efc3, 0xa867df55,
	0x316e8eef, 0x4669be79, 0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236,
	0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f, 0xc5ba3bbe, 0xb2bd0b28,
	0x2bb45a92, 0x5cb36a04, 0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d,
	0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a, 0x9c0906a9, 0xeb0e363f,
	0x72076785, 0x05005713, 0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38,
	0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21, 0x86d3d2d4, 0xf1d4e242,
	0x68ddb3f8, 0x1fda836e, 0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777,
	0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c, 0x8f659eff, 0xf862ae69,
	0x616bffd3, 0x166ccf45, 0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2,
	0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db, 0xaed16a4a, 0xd9d65adc,
	0x40df0b66, 0x37d83bf0, 0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
	0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6, 0xbad03605, 0xcdd70693,
	0x54de5729, 0x23d967bf, 0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94,
	0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d
};

unsigned int crc32(unsigned int  crc, const void * buf, size_t size)
{
	const unsigned char *p = (const unsigned char *)buf;
	crc = crc ^ ~0U;

	while (size--)
		crc = crc32_tab[(crc ^ *p++) & 0xFF] ^ (crc >> 8);

	return crc ^ ~0U;
}

const char * byte2hex(unsigned char b)
{
	static const char * hex_table[] = {
		"00","01","02","03","04","05","06","07","08","09","0A","0B","0C","0D","0E","0F",
		"10","11","12","13","14","15","16","17","18","19","1A","1B","1C","1D","1E","1F",
		"20","21","22","23","24","25","26","27","28","29","2A","2B","2C","2D","2E","2F",
		"30","31","32","33","34","35","36","37","38","39","3A","3B","3C","3D","3E","3F",
		"40","41","42","43","44","45","46","47","48","49","4A","4B","4C","4D","4E","4F",
		"50","51","52","53","54","55","56","57","58","59","5A","5B","5C","5D","5E","5F",
		"60","61","62","63","64","65","66","67","68","69","6A","6B","6C","6D","6E","6F",
		"70","71","72","73","74","75","76","77","78","79","7A","7B","7C","7D","7E","7F",
		"80","81","82","83","84","85","86","87","88","89","8A","8B","8C","8D","8E","8F",
		"90","91","92","93","94","95","96","97","98","99","9A","9B","9C","9D","9E","9F",
		"A0","A1","A2","A3","A4","A5","A6","A7","A8","A9","AA","AB","AC","AD","AE","AF",
		"B0","B1","B2","B3","B4","B5","B6","B7","B8","B9","BA","BB","BC","BD","BE","BF",
		"C0","C1","C2","C3","C4","C5","C6","C7","C8","C9","CA","CB","CC","CD","CE","CF",
		"D0","D1","D2","D3","D4","D5","D6","D7","D8","D9","DA","DB","DC","DD","DE","DF",
		"E0","E1","E2","E3","E4","E5","E6","E7","E8","E9","EA","EB","EC","ED","EE","EF",
		"F0","F1","F2","F3","F4","F5","F6","F7","F8","F9","FA","FB","FC","FD","FE","FF",
	};
	return hex_table[b];
}

unsigned char hex2byte(const unsigned char *ptr)
{
    int i;
    unsigned char result = 0;

    for (i = 0; i < 2; i++) {
        result <<= 4;
        result |= (ptr[i] >= '0' && ptr[i] <= '9') ? (ptr[i] - '0') :
            (((ptr[i] & 0xDF) >= 'A' && (ptr[i] & 0xDF) <= 'F') ? (ptr[i] - 'A' + 0x0A) :
            0x00);
    }

    return result;
}
