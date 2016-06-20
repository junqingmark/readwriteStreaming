#ifndef _RW_OPS_H_
#define _RW_OPS_H_

#include <stdio.h>
#include <stdint.h>

#ifdef _cplusplus
extern "C" {
#endif

#define RWOPS_UNKNOWN 0   //unknown stream type
#define RWOPS_STDFILE 1
#define RWOPS_MEMORY 2
#define RWOPS_MEMORY_RO 3

#define RW_SEEK_SET 0
#define RW_SEEK_CUR 1
#define RW_SEEK_END 2


typedef struct RWops
{
	long (*size)(struct RWops* context);
	long (*seek)(struct RWops* context, long offset, int whence);
	size_t (*read)(struct RWops* context, void* ptr, size_t size, size_t maxnum);
	size_t (*write)(struct RWops* context, void* ptr, size_t size, size_t maxnum);
	int (*close)(struct RWops* context);

	size_t type;

	union
	{
		struct
		{
			bool autoclose;
			FILE* fp;
		}stdio;

		struct
		{
			uint8_t* base;
			uint8_t* here;
			uint8_t* stop;
		}mem;

		struct
		{
			void* data1;
			void* data2;
		}unknown;
	}hidden;

}RWops;

extern RWops* RWFromFile(const char* file, const char* mode);
extern RWops* RWFromFP(FILE* fp, bool autoclose);

extern RWops* RWFromMem(void* mem, int size);
extern RWops* RWFromConstMem(const void* mem, int size);

extern RWops* AllocRW();
extern void FreeRW(RWops* ops);

#define RWsize(ctx)         (ctx)->size(ctx)
#define RWseek(ctx, offset, whence) (ctx)->seek(ctx, offset, whence)
#define RWtell(ctx)         (ctx)->seek(ctx, 0, RW_SEEK_CUR)
#define RWread(ctx, ptr, size, n)   (ctx)->read(ctx, ptr, size, n)
#define RWwrite(ctx, ptr, size, n)  (ctx)->write(ctx, ptr, size, n)
#define RWclose(ctx)        (ctx)->close(ctx)

#ifdef _cplusplus
}
#endif

#endif
