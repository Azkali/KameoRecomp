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
#include <rex/ppc.h>
#include <rex/logging.h>

// Strategy 4 was tried and reverted: removing the NULL bypasses caused 1.4M
// SEGVs in 60s (NULL deref in sub_825F8A50 body, host page 0 unmapped → no
// recovery). The bypasses ARE load-bearing. Keeping them.
//
// Diagnostic confirmed by watchdog while bypasses were off: tid=7 stuck at
// lr=0x825F8A84 inside sub_825F8A50, post-wait on lock 0x34. Worker dequeues
// a job, calls sub_825F8A50, NULL-derefs, dies. The bypass dodges that.

extern "C" void __imp__sub_825F8A50(PPCContext& ctx, uint8_t* base);
extern "C" void __imp__sub_820D4BA0(PPCContext& ctx, uint8_t* base);
extern "C" void __imp__sub_820D2E10(PPCContext& ctx, uint8_t* base);

// sub_820D2E10 dereferences a global pointer chain that's NULL during early
// init: r11 = *(0x8200050C) [or *(0x82000730) if KeGetCurrentProcessType==2],
// r31 = *r11. The function then does `lwz r11, 10376(r31)` which faults at
// host_addr + 0x2888 (10376 = 0x2888) when r31=NULL. This used to be masked
// by the SIGFPE bug in vcfsx; with FP exceptions defensively re-masked in
// PPCContext::setcsr the SEGV is now exposed. Bypass when both candidate
// chain roots dereference to NULL.
extern "C" REX_FUNC(sub_820D2E10) {
    uint32_t addr1 = REX_LOAD_U32(0x8200050C);  // r28 (0x82000000) + 1292
    uint32_t addr2 = REX_LOAD_U32(0x82000000 + 1840);  // r27 (0x82000000) + 1840
    uint32_t deref1 = (addr1 != 0) ? REX_LOAD_U32(addr1 + 0) : 0;
    uint32_t deref2 = (addr2 != 0) ? REX_LOAD_U32(addr2 + 0) : 0;
    if (deref1 == 0 && deref2 == 0) {
        static std::atomic<int> warned{0};
        if (warned.fetch_add(1) < 4) {
            REXKRNL_WARN("sub_820D2E10: both chain roots NULL (*0x8200050C=0x{:08X}, "
                         "*0x82000730=0x{:08X}), returning r3=0", addr1, addr2);
        }
        ctx.r3.u64 = 0;
        return;
    }
    __imp__sub_820D2E10(ctx, base);
}

extern "C" REX_FUNC(sub_825F8A50) {
    uint32_t r6 = ctx.r6.u32;
    if (r6 != 0) {
        uint32_t field_2C = REX_LOAD_U32(r6 + 0x2C);
        if (field_2C == 0) {
            static std::atomic<int> warned{0};
            int n = warned.fetch_add(1);
            if (n < 4) {
                REXKRNL_WARN("sub_825F8A50: r6=0x{:08X} field_0x2C=NULL, returning -1", r6);
            }
            ctx.r3.u64 = static_cast<uint64_t>(static_cast<int64_t>(-1));
            return;
        }
    }
    __imp__sub_825F8A50(ctx, base);
}

extern "C" REX_FUNC(sub_820D4BA0) {
    uint32_t r3 = ctx.r3.u32;
    if (r3 != 0) {
        uint32_t field_12948 = REX_LOAD_U32(r3 + 12948);
        if (field_12948 == 0) {
            static std::atomic<int> warned{0};
            if (warned.fetch_add(1) < 8) {
                REXKRNL_WARN("sub_820D4BA0: r3=0x{:08X} field_12948=NULL, returning 0", r3);
            }
            ctx.r3.u64 = 0;
            return;
        }
    }
    __imp__sub_820D4BA0(ctx, base);
}

// Worker-pool init logger: logs each call to sub_822D4958 with r3 (pool name
// pointer), r4 (function pointer), and the dereferenced ASCII pool name. This
// identifies which subsystem owns the worker pool whose queue semaphore (e.g.
// 0xF80000C0) is permanently blocked.
extern "C" void __imp__sub_822D4958(PPCContext& ctx, uint8_t* base);
extern "C" REX_FUNC(sub_822D4958) {
    uint32_t r3 = ctx.r3.u32;
    uint32_t r4 = ctx.r4.u32;
    uint32_t r5 = ctx.r5.u32;
    uint32_t r6 = ctx.r6.u32;
    uint32_t lr = static_cast<uint32_t>(ctx.lr);

    char name_buf[64] = {0};
    if (r3 != 0) {
        for (size_t i = 0; i < sizeof(name_buf) - 1; ++i) {
            uint8_t c = REX_LOAD_U8(r3 + i);
            if (c == 0 || c < 0x20 || c > 0x7E) {
                name_buf[i] = 0;
                break;
            }
            name_buf[i] = static_cast<char>(c);
        }
    }
    REXKRNL_WARN("[POOL] sub_822D4958 lr={:#x} r3={:#x} ('{}') r4={:#x} r5={:#x} r6={:#x}",
                 lr, r3, name_buf, r4, r5, r6);

    __imp__sub_822D4958(ctx, base);
    REXKRNL_WARN("[POOL] sub_822D4958 returned r3={:#x}", ctx.r3.u32);
}

// Stubs for kernel imports that Kameo references but that are not yet
// implemented in rexglue. These return zero / no-op so the module can link.
// Promote individual stubs to real implementations as needed.
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
