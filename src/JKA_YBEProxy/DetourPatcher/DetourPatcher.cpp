// ==================================================
// DetourPatcher by Deathspike
// --------------------------------------------------
// Collection of functions written to assist in detouring
// functions and patching bytes. It mainly allocates
// trampolines to execute the original bytes and uses
// jumps to go to the new function. Supports both
// Windows and Linux operating systems.
// ==================================================

#ifndef WIN32
	#include <sys/mman.h>
	#include <unistd.h>
#else
	#include <windows.h>
	unsigned long OldProtect;
#endif

#include		<cstdlib>
#include		<cstdio>
#include		<cstring>
#include		"DetourPatcher.hpp"

// ==================================================
// Attach
// --------------------------------------------------
// Attaches a detour on the target function and makes 
// it jump to the new provided address, should be a
// pointer.
// ==================================================

unsigned char *Attach( unsigned char *pAddress, unsigned char *pNewAddress )
{
    size_t iLen = GetLen( pAddress );
	unsigned char *pTramp = GetTramp( pAddress, iLen );

	UnProtect( pAddress, iLen );
	*(( unsigned char * )(( unsigned int ) pAddress )) = 0xE9;
	*(( unsigned int *  )(( unsigned int ) pAddress + 1 )) = ( unsigned int ) pNewAddress - ( unsigned int )(( unsigned int ) pAddress + 5 );
	ReProtect( pAddress, iLen );

	return pTramp;
}

// ==================================================
// Detach
// --------------------------------------------------
// Detaches a function detour. The length is calculated
// and the original bytes are restored, tramp is then
// released.
// ==================================================

unsigned char *Detach( unsigned char *pAddress, unsigned char *pTramp )
{
    size_t iLen = GetLen( pAddress );

	UnProtect( pAddress, iLen );
	memcpy( pAddress, pTramp, iLen );
	ReProtect( pAddress, iLen );

	free( pTramp );
	return pAddress;
}

// ==================================================
// GetLen
// --------------------------------------------------
// Retrieve the minimum ammount of opcodes to patch
// at the given instruction. Utilizes the DisAssemble
// function to count the opcodes.
// ==================================================

size_t GetLen( unsigned char *pAddress )
{
	size_t iLen = 0;
	size_t iSize = 0;
    
	while( iSize < 5 )
	{
		DisAssemble( pAddress, &iLen );
		pAddress += iLen;
		iSize += iLen;
	}

    return iSize;
}

// ==================================================
// GetTramp
// --------------------------------------------------
// Creates a trampoline, allocates the minimum amount
// of memory for the length, copies the opcodes to
// the trampoline and creates a jump to continue
// execution.
// ==================================================

unsigned char *GetTramp( unsigned char *pAddress, size_t iLen )
{
	unsigned char *pTramp = ( unsigned char * ) malloc( iLen + 5 );

	memcpy( pTramp, pAddress, iLen );
	*( unsigned char * )( pTramp + iLen ) = 0xE9;
	*( unsigned int * )( pTramp + iLen + 1 ) = ( unsigned int )(( unsigned int )( pAddress + iLen ) - ( unsigned int )( pTramp + iLen + 5 ));

	return pTramp;
}

// ==================================================
// InlineFetch
// --------------------------------------------------
// Returns the address which is stored in anything
// similar to a jump or call (4 bytes address after
// a single opcode).
// ==================================================

unsigned int InlineFetch( unsigned char *pAddress )
{
	unsigned int *pAddressBase = ( unsigned int * )( pAddress + 5 );
	unsigned int pAddressOffset = *( unsigned int * )( pAddress + 1 );
	return ( unsigned int ) pAddressBase + ( unsigned int ) pAddressOffset;
}

// ==================================================
// InlinePatch
// --------------------------------------------------
// Patches the address which is stored in anything
// similar to a jump or call (4 bytes address after
// a single opcode).
// ==================================================

unsigned int InlinePatch( unsigned char *pAddress, unsigned char *pNewAddress )
{
	unsigned int pReturnAddress = InlineFetch( pAddress );

	UnProtect( pAddress, 5 );
	*( unsigned int * )( pAddress + 1 )= ( unsigned int )(( unsigned int )( pNewAddress ) - ( unsigned int )( pAddress + 5 ));
	ReProtect( pAddress, 5 );

	return pReturnAddress;
}

