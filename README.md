# A little single-header library for strings
This library contains wrappers for two types of C++ strings:
* IString is an "immutable" string, which is essentially a wrapper for a `const char*` an a length.
* MString is a "mutable" string, which behaves mostly like `std::string`. It does not allocate memory until it gets longer than 23 bytes.

This library is designed mostly with simplicity in mind. It provides basic functionality and operator overloads, but doesn't integrate with many STL features and isn't designed for ultimate performance. Also, I've hardly tested it, so it might not even compile with anything other than MSVC. It certainly will not work on big-endian platforms in the current state.

To use, `MString.h` is an stb-style single header library, if you're familiar with those. Essentially you can just add the `MString.h` header, include it wherever you need, and then in exactly one source file, you need to `#define MSTRING_IMPLEMENTATION` before including the header. That's it!

I don't consider this library particularly robust enough for serious use - like I said, I haven't fully tested it! I'm serious, I absolutely can't guarantee that this library is bug-free.
