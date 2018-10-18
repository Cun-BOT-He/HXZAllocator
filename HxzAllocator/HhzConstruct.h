#pragma once
#ifndef _HXZConstruct_
#define _HXZConstruct_

#include<new>

namespace HXZ {

	//只实现了泛化版本，没有考虑接受迭代器的情况
	template<class T1, class T2>
	inline void construct(T1* p, const T2& value) {
		new(p)T1(value);
	}
	//调用析构函数,第一个版本，接受一个指针
	template<class T>
	inline void destroy(T* p) {
		p->~T();
	}
    
}



#endif // !_HXZConstruct_