// ==================================================
// Patch
// --------------------------------------------------
// Patches a single byte at the target address, 
// usually performed to skip instructions from that 
// given address.
// ==================================================

void Patch( unsigned char *pAddress, unsigned char bByte )
{
	UnProtect( pAddress, 1 );
	*pAddress = bByte;
	ReProtect( pAddress, 1 );
}

// ==================================================
// ReProtect
// --------------------------------------------------
// Reapplies protection to the target address, must
// be called after any UnProtect call!
// ==================================================

void ReProtect( void *pAddress, size_t iLen )
{
#ifndef WIN32
	// Since we have no real way to get the original protection
	// on a Linux machine, we can't restore the bytes either.
#else
	VirtualProtect( pAddress, iLen, OldProtect, &OldProtect );
#endif
}

// ==================================================
// ReProtect
// --------------------------------------------------
// Removes protection to the target address, after
// which it can be written upon. Do not open an entire
// page. On linux we have to determine whether or not
// the entire length is on the same page.
// ==================================================

void UnProtect( void *pAddress, size_t iLen )
{
#ifndef WIN32

	int iPage1 = ( int ) pAddress &~ ( getpagesize() - 1 );
	int iPage2 = (( int ) pAddress + iLen ) &~ ( getpagesize() - 1 );

	if ( iPage1 == iPage2 )
	{
		mprotect(( char * ) iPage1, getpagesize(), PROT_READ | PROT_WRITE | PROT_EXEC );
	}
	else
	{
		mprotect(( char * ) iPage1, getpagesize(), PROT_READ | PROT_WRITE | PROT_EXEC );
		mprotect(( char * ) iPage2, getpagesize(), PROT_READ | PROT_WRITE | PROT_EXEC );
	}

#else
	VirtualProtect( pAddress, iLen, /*PAGE_EXECUTE_READWRITE*/ 0x40, &OldProtect );
#endif
}

// ==================================================
// The following function is the disassembler library,
// definitions and utilized functions. It is not
// recommended to alter this.
// ==================================================

typedef unsigned long	DWORD;
#define C_ERROR         0xFFFFFFFF
#define C_PREFIX        0x00000001
#define C_66            0x00000002
#define C_67            0x00000004
#define C_DATA66        0x00000008
#define C_DATA1         0x00000010
#define C_DATA2         0x00000020
#define C_DATA4         0x00000040
#define C_MEM67         0x00000080
#define C_MEM1          0x00000100
#define C_MEM2          0x00000200
#define C_MEM4          0x00000400
#define C_MODRM         0x00000800
#define C_DATAW0        0x00001000
#define C_FUCKINGTEST   0x00002000
#define C_TABLE_0F      0x00004000

