#include <stdint.h>
#include <stdio.h>

/////////////////////////////////
// Example 1: Member Alignment //
/////////////////////////////////

struct Foo {
    uint32_t a; // 4
    uint8_t  b; // 1
				// 1
	uint16_t c; // 2
	uint8_t  d; // 1
				// 7
	uint64_t e; // 8
};

struct Goo {
    uint64_t a; // 8
	uint32_t b; // 4
	uint16_t c; // 2
    uint8_t  d; // 1
	uint8_t  e; // 1
};

//In this first example, the sizes of Foo's members sum to 16 bytes, but the compiler adds so much padding that the struct expands
//to 24 bytes instead. You can see where these additional bytes come from in the comments. To understand why the compiler added
//all of this padding, we need to learn a little bit about "alignment".

//Alignment means a piece of data is stored at an address that is a multiple of its alignment requirement (on modern hardware, this is 
//almost always the size of the data). For example, if a piece of data is 4 bytes wide (like an int), alignment requires that it be 
//located at an address divisible by 4. That means 0x00 is a valid address, 0x04 is valid, 0x08 is valid, etc. All of these addresses 
//are multiples of 4.

//The question is, Why do we need to align our data? Thankfully, the answer is pretty simple.

//When the CPU wants to access a piece of data, it does so in aligned chunks. On modern hardware, these chunks will often be 8 bytes 
//wide (although they can be higher or lower). In this description, we will assume 8-byte accesses, but the same rules apply regardless 
//of access size. These 8-byte chunks are not pulled from arbitrary addresses. Instead, they are always pulled from addresses that are 
//multiples of 8 (i.e., the access size). This means, starting from address zero, the CPU will grab 0x00, 0x08, 0x10, etc. All of these 
//addresses are multiples of 8. Now ask yourself, Which address will the CPU grab to retrieve a 4-byte integer stored at 0x04?

//The answer is 0x00. This will give the CPU all of the data between 0x00 and 0x08, which includes 0x04-0x07 (i.e., our 4-byte integer). 
//So far so good! But what happens when our 4-byte integer is *misaligned*? What happens when the integer lives, not at 0x04, which is a 
//multiple of its size, but at 0x06? From there, it would span the addresses 0x06-0x09, which crosses an 8-byte boundary. When the CPU 
//goes to retrieve this value, it will start at 0x00, grab all of the data from 0x00 to 0x07, and discover that the data is incomplete. 
//This means the CPU will need to retrieve the rest of the data between addresses 0x08 and 0x0F. Once that is done, the data can be 
//stitched together internally.

//The point here is not about the underlying details of how the CPU retrieves and stitches data. It's that misaligned data can lead to 
//unnecessary accesses (either in the cache or RAM), which aren't cheap. In the previous example, the CPU had to perform two accesses 
//for a single piece of data. That's twice the work for absolutely no benefit! To ensure these kinds of issues don't occur, the compiler 
//will align your data for you. Sometimes that means padding structs with additional unused (but hopefully no longer unexpected!) bytes. 
//Thankfully, you can minimize these additions by carefully organizing your data, as was done in the Goo struct.

////////////////////////////
// Example 2: End Padding //
////////////////////////////

//Now we understand why compilers add padding between members in structs. But there's a wrinkle: Sometimes the compiler will also add 
//padding at the *end* of a struct. Let's think about that for a second. In the first example, the compiler added padding to Foo to 
//align `c`, which was a 2-byte unsigned integer. If it hadn't done that, `c` would have been stored at a misaligned address (0x05, if 
//the struct was stored at 0x00) and could have caused access issues. The same logic applies to `e`, which needed to be aligned to an 
//address divisible by 8 (in this case, 0x10). In both cases, padding ensured that each member would be accessed according to its
//alignment requirement. So why would the compiler ever add padding *at the end of a struct*?

