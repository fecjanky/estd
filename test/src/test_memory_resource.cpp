#include <exception>
#include <tuple>

#include "memory.h"
#include "gtest/gtest.h"
#include "gmock/gmock.h"

#ifdef _MSC_VER
__declspec(dllexport)
#endif  // _MSC_VER
int PullInTestMemResourceLibrary() { return 0; }

namespace MemResourceTest {
    using namespace estd;
    TEST(memory_resource_t_test, default_constructed_mem_resource_ptr_is_zero) {
        memory_resource_t m{};
        EXPECT_EQ(nullptr, m.ptr());
    }


}  // namespace MemResourceTest 