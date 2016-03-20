# estd  ![Build Status](https://travis-ci.org/fecjanky/estd.svg?branch=master)](https://travis-ci.org/fecjanky/estd)

This header-only "library" contains some useful code which can be used as it was an STL extension. The name 'estd' is inspired by Bjarne Stroustrup - The C++ Programming Language book.
Currently it has 2 headers:

* functional.h
* memory.h

## functional.h

* the *interface* class is a generalization for std::function. *interface* class can be used for type-erasure classes that provide some well defined usage interface

## memory.h

* *sso\_storage\_t* implements the small size optimization allocation strategy, it accepts the size threshold and Allocator policy as a template parameter
*  *polymorphic\_obj\_storage\_t* can be used for storing polymorphic objects applying small size optimization
