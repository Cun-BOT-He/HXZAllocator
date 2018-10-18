#pragma once
#ifndef _HXZalloc_
#define _HXZalloc_

#include<cstdlib>

namespace HXZ {

	//���д����������Ŀռ���������sub-allocation��
	
	class alloc {

	private:
		enum{ __ALIGN = 8 };//С��������ϵ��߽�
		enum{ __MAX_BYTES = 128 };//С��������Ͻ磬�������ڴ泬��128�ֽڣ���ֱ�ӵ���malloc,free��realloc
		enum{ __NFREELISTS = __MAX_BYTES/__ALIGN};//freelists����
		enum{ NOBJS = 20 };//ÿ�����ӵĽڵ��������Ϊ�̶�Ϊ20��

	private:
		static size_t ROUND_UP(size_t bytes) {  //��bytes�ϵ���8�ı���
			if (bytes % 8 != 0)
			{
				bytes = bytes + (8 - (bytes % 8));
			}
			return bytes;
		}

	private:
		union  obj  //freelists�Ľڵ�
		{
			union obj* free_list_link;
			char client_data[1];
		};
		//16��freelists
		static obj* free_list[__NFREELISTS];

	private:
		//�Խ�mempool
		static char *start_free;
		static char *end_free;
		static size_t heap_size;

	private://���ú���

		//���������С������ʹ�õ�n��free_list n��1����
		static size_t FREELIST_INDEX(size_t bytes) {
			return (((bytes)+__ALIGN-1) / __ALIGN - 1);
		}
		//����һ����СΪn�Ķ��󣬲����ܼ����СΪn�����鵽free_list
		static void *refill(size_t n);

		//����һ���ռ䣬�����ܼ����СΪn���������鵽free_list
		//nobjs���ܻή�ͣ�������������ʱ
		static char *chunk_alloc(size_t size, size_t& nobjs);

	public:
		static void *allocate(size_t bytes);
		static void deallocate(void *ptr, size_t bytes);
		static void *reallocate(void *ptr, size_t old_sz, size_t new_sz);

	public:
		alloc() {};
		~alloc() {};

	};

	//�������ֵ�趨
	char* alloc::start_free = 0;
	char* alloc::end_free = 0;
	size_t alloc::heap_size = 0;

	alloc::obj *alloc::free_list[alloc::__NFREELISTS] =
	{ 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 };

	void *alloc::allocate(size_t bytes) {

		//�������ڴ����128�ֽ�ֱ�ӵ���һ��������
		if (bytes > __MAX_BYTES)
		{
			void *p;
			p = malloc(bytes);
			return p;
		}
		else
		{
			size_t index;
			//Ѱ��free_list�к��ʵ�һ������ռ�ȥȡ�ڴ�
			index = FREELIST_INDEX(bytes);
			obj *my_free_list = free_list[index];
			//�����Ӧ��free_listû���ڴ棬����ڴ����ȥȡ
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

				
		//��������ͷŵ��ڴ����128�ֽڣ���ֱ�ӵ���free
		if (bytes > __MAX_BYTES)
		{
			free(ptr);
			return;

		}
		//���С��128�ֽڣ��������free_list��
		//size_t index;
		
		obj** my_free_list;
		my_free_list = free_list + FREELIST_INDEX(bytes);
		obj *mynode = static_cast<obj*>(ptr);
		mynode->free_list_link = *my_free_list;
		*my_free_list = mynode;
		
	}


	void *alloc::reallocate(void *ptr, size_t old_sz, size_t new_sz) {

		//����Ҫreallocate���ֽڴ���128�ֽ������һ���ڴ�������
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
		//���ڴ����ȡ��nobjs��n bytes����������
		char* chunk = chunk_alloc(n, nobjs);
		obj **my_lists;                   //ָ��free_list��ָ��
		obj *result;
		obj *current_obj, *next_obj;
		int i;
		//���ֻ���1�����飬���������ø��û�ʹ�ã�free_listû���½ڵ�
		if (1 == nobjs) return(chunk);

		//ѡ����ʵ�free_list�������½ڵ�
		my_lists = free_list + FREELIST_INDEX(n);
		//��һ���ڴ�ֱ�ӷ��ظ��Ͷ�ʹ��
		result = (obj*)chunk;
		//ʣ�µ��ڴ����β�����������뵽��Ӧ��free_list��ȥ
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


	//���ڴ����ȡ��nobjs����С���ڴ�鷵�ظ�free_list,�ڴ��Ѿ��ϵ���8�ı���
	char* alloc::chunk_alloc(size_t size, size_t& nobjs) {

		char* result;
		size_t totalsize = size * nobjs;
		size_t bytesleft = end_free - start_free;

		if (bytesleft >= totalsize) {
			//����ڴ��ʣ�µĿռ��������Ҫ���ڴ�ռ�
			result = start_free;
			start_free += totalsize;
			return result;
		}
		else if (bytesleft >= size)
		{
			//����ڴ��ʣ�µĿռ����ٿ��Թ�һ��(������)��size
			nobjs = bytesleft / size;
			totalsize = nobjs * size;
			result = start_free;
			start_free += totalsize;
			return result;

		}
		else {    //�ڴ��ʣ��ռ���һ���ڴ涼�޷��ṩ

			size_t bytes2get = 2 * totalsize + ROUND_UP(heap_size >> 4);
			//���ڴ���ڵ�ʣ���ڴ��ȷ�������ʵ�free_list
			if (bytesleft > 0)
			{
				obj **my_free_list;
				my_free_list = free_list + FREELIST_INDEX(bytesleft);
				((obj*)start_free)->free_list_link = *my_free_list;
				*my_free_list = ((obj*)start_free);
			}

			start_free = (char*)malloc(bytes2get);
			if (0 == start_free) {
				//heap�ռ䲻�㣬mallocʧ��
				int i;
				obj** my_free_list;
				obj* temp;
				//9������Ѱһ��freelist��"����δ�õ��㹻����ڴ��"
				for (i = size; i < __MAX_BYTES; i += __ALIGN)
				{
					my_free_list = free_list + FREELIST_INDEX(i);
					temp = *my_free_list;
					if (0 != temp) {
						*my_free_list = temp->free_list_link;
						start_free = (char*)temp;
						end_free = start_free + i;
						//�ݹ���ã�����nobjs
						return chunk_alloc(size, nobjs);
					}
				}
				//��ȫû�ڴ��ˣ�������
				end_free = 0;
			}
			heap_size += bytes2get;
			end_free = start_free + bytes2get;
			//�ݹ���ã�����nobjs
			return chunk_alloc(size, nobjs);

		}


	}

   

}


#endif // !_HXZalloc_
