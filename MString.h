#ifndef MSTRING_H

// This is formatted as a single-header library. You can include it wherever you want, and in exactly
// one source file you need to #define MSTRING_IMPLEMENTATION before including the header.
// There are some other library options you can change, either by adjusting them in this file, or by
// defining macros in the same place you #define MSTRING_IMPLEMENTATION.

// author: FrogBottom, with some help from Enlynn :)

// If you don't want us to use size_t, you can replace this with an integer type that you want instead.
// Note that the size of this integer affects the size of the MString struct, and thus the maximum length
// of a "short" string! On 64-bit platforms, An 8-byte integer type produces a 32-byte struct, and allows
// short strings to be 23 bytes long. A 4-byte integer type produces a 16 byte struct and allows 15-byte
// short strings. This type can be signed or unsigned, whichever you prefer (this library doesn't use
// negative values anywhere, and the asserts/bounds checks do still check for incorrect negative values).
typedef size_t MSTRING_SIZE_T;

// If you #define your own MSTRING_MALLOC, MSTRING_REALLOC, and MSTRING_FREE,
// then we don't need to #include <stdlib.h>, and will use your versions instead.

// If you #define MSTRING_MEMCPY, MSTRING_MEMMOVE, MSTRING_MEMCMP, and MSTRING_STRLEN,
// then we don't need to #include <string.h>, and will use your versions instead.

// If you #define MSTRING_ASSERT, then we don't need to #include <assert.h>.
// You can also define it to nothing if you don't want the asserts at all.

// An immutable string. Can be a wrapper for a const char* and length, or for other data.
// This does not own the string memory, and we don't do any checks for validity, this
// is just a convenience wrapper to simplify passing strings around.
struct IString
{
    IString() = default;
    IString(const char* ptr);
    constexpr IString(const char* ptr, MSTRING_SIZE_T length) : ptr(ptr), length(length) {}
    constexpr operator const char*() const {return ptr;}

    // Accessors for length and pointer. I would leave these as public fields, but
    // MString needs them to be accessor methods, so IString uses them too just for
    // API parity.
    constexpr MSTRING_SIZE_T Length() const {return length;}
    constexpr const char* Ptr() const {return ptr;}

    // Array access.
    constexpr const char& operator[](MSTRING_SIZE_T i) const {return Ptr()[i];}

    // "Legacy iterator" stuff.
    constexpr const char* begin() const {return Ptr();}
    constexpr const char* end() const {return Ptr() + Length();}

    // Comparison operators. Comparison with MString is implemented inside of MString.
    inline friend bool operator==(IString lhs, IString rhs);
    inline friend bool operator==(IString lhs, const char* rhs);
    inline friend bool operator==(const char* lhs, IString rhs);
    inline friend bool operator!=(IString lhs, IString rhs)     {return !(lhs == rhs);}
    inline friend bool operator!=(IString lhs, const char* rhs) {return !(lhs == rhs);}
    inline friend bool operator!=(const char* lhs, IString rhs) {return !(lhs == rhs);}

    private:
    const char* ptr;
    MSTRING_SIZE_T length;
};

// A mutable string. Doesn't allocate until the string length is long enough.
// Tries to stay null-terminated, but you can put non null-terminated strings
// in here too, if you know not to pass the result to somebody that expects a
// null-terminated string.
struct MString
{
    // Maximum length of a "short" string, not including the null terminator. The length is always
    // stored directly, but the rest of the struct can either contain a pointer + capacity + padding, or
    // can be repurposed to store shorter strings.
    constexpr static MSTRING_SIZE_T MaxShortLength = (2 * sizeof(MSTRING_SIZE_T)) + sizeof(char*) - 1;

    // Constructors. Default constructor produces a valid empty string.
    MString() = default;
    MString(const char* ptr, MSTRING_SIZE_T length);
    MString(const char* ptr);

    // Construction from IString has to be explicit since it might allocate.
    explicit MString(IString str) : MString(str.Ptr(), str.Length()) {}

    // Getters and setters for length and capacity and whatnot.
    constexpr bool IsHeap() const {return data.heap.is_heap;}
    constexpr MSTRING_SIZE_T Length() const {return length;}
    constexpr MSTRING_SIZE_T Capacity() const {return (IsHeap()) ? data.heap.capacity : MaxShortLength;}
    void SetLength(MSTRING_SIZE_T new_length);
    void ExpandIfNeeded(MSTRING_SIZE_T required_capacity);
    void ShrinkToFit();

    // Accessors for the raw pointer, auto-cast, and array subscript operators.
    constexpr const char* Ptr() const {return (IsHeap()) ? data.heap.ptr : data.stack;}
    constexpr char* Ptr() {return (IsHeap()) ? data.heap.ptr : data.stack;}

    constexpr operator IString() const {return IString(Ptr(), Length());}
    constexpr operator const char*() const {return Ptr();}
    constexpr operator char*() {return Ptr();}

