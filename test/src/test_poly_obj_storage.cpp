#include "test_poly_obj_storage.h"

#ifdef _MSC_VER
__declspec(dllexport)
#endif  // _MSC_VER
int PullInTestPolyObjStorageLibrary() { return 0; }


namespace PolyStorageTest {

std::atomic<int> IF::index { 0 };

TEST_F(PolyStorageBasicTest, ConstructionAndUse) {

    EXPECT_EQ(IF::from_impl1, s1->func());
    EXPECT_EQ(IF::from_impl2, s2->func());
    bool bs1 = s1;
    bool bs2 = s2;
    EXPECT_EQ(true, bs1);
    EXPECT_EQ(true, bs2);

}

TEST_F(PolyStorageBasicTest, DefaultConstruction) {
    estd::polymorphic_obj_storage_t<IF> t;
    EXPECT_EQ(nullptr, t.get());
    bool bt = t;
    EXPECT_FALSE(bt);
}

TEST_F(PolyStorageBasicTest, CopyConstruction) {
    estd::polymorphic_obj_storage_t<IF> t1(s1);
    estd::polymorphic_obj_storage_t<IF> t2(s2);
    EXPECT_EQ(true, t1 == s1);
    EXPECT_EQ(true, t2 == s2);
}

TEST_F(PolyStorageBasicTest, MoveConstruction) {
    auto i_s1 = s1->get_index();
    auto i_s2 = s2->get_index();
    estd::polymorphic_obj_storage_t<IF> t1(std::move(s1));
    estd::polymorphic_obj_storage_t<IF> t2(std::move(s2));
    EXPECT_EQ(IF::from_impl1, t1->func());
    EXPECT_EQ(IF::from_impl2, t2->func());
    EXPECT_EQ(i_s1,t1->get_index());
    EXPECT_EQ(i_s2, t2->get_index());
}

TEST_F(PolyStorageBasicTest, CopyAssignment) {
    estd::polymorphic_obj_storage_t<IF> t1(Impl2{});
    estd::polymorphic_obj_storage_t<IF> t2(Impl2{});
    t1 = s1;
    t2 = s2;
    EXPECT_EQ(true, t1 == s1);
    EXPECT_EQ(true, t2 == s2);
}


TEST_F(PolyStorageBasicTest, MoveAssignment) {
    auto i_s1 = s1->get_index();
    auto i_s2 = s2->get_index();
    estd::polymorphic_obj_storage_t<IF> t1(Impl2{});
    estd::polymorphic_obj_storage_t<IF> t2(Impl2{});
    t1 = std::move(s1);
    t2 = std::move(s2);
    EXPECT_EQ(IF::from_impl1, t1->func());
    EXPECT_EQ(IF::from_impl2, t2->func());
    EXPECT_EQ(i_s1, t1->get_index());
    EXPECT_EQ(i_s2, t2->get_index());
}

TEST_F(PolyStorageBasicTest, SwapMixed) {
    auto i_s1 = s1->get_index();
    auto i_s2 = s2->get_index();
    std::swap(s1, s2);
    EXPECT_EQ(IF::from_impl1, s2->func());
    EXPECT_EQ(IF::from_impl2, s1->func());
    EXPECT_EQ(i_s1,s2->get_index());
    EXPECT_EQ(i_s2,s1->get_index());
    std::swap(s1, s2);
    EXPECT_EQ(IF::from_impl1, s1->func());
    EXPECT_EQ(IF::from_impl2, s2->func());
    EXPECT_EQ(i_s1, s1->get_index());
    EXPECT_EQ(i_s2, s2->get_index());
}

TEST_F(PolyStorageBasicTest, SwapInline) {
    auto i_s1 = s1->get_index();
    estd::polymorphic_obj_storage_t<IF> t1(Impl1{});
    auto i_t1 = t1->get_index();
    std::swap(s1, t1);
    EXPECT_EQ(IF::from_impl1, s1->func());
    EXPECT_EQ(IF::from_impl1, t1->func());
    EXPECT_EQ(i_s1, t1->get_index());
    EXPECT_EQ(i_t1, s1->get_index());
}

TEST_F(PolyStorageBasicTest, SwapAllocated) {
    auto i_s2 = s2->get_index();
    auto p_s2 = s2.get();
    estd::polymorphic_obj_storage_t<IF> t2(Impl2{});
    auto i_t2 = t2->get_index();
    auto p_t2 = t2.get();
    std::swap(s2, t2);
    EXPECT_EQ(IF::from_impl2, s2->func());
    EXPECT_EQ(IF::from_impl2, t2->func());
    EXPECT_EQ(i_t2, s2->get_index());
    EXPECT_EQ(i_s2, t2->get_index());
    EXPECT_EQ(p_t2, s2.get());
    EXPECT_EQ(p_s2, t2.get());
}


}
