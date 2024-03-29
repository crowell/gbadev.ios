
/* Code from postloader - thanks to stfour */

#include <gccore.h> 
#include <ogc/machine/processor.h>
#include <string.h>
#include <stdio.h>
#include "identify.h"

static bool apply_patch(char *name, const u8 *old, const u8 *patch, u32 size, bool enable)
{
	if(!enable)
	{
		const u8 *n = old;
		old = patch;
		patch = n;
	}
	u8 *ptr = (u8*)0x93400000;
	u32 found = 0;
	while((u32)ptr < (0x94000000 - size))
	{
		if(memcmp(ptr, old, size) == 0)
		{
			found++;
			u32 i;
			for(i = 0; i < size; i++)
				ptr[i] = patch[i];
			DCFlushRange(ptr, size);
		}
		ptr++;
	}
	printf("patched %s %u times.\n", name, found);
	return (found > 0);
}

const u8 isfs_perm_old[] = { 0x42, 0x8B, 0xD0, 0x01, 0x25, 0x66 };
const u8 isfs_perm_patch[] = { 0x42, 0x8B, 0xE0, 0x01, 0x25, 0x66 };
const u8 setuid_old[] = { 0xD1, 0x2A, 0x1C, 0x39 };
const u8 setuid_patch[] = { 0x46, 0xC0, 0x1C, 0x39 };
const u8 es_identify_old[] = { 0x28, 0x03, 0xD1, 0x23 };
const u8 es_identify_patch[] = { 0x28, 0x03, 0x00, 0x00 };
const u8 hash_old[] = { 0x20, 0x07, 0x23, 0xA2 };
const u8 hash_patch[] = { 0x20, 0x00, 0x23, 0xA2 };
const u8 new_hash_old[] = { 0x20, 0x07, 0x4B, 0x0B };
const u8 new_hash_patch[] = { 0x20, 0x00, 0x4B, 0x0B };

/* vWii Specific */
const u8 Kill_AntiSysTitleInstallv3_pt1_old[] = { 0x68, 0x1A, 0x2A, 0x01, 0xD0, 0x05 };    // Make sure that the pt1
const u8 Kill_AntiSysTitleInstallv3_pt1_patch[] = { 0x68, 0x1A, 0x2A, 0x01, 0x46, 0xC0 };    // patch is applied twice. -dmm
const u8 Kill_AntiSysTitleInstallv3_pt2_old[] = { 0xD0, 0x02, 0x33, 0x06, 0x42, 0x9A, 0xD1, 0x01 };    // Make sure that the pt2 patch
const u8 Kill_AntiSysTitleInstallv3_pt2_patch[] = { 0x46, 0xC0, 0x33, 0x06, 0x42, 0x9A, 0xE0, 0x01 };    // is also applied twice. -dmm
const u8 Kill_AntiSysTitleInstallv3_pt3_old[] = { 0x68, 0xFB, 0x2B, 0x00, 0xDB, 0x01 };
const u8 Kill_AntiSysTitleInstallv3_pt3_patch[] = { 0x68, 0xFB, 0x2B, 0x00, 0xDB, 0x10 };

void PatchIOS(bool enable)
{
	/* Disable memory protection */
	write16((vu32)0xCD8B420A, 0);
	/* Do Patching */
	apply_patch("isfs_permissions", isfs_perm_old, isfs_perm_patch, sizeof(isfs_perm_patch), enable);
	apply_patch("es_setuid", setuid_old, setuid_patch, sizeof(setuid_patch), enable);
	apply_patch("es_identify", es_identify_old, es_identify_patch, sizeof(es_identify_patch), enable);
	apply_patch("hash_check", hash_old, hash_patch, sizeof(hash_patch), enable);
	apply_patch("new_hash_check", new_hash_old, new_hash_patch, sizeof(new_hash_patch), enable);
	/* vWii Specific */
	apply_patch("vwii_patch_ptr1", Kill_AntiSysTitleInstallv3_pt1_old, Kill_AntiSysTitleInstallv3_pt1_patch, sizeof(Kill_AntiSysTitleInstallv3_pt1_patch), enable);
	apply_patch("vwii_patch_ptr2", Kill_AntiSysTitleInstallv3_pt2_old, Kill_AntiSysTitleInstallv3_pt2_patch, sizeof(Kill_AntiSysTitleInstallv3_pt2_patch), enable);
	apply_patch("vwii_patch_ptr3", Kill_AntiSysTitleInstallv3_pt3_old, Kill_AntiSysTitleInstallv3_pt3_patch, sizeof(Kill_AntiSysTitleInstallv3_pt3_patch), enable);
	/* Enable memory protection */
	write16((vu32)0xCD8B420A, 1);
}


/* Thanks to postloader for that patch */
#define ES_MODULE_START	(u16*)0x939F0000

static const u16 ticket_check[] = {
    0x685B,          // ldr r3,[r3,#4] ; get TMD pointer
    0x22EC, 0x0052,  // movls r2, 0x1D8
    0x189B,          // adds r3, r3, r2; add offset of access rights field in TMD
    0x681B,          // ldr r3, [r3]   ; load access rights (haxxme!)
    0x4698,          // mov r8, r3     ; store it for the DVD video bitcheck later
    0x07DB           // lsls r3, r3, #31; check AHBPROT bit
};

void PatchAHB()
{
	u16 *patchme = NULL;
	for(patchme = ES_MODULE_START; patchme < ES_MODULE_START + 0x4000; patchme++) 
	{
		if(!memcmp(patchme, ticket_check, sizeof(ticket_check))) 
		{
			// write16/uncached poke doesn't work for this. Go figure.
			patchme[4] = 0x23FF; // li r3, 0xFF
			DCFlushRange(patchme + 4, 2);
			break;
		}
	}
}

void Patch_AHB()
{
	// Disable memory protection
	write16((vu32)0xCD8B420A, 0);
	// Do patches
	PatchAHB();
	// Enable memory protection
	write16((vu32)0xCD8B420A, 1);
}