DWORD table_1[256] =
{
  /* 00 */   C_MODRM
  /* 01 */,  C_MODRM
  /* 02 */,  C_MODRM
  /* 03 */,  C_MODRM
  /* 04 */,  C_DATAW0
  /* 05 */,  C_DATAW0
  /* 06 */,  0
  /* 07 */,  0
  /* 08 */,  C_MODRM
  /* 09 */,  C_MODRM
  /* 0A */,  C_MODRM
  /* 0B */,  C_MODRM
  /* 0C */,  C_DATAW0
  /* 0D */,  C_DATAW0
  /* 0E */,  0
  /* 0F */,  C_TABLE_0F
  /* 10 */,  C_MODRM
  /* 11 */,  C_MODRM
  /* 12 */,  C_MODRM
  /* 13 */,  C_MODRM
  /* 14 */,  C_DATAW0
  /* 15 */,  C_DATAW0
  /* 16 */,  0
  /* 17 */,  0
  /* 18 */,  C_MODRM
  /* 19 */,  C_MODRM
  /* 1A */,  C_MODRM
  /* 1B */,  C_MODRM
  /* 1C */,  C_DATAW0
  /* 1D */,  C_DATAW0
  /* 1E */,  0
  /* 1F */,  0
  /* 20 */,  C_MODRM
  /* 21 */,  C_MODRM
  /* 22 */,  C_MODRM
  /* 23 */,  C_MODRM
  /* 24 */,  C_DATAW0
  /* 25 */,  C_DATAW0
  /* 26 */,  C_PREFIX
  /* 27 */,  0
  /* 28 */,  C_MODRM
  /* 29 */,  C_MODRM
  /* 2A */,  C_MODRM
  /* 2B */,  C_MODRM
  /* 2C */,  C_DATAW0
  /* 2D */,  C_DATAW0
  /* 2E */,  C_PREFIX
  /* 2F */,  0
  /* 30 */,  C_MODRM
  /* 31 */,  C_MODRM
  /* 32 */,  C_MODRM
  /* 33 */,  C_MODRM
  /* 34 */,  C_DATAW0
  /* 35 */,  C_DATAW0
  /* 36 */,  C_PREFIX
  /* 37 */,  0
  /* 38 */,  C_MODRM
  /* 39 */,  C_MODRM
  /* 3A */,  C_MODRM
  /* 3B */,  C_MODRM
  /* 3C */,  C_DATAW0
  /* 3D */,  C_DATAW0
  /* 3E */,  C_PREFIX
  /* 3F */,  0
  /* 40 */,  0
  /* 41 */,  0
  /* 42 */,  0
  /* 43 */,  0
  /* 44 */,  0
  /* 45 */,  0
  /* 46 */,  0
  /* 47 */,  0
  /* 48 */,  0
  /* 49 */,  0
  /* 4A */,  0
  /* 4B */,  0
  /* 4C */,  0
  /* 4D */,  0
  /* 4E */,  0
  /* 4F */,  0
  /* 50 */,  0
  /* 51 */,  0
  /* 52 */,  0
  /* 53 */,  0
  /* 54 */,  0
  /* 55 */,  0
  /* 56 */,  0
  /* 57 */,  0
  /* 58 */,  0
  /* 59 */,  0
  /* 5A */,  0
  /* 5B */,  0
  /* 5C */,  0
  /* 5D */,  0
  /* 5E */,  0
  /* 5F */,  0
  /* 60 */,  0
  /* 61 */,  0
  /* 62 */,  C_MODRM
  /* 63 */,  C_MODRM
  /* 64 */,  C_PREFIX
  /* 65 */,  C_PREFIX
  /* 66 */,  C_PREFIX+C_66
  /* 67 */,  C_PREFIX+C_67
  /* 68 */,  C_DATA66
  /* 69 */,  C_MODRM+C_DATA66
  /* 6A */,  C_DATA1
  /* 6B */,  C_MODRM+C_DATA1
  /* 6C */,  0
  /* 6D */,  0
  /* 6E */,  0
  /* 6F */,  0
  /* 70 */,  C_DATA1
  /* 71 */,  C_DATA1
  /* 72 */,  C_DATA1
  /* 73 */,  C_DATA1
  /* 74 */,  C_DATA1
  /* 75 */,  C_DATA1
  /* 76 */,  C_DATA1
  /* 77 */,  C_DATA1
  /* 78 */,  C_DATA1
  /* 79 */,  C_DATA1
  /* 7A */,  C_DATA1
  /* 7B */,  C_DATA1
  /* 7C */,  C_DATA1
  /* 7D */,  C_DATA1
  /* 7E */,  C_DATA1
  /* 7F */,  C_DATA1
  /* 80 */,  C_MODRM+C_DATA1
  /* 81 */,  C_MODRM+C_DATA66
  /* 82 */,  C_MODRM+C_DATA1
  /* 83 */,  C_MODRM+C_DATA1
  /* 84 */,  C_MODRM
  /* 85 */,  C_MODRM
  /* 86 */,  C_MODRM
  /* 87 */,  C_MODRM
  /* 88 */,  C_MODRM
  /* 89 */,  C_MODRM
  /* 8A */,  C_MODRM
  /* 8B */,  C_MODRM
  /* 8C */,  C_MODRM
  /* 8D */,  C_MODRM
  /* 8E */,  C_MODRM
  /* 8F */,  C_MODRM
  /* 90 */,  0
  /* 91 */,  0
  /* 92 */,  0
  /* 93 */,  0
  /* 94 */,  0
  /* 95 */,  0
  /* 96 */,  0
  /* 97 */,  0
  /* 98 */,  0
  /* 99 */,  0
  /* 9A */,  C_DATA66+C_MEM2
  /* 9B */,  0
  /* 9C */,  0
  /* 9D */,  0
  /* 9E */,  0
  /* 9F */,  0
  /* A0 */,  C_MEM67
  /* A1 */,  C_MEM67
  /* A2 */,  C_MEM67
  /* A3 */,  C_MEM67
  /* A4 */,  0
  /* A5 */,  0
  /* A6 */,  0
  /* A7 */,  0
  /* A8 */,  C_DATA1
  /* A9 */,  C_DATA66
  /* AA */,  0
  /* AB */,  0
  /* AC */,  0
  /* AD */,  0
  /* AE */,  0
  /* AF */,  0
  /* B0 */,  C_DATA1
  /* B1 */,  C_DATA1
  /* B2 */,  C_DATA1
  /* B3 */,  C_DATA1
  /* B4 */,  C_DATA1
  /* B5 */,  C_DATA1
  /* B6 */,  C_DATA1
  /* B7 */,  C_DATA1
  /* B8 */,  C_DATA66
  /* B9 */,  C_DATA66
  /* BA */,  C_DATA66
  /* BB */,  C_DATA66
  /* BC */,  C_DATA66
  /* BD */,  C_DATA66
  /* BE */,  C_DATA66
  /* BF */,  C_DATA66
  /* C0 */,  C_MODRM+C_DATA1
  /* C1 */,  C_MODRM+C_DATA1
  /* C2 */,  C_DATA2
  /* C3 */,  0
  /* C4 */,  C_MODRM
  /* C5 */,  C_MODRM
  /* C6 */,  C_MODRM+C_DATA66
  /* C7 */,  C_MODRM+C_DATA66
  /* C8 */,  C_DATA2+C_DATA1
  /* C9 */,  0
  /* CA */,  C_DATA2
  /* CB */,  0
  /* CC */,  0
  /* CD */,  C_DATA1+C_DATA4
  /* CE */,  0
  /* CF */,  0
  /* D0 */,  C_MODRM
  /* D1 */,  C_MODRM
  /* D2 */,  C_MODRM
  /* D3 */,  C_MODRM
  /* D4 */,  0
  /* D5 */,  0
  /* D6 */,  0
  /* D7 */,  0
  /* D8 */,  C_MODRM
  /* D9 */,  C_MODRM
  /* DA */,  C_MODRM
  /* DB */,  C_MODRM
  /* DC */,  C_MODRM
  /* DD */,  C_MODRM
  /* DE */,  C_MODRM
  /* DF */,  C_MODRM
  /* E0 */,  C_DATA1
  /* E1 */,  C_DATA1
  /* E2 */,  C_DATA1
  /* E3 */,  C_DATA1
  /* E4 */,  C_DATA1
  /* E5 */,  C_DATA1
  /* E6 */,  C_DATA1
  /* E7 */,  C_DATA1
  /* E8 */,  C_DATA66
  /* E9 */,  C_DATA66
  /* EA */,  C_DATA66+C_MEM2
  /* EB */,  C_DATA1
  /* EC */,  0
  /* ED */,  0
  /* EE */,  0
  /* EF */,  0
  /* F0 */,  C_PREFIX
  /* F1 */,  0                       // 0xF1
  /* F2 */,  C_PREFIX
  /* F3 */,  C_PREFIX
  /* F4 */,  0
  /* F5 */,  0
  /* F6 */,  C_FUCKINGTEST
  /* F7 */,  C_FUCKINGTEST
  /* F8 */,  0
  /* F9 */,  0
  /* FA */,  0
  /* FB */,  0
  /* FC */,  0
  /* FD */,  0
  /* FE */,  C_MODRM
  /* FF */,  C_MODRM
}; // table_1

