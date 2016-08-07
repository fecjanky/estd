#include "poly_vector.h"


struct Interface {
    virtual void function() = 0;
    virtual Interface* clone(void*) = 0;
    virtual ~Interface() = default;
};

class Impl1 : public Interface {
public:
    Impl1(double dd = 0.0) : d{dd} {}
    void function() override { 
        d += 1.0;
    }
    virtual Interface* clone(void* dest) {
        return new (dest) Impl1(*this);
    }


private:
    double d;
};
void test_poly_vector() {
    estd::poly_vector<Interface> v;
    v.push_back(Impl1{3.14});
    v.push_back(Impl1{6.28});
    for (auto& i : v) {
        int a = 0;
        i.function();
    }
    auto v2 = v;
    v[0].function();
    v.pop_back();
    v.back().function();
    for (auto& i : v) {
        int a = 0;
    }
    


}