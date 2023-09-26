// This is formatted as a single-header library. You can include it wherever you want, and in exactly
// one source file you need to #define MSTRING_IMPLEMENTATION before including the header.
// (author: FrogBottom)

// @Revisit(Frog): For MString, is being able to hold 23 bytes instead of 22 on the stack *actually* worth
// having to do a subtract every time we want the length?

// @Todo(Frog): The small string optimization for MString totally won't work on big endian, also I'm not
// sure the union thing even compiles on non-MSVC, so we should probably just do bitmasks instead. Plus
// then we could define the bitmasks depending on endianness which would fix that too.

#ifndef MSTRING_H

// If you don't want us to include stdint.h, you can replace this with a type
// that you know will be an unsigned 64 bit integer on your target platform.
// @Revisit(Frog): Technically this does not need to be unsigned. The only
// problem is that our asserts never check for negative values.
#include <stdint.h>
typedef uint64_t MSTRING_U64;

// If you define MSTRING_MALLOC, MSTRING_REALLOC, and MSTRING_FREE,
// then we don't need to #include <stdlib.h>.
#if !defined MSTRING_MALLOC || !defined MSTRING_REALLOC || !defined MSTRING_FREE
#include <stdlib.h>
#endif

// If you define MSTRING_MEMCPY, MSTRING_MEMMOVE, MSTRING_MEMCMP, and MSTRING_STRLEN,
// then we don't need to #include <string.h>.
#if !defined MSTRING_MEMCPY || !defined MSTRING_MEMMOVE || ~defined MSTRING_MEMCMP || !defined MSTRING_STRLEN
#include <string.h>
#endif

// If you define MSTRING_ASSERT, then we don't need to #include <assert.h>.
// You can also define it to nothing if you don't want the asserts at all.
#ifndef MSTRING_ASSERT
#include <cassert>
#define MSTRING_ASSERT assert
#endif

#ifndef MSTRING_MALLOC
#define MSTRING_MALLOC(size) malloc(size)
#endif
#ifndef MSTRING_REALLOC
#define MSTRING_REALLOC(old_ptr, size) realloc(old_ptr, size)
#endif
#ifndef MSTRING_FREE
#define MSTRING_FREE(ptr) free(ptr)
#endif
#ifndef MSTRING_MEMCPY
#define MSTRING_MEMCPY(dst, src, size) memcpy(dst, src, size)
#endif
#ifndef MSTRING_MEMMOVE
#define MSTRING_MEMMOVE(dst, src, size) memmove(dst, src, size)
#endif
#ifndef MSTRING_MEMCMP
#define MSTRING_MEMCMP(lhs, rhs, size) memcmp(lhs, rhs, size)
#endif
#ifndef MSTRING_STRLEN
#define MSTRING_STRLEN(str) strlen(str)
#endif

// An immutable string. Can be a wrapper for a const char* and length, or for other data.
// This does not own the string memory, and we don't do any checks for validity, this
// is just a convenience wrapper to simplify passing strings around.
// @Todo(Frog): What are the consequences of allowing default construction? Maybe we should default it.
struct IString
{
    IString() = delete;
    const IString(const char* ptr);
    constexpr IString(const char* ptr, MSTRING_U64 length) : ptr(ptr), length(length) {}
    constexpr operator const char*() const {return ptr;}

    // @Revisit(Frog): I hate having these as accessors, but I left them his way so that
    // the API is identical for IString and MString. I'm not sure though, I might just
    // remove these and leave the fields public, especially since they are already const!
    constexpr MSTRING_U64 Length() const {return length;}
    constexpr const char* Ptr() const {return ptr;}

    // Comparison operators. Comparison with MString is implemented inside of MString.
    inline friend bool operator==(IString lhs, IString rhs);
    inline friend bool operator==(IString lhs, const char* rhs);
    inline friend bool operator==(const char* lhs, IString rhs);

    inline friend bool operator!=(IString lhs, IString rhs)     {return !(lhs == rhs);}
    inline friend bool operator!=(IString lhs, const char* rhs) {return !(lhs == rhs);}
    inline friend bool operator!=(const char* lhs, IString rhs) {return !(lhs == rhs);}

    private:
    const char* ptr;
    const MSTRING_U64 length;
};

// A mutable string. Doesn't allocate memory until the length exceeds 23 bytes.
// Tries to stay null-terminated, but you can put non null-terminated strings
// in here too, if you know not to pass the result to somebody that expects a
// null-terminated string.
struct MString
{
    // Constructors. Default constructor produces a valid empty string.
    MString() = default;
    MString(const char* ptr, MSTRING_U64 length);
    MString(const char* ptr);