DWORD table_0F[256] =
{
  /* 00 */   C_MODRM
  /* 01 */,  C_MODRM
  /* 02 */,  C_MODRM
  /* 03 */,  C_MODRM
  /* 04 */,  C_ERROR
  /* 05 */,  C_ERROR
  /* 06 */,  0
  /* 07 */,  C_ERROR
  /* 08 */,  0
  /* 09 */,  0
  /* 0A */,  0
  /* 0B */,  0
  /* 0C */,  C_ERROR
  /* 0D */,  C_ERROR
  /* 0E */,  C_ERROR
  /* 0F */,  C_ERROR
  /* 10 */,  C_ERROR
  /* 11 */,  C_ERROR
  /* 12 */,  C_ERROR
  /* 13 */,  C_ERROR
  /* 14 */,  C_ERROR
  /* 15 */,  C_ERROR
  /* 16 */,  C_ERROR
  /* 17 */,  C_ERROR
  /* 18 */,  C_ERROR
  /* 19 */,  C_ERROR
  /* 1A */,  C_ERROR
  /* 1B */,  C_ERROR
  /* 1C */,  C_ERROR
  /* 1D */,  C_ERROR
  /* 1E */,  C_ERROR
  /* 1F */,  C_ERROR
  /* 20 */,  C_ERROR
  /* 21 */,  C_ERROR
  /* 22 */,  C_ERROR
  /* 23 */,  C_ERROR
  /* 24 */,  C_ERROR
  /* 25 */,  C_ERROR
  /* 26 */,  C_ERROR
  /* 27 */,  C_ERROR
  /* 28 */,  C_ERROR
  /* 29 */,  C_ERROR
  /* 2A */,  C_ERROR
  /* 2B */,  C_ERROR
  /* 2C */,  C_ERROR
  /* 2D */,  C_ERROR
  /* 2E */,  C_ERROR
  /* 2F */,  C_ERROR
  /* 30 */,  C_ERROR
  /* 31 */,  C_ERROR
  /* 32 */,  C_ERROR
  /* 33 */,  C_ERROR
  /* 34 */,  C_ERROR
  /* 35 */,  C_ERROR
  /* 36 */,  C_ERROR
  /* 37 */,  C_ERROR
  /* 38 */,  C_ERROR
  /* 39 */,  C_ERROR
  /* 3A */,  C_ERROR
  /* 3B */,  C_ERROR
  /* 3C */,  C_ERROR
  /* 3D */,  C_ERROR
  /* 3E */,  C_ERROR
  /* 3F */,  C_ERROR
  /* 40 */,  C_ERROR
  /* 41 */,  C_ERROR
  /* 42 */,  C_ERROR
  /* 43 */,  C_ERROR
  /* 44 */,  C_ERROR
  /* 45 */,  C_ERROR
  /* 46 */,  C_ERROR
  /* 47 */,  C_ERROR
  /* 48 */,  C_ERROR
  /* 49 */,  C_ERROR
  /* 4A */,  C_ERROR
  /* 4B */,  C_ERROR
  /* 4C */,  C_ERROR
  /* 4D */,  C_ERROR
  /* 4E */,  C_ERROR
  /* 4F */,  C_ERROR
  /* 50 */,  C_ERROR
  /* 51 */,  C_ERROR
  /* 52 */,  C_ERROR
  /* 53 */,  C_ERROR
  /* 54 */,  C_ERROR
  /* 55 */,  C_ERROR
  /* 56 */,  C_ERROR
  /* 57 */,  C_ERROR
  /* 58 */,  C_ERROR
  /* 59 */,  C_ERROR
  /* 5A */,  C_ERROR
  /* 5B */,  C_ERROR
  /* 5C */,  C_ERROR
  /* 5D */,  C_ERROR
  /* 5E */,  C_ERROR
  /* 5F */,  C_ERROR
  /* 60 */,  C_ERROR
  /* 61 */,  C_ERROR
  /* 62 */,  C_ERROR
  /* 63 */,  C_ERROR
  /* 64 */,  C_ERROR
  /* 65 */,  C_ERROR
  /* 66 */,  C_ERROR
  /* 67 */,  C_ERROR
  /* 68 */,  C_ERROR
  /* 69 */,  C_ERROR
  /* 6A */,  C_ERROR
  /* 6B */,  C_ERROR
  /* 6C */,  C_ERROR
  /* 6D */,  C_ERROR
  /* 6E */,  C_ERROR
  /* 6F */,  C_ERROR
  /* 70 */,  C_ERROR
  /* 71 */,  C_ERROR
  /* 72 */,  C_ERROR
  /* 73 */,  C_ERROR
  /* 74 */,  C_ERROR
  /* 75 */,  C_ERROR
  /* 76 */,  C_ERROR
  /* 77 */,  C_ERROR
  /* 78 */,  C_ERROR
  /* 79 */,  C_ERROR
  /* 7A */,  C_ERROR
  /* 7B */,  C_ERROR
  /* 7C */,  C_ERROR
  /* 7D */,  C_ERROR
  /* 7E */,  C_ERROR
  /* 7F */,  C_ERROR
  /* 80 */,  C_DATA66
  /* 81 */,  C_DATA66
  /* 82 */,  C_DATA66
  /* 83 */,  C_DATA66
  /* 84 */,  C_DATA66
  /* 85 */,  C_DATA66
  /* 86 */,  C_DATA66
  /* 87 */,  C_DATA66
  /* 88 */,  C_DATA66
  /* 89 */,  C_DATA66
  /* 8A */,  C_DATA66
  /* 8B */,  C_DATA66
  /* 8C */,  C_DATA66
  /* 8D */,  C_DATA66
  /* 8E */,  C_DATA66
  /* 8F */,  C_DATA66
  /* 90 */,  C_MODRM
  /* 91 */,  C_MODRM
  /* 92 */,  C_MODRM
  /* 93 */,  C_MODRM
  /* 94 */,  C_MODRM
  /* 95 */,  C_MODRM
  /* 96 */,  C_MODRM
  /* 97 */,  C_MODRM
  /* 98 */,  C_MODRM
  /* 99 */,  C_MODRM
  /* 9A */,  C_MODRM
  /* 9B */,  C_MODRM
  /* 9C */,  C_MODRM
  /* 9D */,  C_MODRM
  /* 9E */,  C_MODRM
  /* 9F */,  C_MODRM
  /* A0 */,  0
  /* A1 */,  0
  /* A2 */,  0
  /* A3 */,  C_MODRM
  /* A4 */,  C_MODRM+C_DATA1
  /* A5 */,  C_MODRM
  /* A6 */,  C_ERROR
  /* A7 */,  C_ERROR
  /* A8 */,  0
  /* A9 */,  0
  /* AA */,  0
  /* AB */,  C_MODRM
  /* AC */,  C_MODRM+C_DATA1
  /* AD */,  C_MODRM
  /* AE */,  C_ERROR
  /* AF */,  C_MODRM
  /* B0 */,  C_MODRM
  /* B1 */,  C_MODRM
  /* B2 */,  C_MODRM
  /* B3 */,  C_MODRM
  /* B4 */,  C_MODRM
  /* B5 */,  C_MODRM
  /* B6 */,  C_MODRM
  /* B7 */,  C_MODRM
  /* B8 */,  C_ERROR
  /* B9 */,  C_ERROR
  /* BA */,  C_MODRM+C_DATA1
  /* BB */,  C_MODRM
  /* BC */,  C_MODRM
  /* BD */,  C_MODRM
  /* BE */,  C_MODRM
  /* BF */,  C_MODRM
  /* C0 */,  C_MODRM
  /* C1 */,  C_MODRM
  /* C2 */,  C_ERROR
  /* C3 */,  C_ERROR
  /* C4 */,  C_ERROR
  /* C5 */,  C_ERROR
  /* C6 */,  C_ERROR
  /* C7 */,  C_ERROR
  /* C8 */,  0
  /* C9 */,  0
  /* CA */,  0
  /* CB */,  0
  /* CC */,  0
  /* CD */,  0
  /* CE */,  0
  /* CF */,  0
  /* D0 */,  C_ERROR
  /* D1 */,  C_ERROR
  /* D2 */,  C_ERROR
  /* D3 */,  C_ERROR
  /* D4 */,  C_ERROR
  /* D5 */,  C_ERROR
  /* D6 */,  C_ERROR
  /* D7 */,  C_ERROR
  /* D8 */,  C_ERROR
  /* D9 */,  C_ERROR
  /* DA */,  C_ERROR
  /* DB */,  C_ERROR
  /* DC */,  C_ERROR
  /* DD */,  C_ERROR
  /* DE */,  C_ERROR
  /* DF */,  C_ERROR
  /* E0 */,  C_ERROR
  /* E1 */,  C_ERROR
  /* E2 */,  C_ERROR
  /* E3 */,  C_ERROR
  /* E4 */,  C_ERROR
  /* E5 */,  C_ERROR
  /* E6 */,  C_ERROR
  /* E7 */,  C_ERROR
  /* E8 */,  C_ERROR
  /* E9 */,  C_ERROR
  /* EA */,  C_ERROR
  /* EB */,  C_ERROR
  /* EC */,  C_ERROR
  /* ED */,  C_ERROR
  /* EE */,  C_ERROR
  /* EF */,  C_ERROR
  /* F0 */,  C_ERROR
  /* F1 */,  C_ERROR
  /* F2 */,  C_ERROR
  /* F3 */,  C_ERROR
  /* F4 */,  C_ERROR
  /* F5 */,  C_ERROR
  /* F6 */,  C_ERROR
  /* F7 */,  C_ERROR
  /* F8 */,  C_ERROR
  /* F9 */,  C_ERROR
  /* FA */,  C_ERROR
  /* FB */,  C_ERROR
  /* FC */,  C_ERROR
  /* FD */,  C_ERROR
  /* FE */,  C_ERROR
  /* FF */,  C_ERROR
};

