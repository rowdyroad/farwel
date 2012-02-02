#include "object.h"
namespace FWL {
    Object::Object()
        : count_(1)
    {}
    void Object::AddRef()
    {
        ++count_;
    }

    void Object::Release()
    {
        if (--count_ <= 0) {
            delete this;
        }
    }
}

namespace boost {
    void intrusive_ptr_add_ref(FWL::Object *obj)
    {
        obj->AddRef();
    }

    void intrusive_ptr_release(FWL::Object *obj)
    {
        obj->Release();
    }
};
