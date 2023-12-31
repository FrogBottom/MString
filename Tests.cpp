
#define MSTRING_IMPLEMENTATION
#include "MString.h"

#include <stdio.h>
#include <assert.h>

// Just runs a few basic sanity checks for MString and IString. Doesn't test every edge-case,
// doesn't validate that move/copy semantics are correct, and doesn't do any checking for
// correct use of malloc/free.
int main()
{
    printf("MString properties: Size of length type: %lld\n"
    "Size of structure: %lld\n"
    "Maximum short string length: %lld\n",
    sizeof(MString::MaxShortLength), sizeof(MString), (size_t)MString::MaxShortLength);

    printf("Testing Default Initialization:\n");
    {
        IString istring = {};
        assert(istring.Length() == 0);
        assert(istring.Ptr() == 0);
        MString mstring = {};
        assert(mstring.Length() == 0);
    }

    printf("Testing Equality Operators:\n");
    {
        MString mstring = "hello, this is a string.";
        assert(mstring == (MString)"hello, this is a string.");
        assert(mstring == (IString)"hello, this is a string.");
        assert(mstring == "hello, this is a string.");
        assert(mstring != (MString)"hello, this is a string2.");
        assert(mstring != (IString)"hello, this is a string2.");
        assert(mstring != "hello, this is a string2.");
    }

    printf("Testing MString heap behavior:\n");
    {
	    MString long_test = MString("A significantly longer string, for no other reason than because I need to test whether this gets heap allocated or not.");
        assert(long_test.IsHeap());
        MString edge_test = MString("");
        edge_test.SetLength(MString::MaxShortLength);
        for (MSTRING_SIZE_T i = 0; i < MString::MaxShortLength; ++i) edge_test[i] = 'a' + (char)i;
        assert(!edge_test.IsHeap());
        assert(edge_test.Length() == MString::MaxShortLength);
        assert(edge_test.Capacity() == MString::MaxShortLength);
        edge_test += 'x';
        assert(edge_test.IsHeap());
        assert(edge_test.Length() == MString::MaxShortLength + 1);
        assert(edge_test.Capacity() >= MString::MaxShortLength + 1);
        edge_test.SetLength(30);
        assert(edge_test.Length() == 30);
        assert(edge_test[30] == '\0');
        assert(edge_test.Capacity() >= 30);

        edge_test.SetLength(7);
        edge_test.ShrinkToFit();
        assert(!edge_test.IsHeap());
        assert(edge_test == "abcdefg");
    }

    printf("Testing MString appending and removing:\n");
    {
        MString str = "example string";
        str += ", and a slightly longer C string";
        str += MString(", and another MString to see if that works.");
        assert(str == "example string, and a slightly longer C string, and another MString to see if that works.");
        str.Remove(14, 75);
        assert(str == "example string");
        str.Remove(8, 2);
        assert(str == "example ring");
        str.Remove(0, 8);
        assert(str == "ring");
    }

    printf("Testing append and prepend chaining:\n");
    {
	    MString appended = MString("C:\\") + "Users" + '\\' + "MyFavoriteUser" + '\\' + "SomePath";
        assert(appended == "C:\\Users\\MyFavoriteUser\\SomePath");

        MString another = MString("SomePath").Prepend('\\').Prepend("MyFavoriteUser").Prepend('\\').Prepend("Users").Prepend("C:\\");
        assert(appended == another);
    }

    printf("Testing insert:\n");
    {
        MString test = "some string wherewas inserted.";
        test.Insert(17, "another string ");
        assert(test == "some string where another string was inserted.");
    }

    printf("Testing non-ascii characters and null bytes:\n");
    {
        MString str = MString("SomePath") + "/" + (const char*)u8"مرحبا بالعالم";
        assert(str == "SomePath/مرحبا بالعالم");

        str = "Some sort of longer string which has a length greater than 10 bytes.";
        str[10] = '\0';
        assert(str.Length() > 10);
    }

    return 0;
}