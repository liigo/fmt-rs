#pragma once

/*
	һ��ͨ�Ű����� ��ͷ(1�ֽ�)0x03 �汾(2�ֽ�) ��������(2�ֽ�) �û�ID(4�ֽ�)
*/

#ifdef __cplusplus
	extern "C"	{
#endif

#include "lib/arraylist.h"
#include "lib/linkhash.h"
#include "lib/automem.h"

#include <stddef.h>
//#include <stdint.h>

#if defined(_M_IX86) || defined(__i386__) || defined(_M_X64) || defined(__x86_64__) || defined(__MIPSEL__) || defined(__ARMEL__)
	//С��ƽ̨ �ֱ��� X86 X64 MIPS ARM
	#define FMT_PROTOCOL_VERSION_V1		0x01
#if defined(FMT_HAS_V2)
	#define FMT_PROTOCOL_VERSION_V2		0x02
#endif
	#define LITTLEENDIANNESS
#elif defined(__MIPSEB__) || defined(__ARMEB__)
	//���ƽ̨ �ֱ��� MIPS ARM
	#define FMT_PROTOCOL_VERSION_V1		0x0100
#if defined(FMT_HAS_V2)
	#define FMT_PROTOCOL_VERSION_V2		0x0200
#endif
	#define BIGENDIANNESS
#endif

#if defined(_MSC_VER)
	// VC������
	typedef unsigned __int64	__UInt64;
#else
	// GCC������
	typedef long long __int64;
	typedef unsigned long long		__UInt64;
#endif

// ƽ̨����
#if defined(_M_IX86) || defined(__i386__)
	#define PLATFORM_TYPE		1
#elif defined(_M_X64) || defined(__x86_64__)
	#define PLATFORM_TYPE		2
#elif defined(__MIPSEL__)
	#define PLATFORM_TYPE		3
#elif __ARMEL__
	#define PLATFORM_TYPE		4
#elif __MIPSEB__
	#define PLATFORM_TYPE		5
#elif __ARMEB__
	#define PLATFORM_TYPE		6
#endif

enum{
	PDT_BYTE     = 0x01,
	PDT_SHORT    = 0x02,
	PDT_USHORT   = 0x03,
	PDT_INTEGER  = 0x04,
	PDT_UINTEGER = 0x05,
	PDT_LONG     = 0x06,
	PDT_ULONG    = 0x07,
	PDT_DOUBLE   = 0x08,
	PDT_STRING   = 0x09,

	//������������
	PDT_DATETIME = 0x0A, //����ʱ��
	PDT_ARRAY    = 0x0B,	 //����
	PDT_OBJECT   = 0x0C,	 //����
	PDT_NULL	 = 0x0D,	//��
	PDT_BOOL	 = 0x0E,	//�߼�����

	_PDT_END = 0xFF, //��β��ʶ��
};


#define fmt_isarray(a)		\
	( a->t == PDT_ARRAY)

#define fmt_isobject(a)		\
	( a->t == PDT_OBJECT)

#define fmt_isstring(a)		\
	( a->t == PDT_STRING)



/* ͨ��Э������� */
enum
{
	// �ͻ����ύ������������.���������� PONG
	PT_ERROR_DATA = 0x00,
	PT_PING = 0x01,		//PING �ͻ��˷��͵�������
	PT_PONG = 0x02,		//PING ��������Ӧ��������
	PT_TOUCH_KEY = 0x03,	//������Կָ��. ����Ϊ FMT String.

	//
	PT_SUBMIT_AGENTINFO = 0x04,	//ҵ��������ϱ���ɫ����.
};

typedef struct FMT_s FMT;
typedef void (fmt_object_delete_fn)(FMT *o);
typedef int (fmt_object_serialize_fn)(FMT * fmt,automem_t * pmem);

struct FMT_s
{
	unsigned char t; //��������  
	int _ref;		//���ü���  
	union{
		unsigned char m_byte;
		short m_short;
		unsigned short m_ushort;
		int m_int;
		unsigned int m_uint;
		__int64 m_long;
		__UInt64 m_ulong;
		double m_double;
		__int64 m_date;
		struct array_list* m_array;
		struct lh_table * m_object;
		struct {
			unsigned int m_slen;
			unsigned char * m_str;
		};
	};
	fmt_object_delete_fn *_delete;
	fmt_object_serialize_fn * _serialize;

};


struct FMTParserState_s
{
	FMT * result;		//�������
	int is_Server;		//�Ƿ�Ϊ���������ݰ�
	int t;				//����״̬ ������ɨ��ʱʹ��.
	//
	unsigned short m_ver;	//���ݰ��汾
	unsigned int m_cmd;		//	������
	unsigned int m_dLen;	//���ݳ���
	char * key;
};
typedef struct FMTParserState_s FMTParserState;

/*

*/
int fmt_parser_push(FMTParserState* st, const void* buffer, unsigned int size);

//���ü�������
FMT * fmt_object_get(FMT* fmt);
void fmt_object_put(FMT* fmt);

//�����ı�����
FMT * fmt_new_string(const void* str, unsigned int len);
//�����ֽ�����
FMT * fmt_new_byte(unsigned char bt);
//��������������
FMT * fmt_new_short(short val);
FMT * fmt_new_ushort(unsigned short val);
//������������
FMT * fmt_new_integer(int val);
FMT * fmt_new_uinteger(unsigned int val);
//��������������
FMT * fmt_new_long(__int64 val);
FMT * fmt_new_ulong(__UInt64 val);
//������������
FMT * fmt_new_double(double val);

//��������ʱ������ ����Unixʱ���
FMT * fmt_new_datetime(__int64 val);

FMT * fmt_new_boolean(int b);
FMT * fmt_new_null(void);
//������������
FMT * fmt_new_array();
unsigned int fmt_array_length(FMT * fmt);
void fmt_array_append(FMT * fmt,FMT * item);
void fmt_array_remove(FMT * fmt,unsigned int idx);
FMT * fmt_array_get_idx(FMT * fmt,unsigned int idx);



// ����������
FMT* fmt_new_object();
void fmt_object_add(FMT* fmt, const char* key, FMT* val);
void fmt_object_remove(FMT * fmt, const char * key);
FMT * fmt_object_lookup(FMT * fmt, const char * key);
int fmt_object_total(FMT * fmt);

//���������ڹ������ݰ�
int fmt_packet_build(automem_t * pmem, FMT * fmt, short cmd);

#if defined(FMT_HAS_V2)
	int fmt_packet_build_v2(automem_t * pmem, FMT * fmt, short cmd, char * key /* ����Ϊ 16 Byte�ļ�����Կ*/);
#endif

/// ��������
/// ������������
FMTParserState * fmt_parser_new_state();

/// ���ٰ�������
void fmt_parser_close_state(FMTParserState * st);
///�����յ�������ѹ��������������ذ��������ɹ�ɨ����ֽ�����
int fmt_parser_push(FMTParserState* st, const void* buffer, unsigned int size);

///�ж��Ƿ������һ�����������ݰ�
int fmt_parser_complete(FMTParserState * st);

///����ɹ�������1�����ݰ���������ݰ��л�ȡ���ݶ���.
FMT * fmt_parser_take(FMTParserState * st);


/// ���ð�������״̬,׼��������һ�ν�����
void fmt_parser_reset(FMTParserState * st);

#if defined(FMT_HAS_V2)
// �������ݰ������� ��������.
int fmt_parser_set_key(FMTParserState * t, const char * key);
#endif

/// ���ٽ���һ�����ݶ���.
FMT * fmt_quick_parse(char * data,int length);
/// ���� ���л�һ�����ݶ���.
int fmt_quick_serial(automem_t * pmem,FMT * fmt);


//	�����ݽ���CRC У��
unsigned int crc32(unsigned int  crc, const void *buf, size_t size);

const char * byte2hex(unsigned char b);
unsigned char hex2byte(const unsigned char *ptr);
#ifdef __cplusplus
};
#endif