    // Construction from IString has to be explicit since it might allocate.
    explicit MString(IString str) : MString(str.Ptr(), str.Length()) {}

    // Getters and setters for length and capacity and whatnot.
    constexpr MSTRING_U64 Length() const {return (long_data.is_heap) ? long_data.length : 23 - short_data[23];}
    constexpr MSTRING_U64 Capacity() const {return (long_data.is_heap) ? long_data.capacity : 23;}
    constexpr bool IsHeap() const {return long_data.is_heap;}
    void SetLength(MSTRING_U64 new_length);
    void ExpandIfNeeded(MSTRING_U64 requred_capacity);
    void ShrinkToFit();

    // Accessors for the raw pointer, auto-cast, and array subscript operators.
    constexpr const char* Ptr() const {return (long_data.is_heap) ? long_data.ptr : short_data;}
    constexpr char* Ptr() {return (long_data.is_heap) ? long_data.ptr : short_data;}

    constexpr operator IString() const {return IString(Ptr(), Length());}
    constexpr operator const char*() const {return Ptr();}
    constexpr operator char*() {return Ptr();}

    constexpr const char& operator[](MSTRING_U64 i) const {return Ptr()[i];}
    constexpr char& operator[](MSTRING_U64 i) {return Ptr()[i];}

    // Comparison operators.
    // @Speed(Frog): These could be faster if they didn't call memcmp(), we don't care about lexicographic ordering.
    inline friend bool operator==(const MString& lhs, const MString& rhs);
    inline friend bool operator==(const MString& lhs, IString rhs);
    inline friend bool operator==(const MString& lhs, const char* rhs);
    inline friend bool operator==(IString lhs, const MString& rhs);
    inline friend bool operator==(const char* lhs, const MString& rhs);

    inline friend bool operator!=(const MString& lhs, const MString& rhs) {return !(lhs == rhs);}
    inline friend bool operator!=(const MString& lhs, IString rhs)        {return !(lhs == rhs);}
    inline friend bool operator!=(const MString& lhs, const char* rhs)    {return !(lhs == rhs);}
    inline friend bool operator!=(IString lhs, const MString& rhs)        {return !(lhs == rhs);}
    inline friend bool operator!=(const char* lhs, const MString& rhs)    {return !(lhs == rhs);}

    // These are the methods that do actual work. Most remaining methods and operators
    // will just inline a call to Insert(), and many are only here to remove type ambiguity.
    MString& Insert(MSTRING_U64 index, const char* str, MSTRING_U64 str_length);
    MString& Remove(MSTRING_U64 index, MSTRING_U64 count);
    
    inline MString& Insert(MSTRING_U64 index, const MString& str) {return Insert(index, str.Ptr(), str.Length());}
    inline MString& Insert(MSTRING_U64 index, const char* str); // Defined in implementation since it has to call strlen().
    inline MString& Insert(MSTRING_U64 index, IString str)        {return Insert(index, str.Ptr(), str.Length());}
    inline MString& Insert(MSTRING_U64 index, char c)             {return Insert(index, &c, 1);}

    inline MString& Prepend(const char* str, MSTRING_U64 length) {return Insert(0, str, length);}
    inline MString& Prepend(const MString& str)                  {return Insert(0, str.Ptr(), str.Length());}
    inline MString& Prepend(const char* str); // Defined in implementation since it has to call strlen().
    inline MString& Prepend(IString str)                         {return Insert(0, str.Ptr(), str.Length());}
    inline MString& Prepend(char c)                              {return Insert(0, &c, 1);}

    inline MString& Append(const char* str, MSTRING_U64 length) {return Insert(Length(), str, length);}
    inline MString& Append(const MString& str)                  {return Insert(Length(), str.Ptr(), str.Length());}
    inline MString& Append(const char* str); // Defined in implementation since it has to call strlen().
    inline MString& Append(IString str)                         {return Insert(Length(), str.Ptr(), str.Length());}
    inline MString& Append(char c)                              {return Insert(Length(), &c, 1);}

    inline MString& operator+=(const MString& rhs) {return Insert(Length(), rhs);}
    inline MString& operator+=(const char* rhs)    {return Insert(Length(), rhs);}
    inline MString& operator+=(IString rhs)        {return Insert(Length(), rhs);}
    inline MString& operator+=(char rhs)           {return Insert(Length(), rhs);}

