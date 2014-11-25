#pragma once

/*
	一个通信包包含 包头(1字节)0x03 版本(2字节) 命令类型(2字节) 用户ID(4字节)
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
	//小端平台 分别是 X86 X64 MIPS ARM
	#define FMT_PROTOCOL_VERSION_V1		0x01
#if defined(FMT_HAS_V2)
	#define FMT_PROTOCOL_VERSION_V2		0x02
#endif
	#define LITTLEENDIANNESS
#elif defined(__MIPSEB__) || defined(__ARMEB__)
	//大端平台 分别是 MIPS ARM
	#define FMT_PROTOCOL_VERSION_V1		0x0100
#if defined(FMT_HAS_V2)
	#define FMT_PROTOCOL_VERSION_V2		0x0200
#endif
	#define BIGENDIANNESS
#endif

#if defined(_MSC_VER)
	// VC编译器
	typedef unsigned __int64	__UInt64;
#else
	// GCC编译器
	typedef long long __int64;
	typedef unsigned long long		__UInt64;
#endif

// 平台类型
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

	//复合数据类型
	PDT_DATETIME = 0x0A, //日期时间
	PDT_ARRAY    = 0x0B,	 //数组
	PDT_OBJECT   = 0x0C,	 //对象
	PDT_NULL	 = 0x0D,	//空
	PDT_BOOL	 = 0x0E,	//逻辑类型

	_PDT_END = 0xFF, //结尾标识符
};


#define fmt_isarray(a)		\
	( a->t == PDT_ARRAY)

#define fmt_isobject(a)		\
	( a->t == PDT_OBJECT)

#define fmt_isstring(a)		\
	( a->t == PDT_STRING)



/* 通信协议包类型 */
enum
{
	// 客户端提交过来的心跳包.服务器返回 PONG
	PT_ERROR_DATA = 0x00,
	PT_PING = 0x01,		//PING 客户端发送的心跳包
	PT_PONG = 0x02,		//PING 服务器回应的心跳包
	PT_TOUCH_KEY = 0x03,	//交换密钥指令. 内容为 FMT String.

	//
	PT_SUBMIT_AGENTINFO = 0x04,	//业务服务器上报角色数据.
};

typedef struct FMT_s FMT;
typedef void (fmt_object_delete_fn)(FMT *o);
typedef int (fmt_object_serialize_fn)(FMT * fmt,automem_t * pmem);

struct FMT_s
{
	unsigned char t; //数据类型  
	int _ref;		//引用计数  
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
	FMT * result;		//解析结果
	int is_Server;		//是否为服务器数据包
	int t;				//解析状态 仅仅在扫描时使用.
	//
	unsigned short m_ver;	//数据包版本
	unsigned int m_cmd;		//	包类型
	unsigned int m_dLen;	//数据长度
	char * key;
};
typedef struct FMTParserState_s FMTParserState;

/*

*/
int fmt_parser_push(FMTParserState* st, const void* buffer, unsigned int size);

//引用计数操作
FMT * fmt_object_get(FMT* fmt);
void fmt_object_put(FMT* fmt);

//创建文本类型
FMT * fmt_new_string(const void* str, unsigned int len);
//创建字节类型
FMT * fmt_new_byte(unsigned char bt);
//创建短整数类型
FMT * fmt_new_short(short val);
FMT * fmt_new_ushort(unsigned short val);
//创建整数类型
FMT * fmt_new_integer(int val);
FMT * fmt_new_uinteger(unsigned int val);
//创建长整数类型
FMT * fmt_new_long(__int64 val);
FMT * fmt_new_ulong(__UInt64 val);
//创建浮点类型
FMT * fmt_new_double(double val);

//创建日期时间类型 传递Unix时间戳
FMT * fmt_new_datetime(__int64 val);

FMT * fmt_new_boolean(int b);
FMT * fmt_new_null(void);
//创建数组类型
FMT * fmt_new_array();
unsigned int fmt_array_length(FMT * fmt);
void fmt_array_append(FMT * fmt,FMT * item);
void fmt_array_remove(FMT * fmt,unsigned int idx);
FMT * fmt_array_get_idx(FMT * fmt,unsigned int idx);



// 创建对象型
FMT* fmt_new_object();
void fmt_object_add(FMT* fmt, const char* key, FMT* val);
void fmt_object_remove(FMT * fmt, const char * key);
FMT * fmt_object_lookup(FMT * fmt, const char * key);
int fmt_object_total(FMT * fmt);

//本函数用于构建数据包
int fmt_packet_build(automem_t * pmem, FMT * fmt, short cmd);

#if defined(FMT_HAS_V2)
	int fmt_packet_build_v2(automem_t * pmem, FMT * fmt, short cmd, char * key /* 长度为 16 Byte的加密密钥*/);
#endif

/// 包解析器
/// 创建包解析器
FMTParserState * fmt_parser_new_state();

/// 销毁包解析器
void fmt_parser_close_state(FMTParserState * st);
///将接收到的数据压入包解析器。返回包解析器成功扫描的字节数。
int fmt_parser_push(FMTParserState* st, const void* buffer, unsigned int size);

///判断是否解析了一个完整的数据包
int fmt_parser_complete(FMTParserState * st);

///如果成功解析了1个数据包，则从数据包中获取数据对象.
FMT * fmt_parser_take(FMTParserState * st);


/// 重置包解析器状态,准备进行下一次解析。
void fmt_parser_reset(FMTParserState * st);

#if defined(FMT_HAS_V2)
// 设置数据包解析器 解密密码.
int fmt_parser_set_key(FMTParserState * t, const char * key);
#endif

/// 快速解析一个数据对象.
FMT * fmt_quick_parse(char * data,int length);
/// 快速 序列化一个数据对象.
int fmt_quick_serial(automem_t * pmem,FMT * fmt);


//	对数据进行CRC 校验
unsigned int crc32(unsigned int  crc, const void *buf, size_t size);

const char * byte2hex(unsigned char b);
unsigned char hex2byte(const unsigned char *ptr);
#ifdef __cplusplus
};
#endif