    constexpr const char& operator[](MSTRING_SIZE_T i) const {return Ptr()[i];}
    constexpr char& operator[](MSTRING_SIZE_T i) {return Ptr()[i];}

    // "Legacy iterator" stuff.
    constexpr char* begin() {return Ptr();}
    constexpr char* end() {return Ptr() + Length();}
    constexpr const char* begin() const {return Ptr();}
    constexpr const char* end() const {return Ptr() + Length();}

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
    MString& Insert(MSTRING_SIZE_T index, const char* str, MSTRING_SIZE_T str_length);
    MString& Remove(MSTRING_SIZE_T index, MSTRING_SIZE_T count);

    inline MString& Insert(MSTRING_SIZE_T index, const MString& str) {return Insert(index, str.Ptr(), str.Length());}
    inline MString& Insert(MSTRING_SIZE_T index, const char* str); // Defined in implementation since it has to call strlen().
    inline MString& Insert(MSTRING_SIZE_T index, IString str)        {return Insert(index, str.Ptr(), str.Length());}
    inline MString& Insert(MSTRING_SIZE_T index, char c)             {return Insert(index, &c, 1);}

    inline MString& Prepend(const char* str, MSTRING_SIZE_T len) {return Insert(0, str, len);}
    inline MString& Prepend(const MString& str)                  {return Insert(0, str.Ptr(), str.Length());}
    inline MString& Prepend(const char* str); // Defined in implementation since it has to call strlen().
    inline MString& Prepend(IString str)                         {return Insert(0, str.Ptr(), str.Length());}
    inline MString& Prepend(char c)                              {return Insert(0, &c, 1);}

    inline MString& Append(const char* str, MSTRING_SIZE_T len) {return Insert(Length(), str, len);}
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
        char stack[MaxShortLength + 1];
        struct
        {
            char* ptr;
            MSTRING_SIZE_T capacity;
            char unused[MaxShortLength - sizeof(MSTRING_SIZE_T) - sizeof(char*)];
            char is_heap;
        } heap;
    } data;
    MSTRING_SIZE_T length;
};

#define MSTRING_H
#endif

// ========================================================================== //
// End of header. Implementation below.
// ========================================================================== //

#ifdef MSTRING_IMPLEMENTATION

// Include and use standard library versions of malloc, realloc, free, memcpy, memmove, memcmp, and strlen,
// if they were not defined by the user.
#if !defined MSTRING_MALLOC || !defined MSTRING_REALLOC || !defined MSTRING_FREE
#include <stdlib.h>
#endif
#if !defined MSTRING_MEMCPY || !defined MSTRING_MEMMOVE || ~defined MSTRING_MEMCMP || !defined MSTRING_STRLEN
#include <string.h>
#endif
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

// Misc one-liners that have to be in the implementation section because they call
// strlen() or memcmp(), which the caller of this library might re-define.
bool operator==(IString lhs, IString rhs)     {return (lhs.Length() == rhs.Length() && MSTRING_MEMCMP(lhs.Ptr(), rhs.Ptr(), lhs.Length()) == 0);}
bool operator==(IString lhs, const char* rhs) {return (lhs.Length() == (MSTRING_SIZE_T)MSTRING_STRLEN(rhs) && MSTRING_MEMCMP(lhs.Ptr(), rhs, lhs.Length()) == 0);}
bool operator==(const char* lhs, IString rhs) {return ((MSTRING_SIZE_T)MSTRING_STRLEN(lhs) == rhs.Length() && MSTRING_MEMCMP(lhs, rhs.Ptr(), rhs.Length()) == 0);}

bool operator==(const MString& lhs, const MString& rhs) {return (lhs.Length() == rhs.Length() && MSTRING_MEMCMP(lhs.Ptr(), rhs.Ptr(), lhs.Length()) == 0);}
bool operator==(const MString& lhs, IString rhs)        {return (lhs.Length() == rhs.Length() && MSTRING_MEMCMP(lhs.Ptr(), rhs.Ptr(), lhs.Length()) == 0);}
bool operator==(const MString& lhs, const char* rhs)    {return (lhs.Length() == (MSTRING_SIZE_T)MSTRING_STRLEN(rhs) && MSTRING_MEMCMP(lhs.Ptr(), rhs, lhs.Length()) == 0);}
bool operator==(IString lhs, const MString& rhs)        {return (lhs.Length() == rhs.Length() && MSTRING_MEMCMP(lhs.Ptr(), rhs.Ptr(), lhs.Length()) == 0);}
bool operator==(const char* lhs, const MString& rhs)    {return ((MSTRING_SIZE_T)MSTRING_STRLEN(lhs) == rhs.Length() && MSTRING_MEMCMP(lhs, rhs.Ptr(), rhs.Length()) == 0);}

IString::IString(const char* ptr) : ptr(ptr), length((MSTRING_SIZE_T)MSTRING_STRLEN(ptr)) {}
MString::MString(const char* ptr) : MString(ptr, (MSTRING_SIZE_T)MSTRING_STRLEN(ptr)) {}

