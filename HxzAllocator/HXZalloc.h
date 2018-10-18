#pragma once
#ifndef _HXZalloc_
#define _HXZalloc_

#include<cstdlib>

namespace HXZ {

	//具有次配置能力的空间配置器（sub-allocation）
	
	class alloc {

	private:
		enum{ __ALIGN = 8 };//小型区块的上调边界
		enum{ __MAX_BYTES = 128 };//小型区块的上界，若调用内存超过128字节，则直接调用malloc,free和realloc
		enum{ __NFREELISTS = __MAX_BYTES/__ALIGN};//freelists个数
		enum{ NOBJS = 20 };//每次增加的节点个数，人为固定为20个

	private:
		static size_t ROUND_UP(size_t bytes) {  //将bytes上调至8的倍数
			if (bytes % 8 != 0)
			{
				bytes = bytes + (8 - (bytes % 8));
			}
			return bytes;
		}

	private:
		union  obj  //freelists的节点
		{
			union obj* free_list_link;
			char client_data[1];
		};
		//16个freelists
		static obj* free_list[__NFREELISTS];

	private:
		//自建mempool
		static char *start_free;
		static char *end_free;
		static size_t heap_size;

	private://内用函数

		//根据区块大小，决定使用第n号free_list n从1算起
		static size_t FREELIST_INDEX(size_t bytes) {
			return (((bytes)+__ALIGN-1) / __ALIGN - 1);
		}
		//返回一个大小为n的对象，并可能加入大小为n的区块到free_list
		static void *refill(size_t n);

		//配置一大块空间，并可能加入大小为n的其他区块到free_list
		//nobjs可能会降低，当配置有困难时
		static char *chunk_alloc(size_t size, size_t& nobjs);

	public:
		static void *allocate(size_t bytes);
		static void deallocate(void *ptr, size_t bytes);
		static void *reallocate(void *ptr, size_t old_sz, size_t new_sz);

	public:
		alloc() {};
		~alloc() {};

	};

	//定义与初值设定
	char* alloc::start_free = 0;
	char* alloc::end_free = 0;
	size_t alloc::heap_size = 0;

	alloc::obj *alloc::free_list[alloc::__NFREELISTS] =
	{ 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 };

	void *alloc::allocate(size_t bytes) {

		//若所需内存大于128字节直接调用一级配置器
		if (bytes > __MAX_BYTES)
		{
			void *p;
			p = malloc(bytes);
			return p;
		}
		else
		{
			size_t index;
			//寻找free_list中合适的一个管理空间去取内存
			index = FREELIST_INDEX(bytes);
			obj *my_free_list = free_list[index];
			//如果对应的free_list没有内存，则从内存池中去取
			if (my_free_list==0)
			{
				void *r = refill(ROUND_UP(bytes));
				return r;

			}

			free_list[index] = my_free_list->free_list_link;
			return my_free_list;

		}
	}

	void alloc::deallocate(void *ptr, size_t bytes) {

				
		//如果申请释放的内存大于128字节，则直接调用free
		if (bytes > __MAX_BYTES)
		{
			free(ptr);
			return;

		}
		//如果小于128字节，则回收至free_list中
		//size_t index;
		
		obj** my_free_list;
		my_free_list = free_list + FREELIST_INDEX(bytes);
		obj *mynode = static_cast<obj*>(ptr);
		mynode->free_list_link = *my_free_list;
		*my_free_list = mynode;
		
	}


	void *alloc::reallocate(void *ptr, size_t old_sz, size_t new_sz) {

		//若需要reallocate的字节大于128字节则调用一级内存配置器
		if (old_sz > __MAX_BYTES) {
			void *p = realloc(ptr, new_sz);
			return p;
		}
		else
		{
			deallocate(ptr, old_sz);
			return allocate(new_sz);
		}
	}

	void* alloc::refill(size_t n) {

		size_t nobjs = 20;
		//从内存池中取出nobjs个n bytes容量的区块
		char* chunk = chunk_alloc(n, nobjs);
		obj **my_lists;                   //指向free_list的指针
		obj *result;
		obj *current_obj, *next_obj;
		int i;
		//如果只获得1个区块，则该区块调用给用户使用，free_list没有新节点
		if (1 == nobjs) return(chunk);

		//选择合适的free_list，纳入新节点
		my_lists = free_list + FREELIST_INDEX(n);
		//第一块内存直接返回给客端使用
		result = (obj*)chunk;
		//剩下的内存块首尾依次相连加入到对应的free_list中去
		*my_lists = next_obj = (obj*)(chunk + n);
		for (i = 1;i<nobjs; i++)
		{
			current_obj = next_obj;
			next_obj = (obj*)((char*)next_obj + n);
			if (nobjs-1 == i)
			{
				current_obj->free_list_link = 0;
			}
			else
			{
				current_obj->free_list_link = next_obj;
			}

		}
		return result;

	}


	//从内存池中取出nobjs个大小的内存块返回给free_list,内存已经上调至8的倍数
	char* alloc::chunk_alloc(size_t size, size_t& nobjs) {

		char* result;
		size_t totalsize = size * nobjs;
		size_t bytesleft = end_free - start_free;

		if (bytesleft >= totalsize) {
			//如果内存池剩下的空间大于所需要的内存空间
			result = start_free;
			start_free += totalsize;
			return result;
		}
		else if (bytesleft >= size)
		{
			//如果内存池剩下的空间至少可以供一个(或以上)的size
			nobjs = bytesleft / size;
			totalsize = nobjs * size;
			result = start_free;
			start_free += totalsize;
			return result;

		}
		else {    //内存池剩余空间连一块内存都无法提供

			size_t bytes2get = 2 * totalsize + ROUND_UP(heap_size >> 4);
			//将内存池内的剩余内存先分配给合适的free_list
			if (bytesleft > 0)
			{
				obj **my_free_list;
				my_free_list = free_list + FREELIST_INDEX(bytesleft);
				((obj*)start_free)->free_list_link = *my_free_list;
				*my_free_list = ((obj*)start_free);
			}

			start_free = (char*)malloc(bytes2get);
			if (0 == start_free) {
				//heap空间不足，malloc失败
				int i;
				obj** my_free_list;
				obj* temp;
				//9试着搜寻一下freelist中"尚有未用但足够大的内存块"
				for (i = size; i < __MAX_BYTES; i += __ALIGN)
				{
					my_free_list = free_list + FREELIST_INDEX(i);
					temp = *my_free_list;
					if (0 != temp) {
						*my_free_list = temp->free_list_link;
						start_free = (char*)temp;
						end_free = start_free + i;
						//递归调用，调整nobjs
						return chunk_alloc(size, nobjs);
					}
				}
				//完全没内存了，放弃吧
				end_free = 0;
			}
			heap_size += bytes2get;
			end_free = start_free + bytes2get;
			//递归调用，调整nobjs
			return chunk_alloc(size, nobjs);

		}


	}

   

}


#endif // !_HXZalloc_
