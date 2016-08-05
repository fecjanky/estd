#include "poly_vector.h"


struct Interface {
    virtual void function() = 0;
    virtual Interface* clone(void*) = 0;
    virtual ~Interface() = default;
};

class Impl1 : public Interface {
public:
    Impl1() : d{} {}
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
    v.push_back(Impl1{});
    v.push_back(Impl1{});

}