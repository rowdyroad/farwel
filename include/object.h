#pragma once

#include <boost/detail/atomic_count.hpp>
namespace FWL {
    class Object
    {
        private:
            boost::detail::atomic_count count_;
        protected:
            Object();
        public:
            void AddRef();
            void Release();
            virtual ~Object() {}
    };
}

namespace boost {
    void intrusive_ptr_add_ref(FWL::Object *obj);
    void intrusive_ptr_release(FWL::Object *obj);
};
