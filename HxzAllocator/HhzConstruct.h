#pragma once
#ifndef _HXZConstruct_
#define _HXZConstruct_

#include<new>

namespace HXZ {

	//ֻʵ���˷����汾��û�п��ǽ��ܵ����������
	template<class T1, class T2>
	inline void construct(T1* p, const T2& value) {
		new(p)T1(value);
	}
	//������������,��һ���汾������һ��ָ��
	template<class T>
	inline void destroy(T* p) {
		p->~T();
	}
    
}



#endif // !_HXZConstruct_