//It's clear that "end padding" is not used to align the members of a struct. They are already aligned by the padding *between* members.
//Instead, end padding is used to align structs arrayed contiguously in memory. 

//Note: Misalignment can cause crashes on some architectures!

//Without end padding: 9 bytes
struct __attribute__((packed)) Moo {
	uint64_t a; // 8
	uint8_t  b; // 1
};

//With end padding: 16 bytes
struct Woo {
	uint64_t a; // 8
	uint8_t  b; // 1
				// 7
};

//Without end padding, and starting from address 0x00, two Moos will look like this in an array:
// First Moo:
// 0x00-0x07 = a;
// 0x08 	 = b;

// Second Moo
// 0x09-0x11 = a;
// 0x12 	 = b;

//See the problem? The first Moo is totally fine, because both of its members are happily aligned. But what about the second Moo? Well, 
//it's first member, `a`, begins at address 0x09, which is not an aligned address for an 8-byte integer. That means, despite the second
//Moo's internal alignment, it is not properly aligned in an array. The first Moo causes the second Moo to be misaligned.

//Let's see how Woo fairs in an array:
// First Woo:
// 0x00-0x07 = `a`
// 0x08 	 = `b`
// 0x09-0x0F = end padding

// Second Woo
// 0x10-0x17 = `a`
// 0x18 	 = `b`
// 0x19-0x1F = end padding

//With Woo, everything remains aligned, even in arrays. End padding is used for just this purpose. End padding rounds the size of a
//struct to a multiple of its largest member, in this case uint64_t (8 bytes). This places each struct in an array at an aligned 
//address, which ensures all of its members will be truly, properly aligned.

int main(void) {
	//Example 1
	Foo f = {0};
	Goo g = {0};
	
	printf("Example 1\n---------\n");
	printf("Foo has size: %zu\n", sizeof(f)); //24
	printf("Goo has size: %zu\n", sizeof(g)); //16

	//Example 2
	Moo m = {0};
	Woo w = {0};
	
	Moo m_array[2] = {
		m,
		m
	};
	
	Woo w_array[2] = {
		w,
		w
	};

	ptrdiff_t m1_relative = (char *)&m_array[1] - (char *)&m_array[0];
	ptrdiff_t w1_relative = (char *)&w_array[1] - (char *)&w_array[0];

	printf("\nExample 2\n---------\n");

	printf("Moo has size: %zu\n", sizeof(m));
	printf("Woo has size: %zu\n", sizeof(w));

	printf("\n[Moo Array]\n");
	printf("Address of 1st Moo: 0x%#tx\n", 0);
	printf("Address of 2nd Moo: %#tx\n", m1_relative);
	printf("Difference between addresses: %td\n", m1_relative);
	
	printf("\n[Woo Array]\n");
	printf("Address of 1st Woo: 0x%#tx\n", 0);
	printf("Address of 2nd Woo: %#tx\n", w1_relative);
	printf("Difference between addresses: %td\n", w1_relative);
	
	return 0;
}

////////////
// Output //
////////////

//Example 1
//---------
//Foo has size: 24
//Goo has size: 16
//
//Example 2
//---------
//Moo has size: 9
//Woo has size: 16
//
//[Moo Array]
//Address of 1st Moo: 0x0
//Address of 2nd Moo: 0x9
//Difference between addresses: 9
//
//[Woo Array]
//Address of 1st Woo: 0x0
//Address of 2nd Woo: 0x10
//Difference between addresses: 16

//Final note:
//Unaligned loads and stores are not friendly to multithreaded applications. An unaligned load of a simple variable, say an `int`, 
//can require two loads in reality, which are combined to produce the final value. It is possible for a CPU to context switch
//between those loads (which may be *individually* atomic), allowing a new thread to write into the second loaded address. Once the
//CPU switches back, the second load will see new, irrelevant data, which will be stitched with the previous load and create chaos.
//This is called a "torn read". Just one more reason to understand padding and memory alignment!