MString& MString::Insert(MSTRING_SIZE_T index, const char* str) {return Insert(index, str, (MSTRING_SIZE_T)MSTRING_STRLEN(str));}
MString& MString::Prepend(const char* str) {return Insert(0, str, (MSTRING_SIZE_T)MSTRING_STRLEN(str));}
MString& MString::Append(const char* str) {return Insert(Length(), str, (MSTRING_SIZE_T)MSTRING_STRLEN(str));}

MString::MString(const char* ptr, MSTRING_SIZE_T len) : MString()
{
    MSTRING_ASSERT(ptr && len >= 0);

    if (len > 0)
    {
        if (len <= MaxShortLength) MSTRING_MEMCPY(data.stack, ptr, len);
        else
        {
            data.heap.is_heap = true;
            data.heap.ptr = (char*)MSTRING_MALLOC(len + 1);
            MSTRING_MEMCPY(data.heap.ptr, ptr, len);
            data.heap.capacity = len;
        }
    }

    Ptr()[len] = '\0';
    length = len;
}

void MString::SetLength(MSTRING_SIZE_T len)
{
    MSTRING_ASSERT(len >= 0);
    if (len == length) return;

    ExpandIfNeeded(len);
    Ptr()[len] = '\0';
    length = len;
}

void MString::ExpandIfNeeded(MSTRING_SIZE_T required_capacity)
{
    if (Capacity() >= required_capacity) return;
    // We'll double in size, or if that isn't enough we will just allocate exactly the required number of bytes.
    MSTRING_SIZE_T capacity = (Capacity() * 2 > required_capacity) ? Capacity() * 2 : required_capacity;
    // If we are already on the heap, just reallocate.
    if (IsHeap()) data.heap.ptr = (char*)MSTRING_REALLOC(data.heap.ptr, capacity + 1);
    else // Otherwise if we need to move to the heap for the first time, allocate and copy.
    {
        char* new_ptr = (char*)MSTRING_MALLOC(capacity + 1);
        if (length) MSTRING_MEMCPY(new_ptr, data.stack, length + 1);
        data.heap = {new_ptr, capacity, {}, true};
    }
}

void MString::ShrinkToFit()
{
    if (!IsHeap()) return; // If we aren't on the heap, there is nothing to shrink!

    if (length <= MaxShortLength) // Move back onto the stack if we are small enough.
    {
        char* ptr = data.heap.ptr;
        data = {};
        MSTRING_MEMCPY(data.stack, ptr, length + 1);
        MSTRING_FREE(ptr);
    }
    else
    {
        data.heap.ptr = (char*)MSTRING_REALLOC(data.heap.ptr, length + 1);
        data.heap.capacity = length;
    }
}

MString& MString::Insert(MSTRING_SIZE_T index, const char* str, MSTRING_SIZE_T len)
{
    MSTRING_ASSERT(str && index <= length && len >= 0);
    if (len <= 0 || index < 0 || !str) return *this;

    MSTRING_SIZE_T old_length = length;
    SetLength(old_length + len);
    if (index < old_length) MSTRING_MEMMOVE(Ptr() + index + len, Ptr() + index, old_length - index);
    else if (index == old_length) MSTRING_MEMCPY(Ptr() + index, str, len);
    return *this;
}


MString& MString::Remove(MSTRING_SIZE_T index, MSTRING_SIZE_T count)
{
    MSTRING_SIZE_T shift_index = index + count; // Start index of the bytes we need to shift forwards.
    MSTRING_ASSERT(index >= 0 && count >= 0 && shift_index <= length);
    if (count <= 0 || index < 0 || index >= length) return *this;

    if (shift_index < length) MSTRING_MEMMOVE(Ptr() + index, Ptr() + shift_index, length - shift_index);
    else if (shift_index > length) count = length - index;
    SetLength(length - count);
    return *this;
}

MString::MString(const MString& other)
{
    if (other.IsHeap())
    {
        data.heap.is_heap = true;
        data.heap.ptr = (char*)MSTRING_MALLOC(other.data.heap.capacity + 1);
        MSTRING_MEMCPY(data.heap.ptr, other.data.heap.ptr, other.length + 1);
        data.heap.capacity = other.data.heap.capacity;

    }
    else data = other.data;
    length = other.length;
}

MString::MString(MString&& other)
{
    data = other.data;
    other.data = {};
}

MString& MString::operator=(const MString& other)
{
    if (this != &other)
    {
        Free();
        SetLength(other.length);
        MSTRING_MEMCPY(Ptr(), other.Ptr(), length);
    }
    return *this;
}

MString& MString::operator=(MString&& other)
{
    if (this != &other)
    {
        Free();
        data = other.data;
        other.data = {};
    }
    return *this;
}

void MString::Free()
{
    if (IsHeap()) MSTRING_FREE(data.heap.ptr);
    data = {};
}

#endif