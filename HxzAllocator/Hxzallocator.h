#pragma once
#ifndef _HXZALLOC_
#define _HXZALLOC_

#include"HhzConstruct.h"
#include"HXZalloc.h"
#include<new>
#include<cstddef>
#include<cstdlib>
#include<climits>
#include<iostream>


namespace HXZ {

	
	template <class T>
	class allocator {
	public:
		//allocator��һЩ��Ҫ�ӿ�
		typedef T         value_type;
		typedef T*        pointer;
		typedef const T*  const_pointer;
		typedef T&        reference;
		typedef const T&  const_reference;
		typedef size_t    size_type;
		typedef ptrdiff_t difference_type;

	public:
		//�������������
		static void construct(T* ptr);
		static void destroy(T* ptr);

		//���ÿռ�
		static T* allocate();
		static T* allocate(size_t n);
		//�黹��ǰ�����ÿռ�
		static void deallocate(pointer p);
		static void deallocate(pointer p, size_t n);

	public:
		allocator() {};
		~allocator() {};


		//rebind allocator of type _Other
		template <class _Other>
		struct rebind
		{
			typedef allocator<_Other> other;
		};

		//����ĳ������ĵ�ַ
		pointer address(reference x) { return (pointer)&x; }

		//����ĳ��const����ĵ�ַ
		const_pointer const_address(const_reference x) {
			return (const_pointer)&x;
		}
		//���ؿɳɹ����õ������
		size_type max_size()const {
			return size_type(UINT_MAX / sizeof(T));
		}

};

	template<class T>
	void allocator<T>::construct(T* ptr) {
		HXZ::construct(ptr, T());
	}

	template<class T>
	void allocator<T>::destroy(T* ptr) {
		HXZ::destroy(ptr);
	}

	template<class T>
	T* allocator<T>::allocate() {
		return static_cast<T*>(alloc::allocate(sizeof(T)));
	}

	template<class T>
	T* allocator<T>::allocate(size_t n) {
		return static_cast<T*>(alloc::allocate(n*sizeof(T)));
	}

	template<class T>
	void allocator<T>::deallocate(pointer p) {
		alloc::deallocate(static_cast<void*>(p), sizeof(T));
	}

	template<class T>
	void allocator<T>::deallocate(pointer p,size_t n) {
		if (n == 0) return;
		alloc::deallocate(static_cast<void*>(p), n*sizeof(T));
	}
	

} //end of namespace HXZ


#endif // !_HXZALLOC_