    // Passing one argument by value and then returning it helps the compiler figure out that it should
    // use the move constructor when we chain a bunch of + operators together.
    inline friend MString operator+(MString lhs, const MString& rhs) {lhs.Insert(lhs.Length(), rhs); return lhs;}
    inline friend MString operator+(MString lhs, const char* rhs)    {lhs.Insert(lhs.Length(), rhs); return lhs;}
    inline friend MString operator+(MString lhs, IString rhs)        {lhs.Insert(lhs.Length(), rhs); return lhs;}
    inline friend MString operator+(MString lhs, char rhs)           {lhs.Insert(lhs.Length(), rhs); return lhs;}

    inline friend MString operator+(const char* lhs, MString rhs)    {rhs.Insert(0, lhs); return rhs;}
    inline friend MString operator+(IString lhs, MString rhs)        {rhs.Insert(0, lhs); return rhs;}
    inline friend MString operator+(char lhs, MString rhs)           {rhs.Insert(0, lhs); return rhs;}

    // Copy and move constructor/assignment nonsense.    
    MString(const MString& other);
    MString(MString&& other);
    MString& operator=(const MString& other);
    MString& operator=(MString&& other);

    // Destructor (or you can call Free() to deallocate).
    void Free();
    ~MString() {Free();}

    private:
    union
    {
        struct LongData
        {
            char* ptr;
            MSTRING_U64 capacity; // Total allocated space, NOT including null terminator.
            MSTRING_U64 length : 63; // String length, NOT including null terminator.
            MSTRING_U64 is_heap : 1; // Flag indicates that this string is heap-allocated.
        } long_data;

        // The space used by long_data is repurposed to hold the string if it is short enough.
        // The is_heap flag is the high bit of the last byte (at least on little-endian platforms), which
        // will always be zero when the string is "short". The last byte encodes (23 - length) so that
        // when length is 23, it doubles as the null terminator.
        char short_data[24];
    };
};

#define MSTRING_H
#endif

// ========================================================================== //
// End of header. Implementation below.
// ========================================================================== //

#ifdef MSTRING_IMPLEMENTATION

// Misc one-liners that have to be in the implementation section because they call
// strlen() or memcmp(), which the caller of this library might re-define.
bool operator==(IString lhs, IString rhs)     {return (lhs.Length() == rhs.Length() && MSTRING_MEMCMP(lhs.Ptr(), rhs.Ptr(), lhs.Length()) == 0);}
bool operator==(IString lhs, const char* rhs) {return (lhs.Length() == MSTRING_STRLEN(rhs) && MSTRING_MEMCMP(lhs.Ptr(), rhs, lhs.Length()) == 0);}
bool operator==(const char* lhs, IString rhs) {return (MSTRING_STRLEN(lhs) == rhs.Length() && MSTRING_MEMCMP(lhs, rhs.Ptr(), rhs.Length()) == 0);}

bool operator==(const MString& lhs, const MString& rhs) {return (lhs.Length() == rhs.Length() && MSTRING_MEMCMP(lhs.Ptr(), rhs.Ptr(), lhs.Length()) == 0);}
bool operator==(const MString& lhs, IString rhs)        {return (lhs.Length() == rhs.Length() && MSTRING_MEMCMP(lhs.Ptr(), rhs.Ptr(), lhs.Length()) == 0);}
bool operator==(const MString& lhs, const char* rhs)    {return (lhs.Length() == MSTRING_STRLEN(rhs) && MSTRING_MEMCMP(lhs.Ptr(), rhs, lhs.Length()) == 0);}
bool operator==(IString lhs, const MString& rhs)        {return (lhs.Length() == rhs.Length() && MSTRING_MEMCMP(lhs.Ptr(), rhs.Ptr(), lhs.Length()) == 0);}
bool operator==(const char* lhs, const MString& rhs)    {return (MSTRING_STRLEN(lhs) == rhs.Length() && MSTRING_MEMCMP(lhs, rhs.Ptr(), rhs.Length()) == 0);}

IString::IString(const char* ptr) : ptr(ptr), length(MSTRING_STRLEN(ptr)) {}
MString::MString(const char* ptr) : MString(ptr, MSTRING_STRLEN(ptr)) {}

MString& MString::Insert(MSTRING_U64 index, const char* str) {return Insert(index, str, MSTRING_STRLEN(str));}
MString& MString::Prepend(const char* str) {return Insert(0, str, MSTRING_STRLEN(str));}
MString& MString::Append(const char* str) {return Insert(Length(), str, MSTRING_STRLEN(str));}

