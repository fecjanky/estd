[![Build Status](https://travis-ci.org/fecjanky/estd.svg?branch=poly_container_dev)](https://travis-ci.org/fecjanky/estd)  [![Coverage Status](https://coveralls.io/repos/github/fecjanky/estd/badge.svg?branch=poly_container_dev)](https://coveralls.io/github/fecjanky/estd?branch=poly_container_dev)
# estd

This header-only "library" contains some useful code which can be used as it was an STL extension. The name 'estd' is inspired by Bjarne Stroustrup - The C++ Programming Language book.
Currently it has the following headers:

* functional.h
* memory.h
* poly_vector.h

## functional.h

* the *interface* class is a generalization of std::function. *interface* class can be used for type-erasure of classes that provide some well defined usage interface

## memory.h

* *sso\_storage\_t* implements the small size optimization allocation strategy, it accepts the size threshold and Allocator policy as a template parameter
*  *polymorphic\_obj\_storage\_t* can be used for storing polymorphic objects applying small size optimization and is implemented through *sso\_storage\_t*


## poly_vector.h

* similar to std::vector but tailored for storing instances of polymorphic classes. 
Using this one can move from the std::vector<SmartPointer<Interface>> idiom to using 
estd::poly_vector\<Interface\>