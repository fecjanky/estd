#ifndef TEST_POLY_OBJ_STORAGE_H_
#define TEST_POLY_OBJ_STORAGE_H_

#include <atomic>

#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "memory.h"

namespace PolyStorageTest {
    struct IF {
        enum ret_code_t {
            from_impl1,
            from_impl2,
        };
        static constexpr int moved_from_indicator = -1;
        virtual ret_code_t func() = 0;
        virtual IF* clone(void*)const = 0;
        virtual IF* move(void*)noexcept = 0;
        virtual ~IF() = default;
        IF() : my_index{ index++ } {}
        
        int get_index() const noexcept {
            return my_index;
        }
    protected :
        int my_index;
    private:
        static std::atomic<int> index;
    };
    
    bool operator== (const IF& lhs, const IF& rhs) noexcept
    {
        return lhs.get_index() == rhs.get_index();
    }

    bool operator!= (const IF& lhs, const IF& rhs) noexcept
    {
        return !(lhs == rhs);
    }


    struct Impl1 : public IF {
        Impl1() = default;
        Impl1(const Impl1& rhs) {
            my_index = rhs.my_index;
        }

        Impl1(Impl1&& rhs) {
            my_index = rhs.my_index;
            rhs.my_index = moved_from_indicator;
        }

        virtual ret_code_t func() override {
            return from_impl1;
        }
        virtual IF* clone(void*d)const override {
            return new (d) Impl1(*this);
        }
        virtual IF* move(void*d) noexcept override {
            return new (d) Impl1(std::move(*this));
        }
    };

    struct Impl2 : public IF {
        Impl2() = default;
        Impl2(const Impl2& rhs) {
            my_index = rhs.my_index;
        }

        Impl2(Impl2&& rhs) {
            my_index = rhs.my_index;
            rhs.my_index = moved_from_indicator;
        }

        virtual ret_code_t func() override {
            return from_impl2;
        }
        virtual IF* clone(void*d)const override {
            return new (d) Impl2(*this);
        }
        virtual IF* move(void*d) noexcept override {
            return new (d) Impl2(std::move(*this));
        }

        double t[16];
    };

    class PolyStorageBasicTest : public ::testing::Test {
    public:
        PolyStorageBasicTest() : s1(Impl1{}), s2(Impl2{}) {}
    protected:
        estd::polymorphic_obj_storage_t<IF> s1,s2;
    };

}  // namespace PolyStorageTest

#endif  // TEST_POLY_OBJ_STORAGE_H_
