/**
 * @file        src/hooks.cpp
 *
 * @brief       Function hooks and replacements for guest module runtime.
 *
 * @copyright   Copyright (c) 2026 Tom Clay <tomc@tctechstuff.com>
 *              All rights reserved.
 *
 * @license     BSD 3-Clause License
 *              See LICENSE file in the project root for full license text.
 */

#include "generated/Kameo_init.h"

#include <atomic>
#include <chrono>

#include <rex/hook.h>
#include <rex/logging.h>
#include <rex/ppc.h>

// Forward decls for the original recompiled bodies, so the bypass hooks below
// can fall through when their NULL precondition does not hold. Spelled with
// REX_EXTERN (which expands to `extern "C" REX_FUNC(name)`) to match the
// reblue / rexglue hook authoring style.
REX_EXTERN(__imp__sub_820D2E10);
REX_EXTERN(__imp__sub_820D4BA0);
REX_EXTERN(__imp__sub_825F8A50);


//=============================================================================
// NULL-deref bypasses (load-bearing)
//=============================================================================
// Without these, several Kameo worker threads crash on a NULL ptr deref the
// moment they pick up their first job — producing 1.4 M SEGV/sec. The
// bypasses skip the NULL path with a short warning the first few times so
// the engine can keep moving.

// sub_820D2E10 dereferences a global pointer chain that's NULL during early
// init: r11 = *(0x8200050C) [or *(0x82000730) if KeGetCurrentProcessType==2],
// r31 = *r11, then `lwz r11, 10376(r31)` faults at host_addr + 0x2888 when
// r31 is NULL. Bypass when both candidate chain roots dereference to NULL.
REX_HOOK_RAW(sub_820D2E10) {
    uint32_t addr1 = REX_LOAD_U32(0x8200050C);  // r28 (0x82000000) + 1292
    uint32_t addr2 = REX_LOAD_U32(0x82000000 + 1840);  // r27 (0x82000000) + 1840
    uint32_t deref1 = (addr1 != 0) ? REX_LOAD_U32(addr1 + 0) : 0;
    uint32_t deref2 = (addr2 != 0) ? REX_LOAD_U32(addr2 + 0) : 0;
    if (deref1 == 0 && deref2 == 0) {
        static std::atomic<int> warned{0};
        if (warned.fetch_add(1) < 4) {
            REXKRNL_WARN("sub_820D2E10: both chain roots NULL (*0x8200050C={:#010x}, "
                         "*0x82000730={:#010x}), returning r3=0", addr1, addr2);
        }
        ctx.r3.u64 = 0;
        return;
    }
    __imp__sub_820D2E10(ctx, base);
}

// sub_825F8A50 dereferences r6.[0x2C] (a pointer field). When that's NULL the
// body's `lwz r11, 0(r3)` loads from host_addr + 0 and faults. Caller's
// `mr. r28,r3 / blt` treats negative as failure — return -1.
REX_HOOK_RAW(sub_825F8A50) {
    uint32_t r6 = ctx.r6.u32;
    if (r6 != 0 && REX_LOAD_U32(r6 + 0x2C) == 0) {
        static std::atomic<int> warned{0};
        if (warned.fetch_add(1) < 4) {
            REXKRNL_WARN("sub_825F8A50: r6={:#010x} field_0x2C=NULL, returning -1", r6);
        }
        ctx.r3.u64 = static_cast<uint64_t>(static_cast<int64_t>(-1));
        return;
    }
    __imp__sub_825F8A50(ctx, base);
}

// sub_820D4BA0 dereferences r3.[12948]; same pattern. Caller checks for
// non-zero return — returning 0 sends it down the "no result" branch.
REX_HOOK_RAW(sub_820D4BA0) {
    uint32_t r3 = ctx.r3.u32;
    if (r3 != 0 && REX_LOAD_U32(r3 + 12948) == 0) {
        static std::atomic<int> warned{0};
        if (warned.fetch_add(1) < 8) {
            REXKRNL_WARN("sub_820D4BA0: r3={:#010x} field_12948=NULL, returning 0", r3);
        }
        ctx.r3.u64 = 0;
        return;
    }
    __imp__sub_820D4BA0(ctx, base);
}

//=============================================================================
// Missing-export stubs
//=============================================================================
// Kernel / XAM exports Kameo references but that ReXGlue does not implement.
// Each is a no-op so the module links; promote to a real implementation as
// the title needs it.

PPC_STUB(__imp__XamShowGamerCardUIForXUID);
PPC_STUB(__imp__XamShowAchievementsUI);
PPC_STUB(__imp__XamShowMarketplaceUI);
PPC_STUB(__imp__IoInvalidDeviceRequest);
PPC_STUB(__imp__ObReferenceObject);
PPC_STUB(__imp__IoDeleteDevice);
PPC_STUB(__imp__IoCompleteRequest);
PPC_STUB(__imp__RtlUpcaseUnicodeChar);
PPC_STUB(__imp__ObIsTitleObject);
PPC_STUB(__imp__IoCheckShareAccess);
PPC_STUB(__imp__IoRemoveShareAccess);
PPC_STUB(__imp__XNetLogonGetTitleID);
PPC_STUB(__imp__NetDll_WSAGetOverlappedResult);
PPC_STUB(__imp__NetDll_XNetRegisterKey);
PPC_STUB(__imp__NetDll_XNetUnregisterKey);
PPC_STUB(__imp__NetDll_XNetUnregisterInAddr);
PPC_STUB(__imp__NetDll_XNetConnect);
PPC_STUB(__imp__NetDll_XNetGetConnectStatus);
PPC_STUB(__imp__IoSetShareAccess);
PPC_STUB(__imp__XamUserGetMembershipTierFromXUID);
PPC_STUB(__imp__XamUserGetOnlineCountryFromXUID);
PPC_STUB(__imp__XamVoiceSubmitPacket);
