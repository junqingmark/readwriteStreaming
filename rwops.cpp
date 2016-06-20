#include "rwops.h"
#include <stdlib.h>
#include <string.h>
#include <iostream>


static long stdio_size(RWops* context)
{
	long pos, size;
	pos = RWseek(context, 0, RW_SEEK_CUR);
	if(pos < 0)
	{
		return -1;
	}

	size = RWseek(context, 0, RW_SEEK_END);
	RWseek(context, 0, RW_SEEK_SET);

	return size;
}

static long stdio_seek(struct RWops* context, long offset, int whence)
{
	if(fseek(context->hidden.stdio.fp, offset, whence) == 0)
	{
		return ftell(context->hidden.stdio.fp);
	}
	else
		return -1;
}

static size_t stdio_read(RWops* context, void* ptr, size_t size, size_t maxnum )
{
	size_t nread;
	//size_t fread(void* buff,size_t size,size_t count,FILE* stream)
	//int ferror(FILE *stream);
	nread = fread(ptr, size, maxnum, context->hidden.stdio.fp);
	if(0 == nread || ferror(context->hidden.stdio.fp))
	{
		std::cerr << "fail in fread" << std::endl;
	}
	return nread;
}

static size_t stdio_write(RWops* context, void* ptr, size_t size, size_t maxnum)
{
	size_t nwrite;
	//size_t fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream);

	nwrite = fwrite(ptr, size, maxnum, context->hidden.stdio.fp);
	if(0 == nwrite || ferror(context->hidden.stdio.fp))
	{
		std::cerr << "fail in fwrite" << std::endl;
	}
	return nwrite;
}

static int stdio_close(RWops* context)
{
	int status = 0;
	if(context)
	{
		if(context->hidden.stdio.autoclose)
		{
			if(fclose(context->hidden.stdio.fp) != 0)
			{
				status = -1;
			}
		}

		FreeRW(context);
	}

	return status;
}

static long mem_size(RWops* context)
{
	return (long)(context->hidden.mem.stop-context->hidden.mem.base);
}

static long mem_seek(struct RWops* context, long offset, int whence)
{
	uint8_t* newpos;
	switch(whence)
	{
	case RW_SEEK_SET:
		newpos = context->hidden.mem.base + offset;
		break;
	case RW_SEEK_CUR:
		newpos = context->hidden.mem.here + offset;
		break;
	case RW_SEEK_END:
		newpos = context->hidden.mem.stop + offset;
		break;
	default:
		return -1;
	}

	if(newpos < context->hidden.mem.base)
	{
		newpos = context->hidden.mem.base;
	}
	if(newpos > context->hidden.mem.stop)
	{
		newpos = context->hidden.mem.stop;
	}

	context->hidden.mem.here = newpos;

	return (long)(context->hidden.mem.here - context->hidden.mem.base);
}

static size_t mem_read(struct RWops* context, void* ptr, size_t size, size_t maxnum)
{
	size_t total_bytes;
	size_t mem_available;

	total_bytes = size * maxnum;
	if(size <= 0 || maxnum <= 0)
	{
		return 0;
	}

	mem_available = (context->hidden.mem.stop - context->hidden.mem.here);
	if(total_bytes > mem_available)
	{
		total_bytes = mem_available;
	}

	memcpy(ptr, context->hidden.mem.here, total_bytes);
	context->hidden.mem.here += total_bytes;

	return total_bytes/size;
}

static size_t mem_write(struct RWops* context, void* ptr, size_t size, size_t maxnum)
{
	if((context->hidden.mem.here + maxnum*size > context->hidden.mem.stop))
	{
		maxnum = (context->hidden.mem.stop - context->hidden.mem.here)/size;
	}
	memcpy(context->hidden.mem.here, ptr, maxnum*size);
	context->hidden.mem.here += maxnum*size;

	return maxnum;

}

static size_t mem_writeconst(struct RWops* context, void* ptr, size_t size, size_t maxnum)
{
	std::cerr << "cannot write to read-only memory!" << std::endl;
	return 0;
}

static int mem_close(struct RWops* context)
{
	if(context)
	{
		free(context);
	}
	return 0;
}

RWops* RWFromFile(const char* file, const char* mode)
{
	RWops* rwops = NULL;
	if(!file || !*file || !mode || !*mode)
	{
		std::cerr << "No file or mode specified!" << std::endl;
		return NULL;
	}

	FILE* fp = fopen(file, mode);
	if(NULL == fp)
	{
		std::cerr << "cannot open " << file << std::endl;
	}
	else
	{
		rwops = RWFromFP(fp, 1);
	}

	return rwops;


}

RWops* RWFromFP(FILE* fp, bool autoclose)
{
	RWops* rwops = NULL;
	rwops = AllocRW();
	if(rwops != NULL)
	{
		rwops->read = stdio_read;
		rwops->write = stdio_write;
		rwops->size = stdio_size;
		rwops->seek = stdio_seek;
		rwops->close = stdio_close;
		rwops->hidden.stdio.fp = fp;
		rwops->hidden.stdio.autoclose = autoclose;
		rwops->type = RWOPS_STDFILE;
	}

	return rwops;
}

RWops* RWFromMem(void* mem, int size)
{
	RWops* rwops = NULL;
	if(NULL == mem)
	{
		std::cerr << "wrong parameter mem!" << std::endl;
		return rwops;
	}

	if(0 == size)
	{
		std::cerr << "wrong parameter size!" << std::endl;
		return rwops;
	}
	rwops = AllocRW();
	if(rwops != NULL)
	{
		rwops->size = mem_size;
		rwops->seek = mem_seek;
		rwops->read = mem_read;
		rwops->write = mem_write;
		rwops->close = mem_close;

		rwops->hidden.mem.base = (uint8_t*)mem;
		rwops->hidden.mem.here = rwops->hidden.mem.base;
		rwops->hidden.mem.stop = rwops->hidden.mem.base + size;
		rwops->type = RWOPS_MEMORY;
	}

	return rwops;

}

RWops* RWFromConstMem(const void* mem, int size)
{
	RWops* rwops = NULL;
	if(NULL == mem)
	{
		std::cerr << "wrong parameter mem!" << std::endl;
		return rwops;
	}

	if(0 == size)
	{
		std::cerr << "wrong parameter size!" << std::endl;
		return rwops;
	}
	rwops = AllocRW();
	if(rwops != NULL)
	{
		rwops->size = mem_size;
		rwops->seek = mem_seek;
		rwops->read = mem_read;
		rwops->write = mem_writeconst;
		rwops->close = mem_close;

		rwops->hidden.mem.base = (uint8_t*)mem;
		rwops->hidden.mem.here = rwops->hidden.mem.base;
		rwops->hidden.mem.stop = rwops->hidden.mem.base + size;
		rwops->type = RWOPS_MEMORY_RO;
	}

	return rwops;

}

RWops* AllocRW()
{
	RWops* area;
	area = (RWops*)malloc(sizeof(RWops));

	if(NULL == area)
	{
		std::cerr << "fail in malloc" << std::endl;
	}
	else
	{
		area->type = RWOPS_UNKNOWN;
	}

	return area;
}

void FreeRW(RWops* ops)
{
	free(ops);
}