// ==============================
// DisAssemble
//
// Disassembles an address in order to find the opcodes.
// ==============================

void DisAssemble( unsigned char *iptr0, size_t *osizeptr )
{
	unsigned char	*iptr = iptr0;
	unsigned char	 mod, rm, b;
	unsigned int	 f = 0;

prefix: 

	b  = *iptr++;
	f |= table_1[b];

	if ( f & C_FUCKINGTEST )
	{
		if ((( *iptr ) &0x38 ) == 0x00 )
		{
			f = C_MODRM + C_DATAW0;
		}
		else
		{
			f = C_MODRM;
		}
	}

	if ( f &C_TABLE_0F )
	{
		b = *iptr++;
		f = table_0F[b];
	}

	if (f == C_ERROR)
	{
		*osizeptr = C_ERROR;
		return;
	}

	if ( f & C_PREFIX )
	{
		f &= ~C_PREFIX;
		goto prefix;
	}

	if ( f & C_DATAW0 )
	{
		if ( b & 0x01 )
		{
			f |= C_DATA66;
		}
		else
		{
			f |= C_DATA1;
		}
	}

	if ( f &C_MODRM )
	{
		b	=  *iptr++;
		mod	= b &0xC0;
		rm	= b &0x07;

		if ( mod != 0xC0 )
		{
			// modrm16
			if ( f &C_67 )
			{
				if (( mod == 0x00 ) && ( rm == 0x06 ))
				{
					f |= C_MEM2;
				}

				if ( mod == 0x40 )
				{
					f |= C_MEM1;
				}

				if ( mod == 0x80 )
				{
					f |= C_MEM2;
				}
			}
			// modrm32
			else
			{
				if ( mod == 0x40 )
				{
					f |= C_MEM1;
				}

				if (mod == 0x80)
				{
					f |= C_MEM4;
				}

				if (rm == 0x04)
				{
					rm = (*iptr++) &0x07;
				}

				// rm<-sib.base
				if (( rm == 0x05 ) && ( mod == 0x00 ))
				{
					f |= C_MEM4;
				}
			}
		}
	}

	if ( f & C_MEM67 )
	{
		if (f & C_67 )
		{
			f |= C_MEM2;
		}
		else
		{
			f |= C_MEM4;
		}
	}

	if ( f & C_DATA66 )
	{
		if ( f & C_66 )
		{
			f |= C_DATA2;
		}
		else
		{
			f |= C_DATA4;
		}
	}

	if ( f & C_MEM1 )
	{
		iptr++;
	}

	if ( f &C_MEM2 )
	{
		iptr += 2;
	}

	if ( f & C_MEM4 )
	{
		iptr += 4;
	}

	if ( f & C_DATA1 )
	{
		iptr++;
	}
	if ( f & C_DATA2 )
	{
		iptr += 2;
	}
	if ( f & C_DATA4 )
	{
		iptr += 4;
	}

	*osizeptr = ( size_t )( iptr - iptr0 );
}