MString::MString(const char* ptr, MSTRING_U64 length) : MString()
{
    MSTRING_ASSERT(ptr);
    if (length < 24)
    {
        MSTRING_MEMCPY(short_data, ptr, length);
        short_data[length] = '\0';
        short_data[23] = (char)(23 - length);
    }
    else
    {
        long_data.is_heap = true;
        long_data.ptr = (char*)MSTRING_MALLOC(length + 1);
        MSTRING_MEMCPY(long_data.ptr, ptr, length);
        long_data.ptr[length] = '\0';
        long_data.length = length;
        long_data.capacity = length;
    }

}

void MString::SetLength(MSTRING_U64 length)
{
    if (length == Length()) return;
    ExpandIfNeeded(length);

    if (long_data.is_heap)
    {
        long_data.length = length;
        long_data.ptr[length] = '\0';
    }
    else
    {
        short_data[23] = 23 - length;
        short_data[length] = '\0';
    }
}

void MString::ExpandIfNeeded(MSTRING_U64 required_capacity)
{
    MSTRING_U64 old_capacity = Capacity();
    if (required_capacity < 24 || old_capacity >= required_capacity) return;

    // We'll double in size, or if that isn't enough we will just allocate exactly the required number of bytes.
    MSTRING_U64 new_capacity = (old_capacity * 2 > required_capacity) ? old_capacity * 2 : required_capacity;
    
    // If we are already on the heap, just reallocate. 
    if (long_data.is_heap) long_data.ptr = (char*)MSTRING_REALLOC(long_data.ptr, new_capacity + 1);
    else // Otherwise if we need to move to the heap for the first time, allocate and copy.
    {
        char* new_ptr = (char*)MSTRING_MALLOC(new_capacity + 1);
        MSTRING_U64 old_length = Length();
        if (old_length) MSTRING_MEMCPY(new_ptr, short_data, old_length + 1);
        long_data.is_heap = true;
        long_data.ptr = new_ptr;
        long_data.length = old_length;
    }
    
    long_data.capacity = new_capacity;
}

void MString::ShrinkToFit()
{
    if (!long_data.is_heap) return; // If we aren't on the heap, there is nothing to shrink!

    MSTRING_U64 length = Length();
    if (length < 24) // Move back onto the stack if we are small enough.
    {
        char* ptr = long_data.ptr;
        long_data = {};
        MSTRING_MEMCPY(short_data, ptr, length + 1);
        short_data[23] = 23 - length;
    }
    else
    {
        long_data.ptr = (char*)MSTRING_REALLOC(long_data.ptr, length + 1);
        long_data.capacity = length;
    }
}

MString& MString::Insert(MSTRING_U64 index, const char* str, MSTRING_U64 length)
{
    MSTRING_ASSERT(str && index <= Length());
    
    MSTRING_U64 old_length = Length();
    SetLength(old_length + length);

    if (index < old_length) MSTRING_MEMMOVE(Ptr() + index + length, Ptr() + index, old_length - index);
    if (length > 0) MSTRING_MEMCPY(Ptr() + index, str, length);

    return *this;
}

MString& MString::Remove(MSTRING_U64 index, MSTRING_U64 count)
{
    MSTRING_ASSERT(index + count <= Length());
    if (count == 0) return *this;

    MSTRING_U64 old_length = Length();
    if (index + count < old_length) MSTRING_MEMMOVE(Ptr() + index, Ptr() + index + count, count);
    SetLength(old_length - count);

    return *this;
}

MString::MString(const MString& other)
{
    if (other.long_data.is_heap)
    {
        long_data.is_heap = true;
        long_data.ptr = (char*)MSTRING_MALLOC(other.long_data.capacity + 1);
        MSTRING_MEMCPY(long_data.ptr, other.long_data.ptr, other.long_data.length + 1);
        long_data.length = other.long_data.length;
        long_data.capacity = other.long_data.capacity;

    }
    else long_data = other.long_data;
}

MString::MString(MString&& other)
{
    long_data = other.long_data;
    other.long_data = {};
}

MString& MString::operator=(const MString& other)
{
    if (this != &other)
    {
        Free();
        MSTRING_U64 length = other.Length();
        SetLength(length);
        MSTRING_MEMCPY(Ptr(), other.Ptr(), length);
    }
    return *this;
}

MString& MString::operator=(MString&& other)
{
    if (this != &other)
    {
        Free();
        long_data = other.long_data;
        other.long_data = {};
    }
    return *this;
}

void MString::Free()
{
    if (long_data.is_heap) MSTRING_FREE(long_data.ptr);
    long_data = {};
}

#endif