#pragma once

#include <boost/detail/atomic_count.hpp>

class Object
{
    private:
	boost::detail::atomic_count count_;
    protected:
	Object()
	    : count_(0)
	{}
    public:
	void AddRef()
	{
	    ++count_;
	}
	
	void Release()
	{
	    if (--count_ <= 0) {
		delete this;
	    }
	}
};

namespace boost {
    void intrusive_ptr_add_ref(::Object* obj)
    {
	obj->AddRef();
    }
    
    void intrusive_ptr_release(::Object* obj)
    {
	obj->Release();
    }
};

