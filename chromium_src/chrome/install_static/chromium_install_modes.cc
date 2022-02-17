/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

// Brand-specific constants and install modes for Brave.

#include <stdlib.h>

#include "chrome/common/chrome_icon_resources_win.h"
#include "chrome/app/chrome_dll_resource.h"
#include "chrome/install_static/install_modes.h"

namespace install_static {

const wchar_t kCompanyPathName[] = L"BraveSoftware";

#if defined(OFFICIAL_BUILD)
const wchar_t kProductPathName[] = L"Brave-Browser";
#else
const wchar_t kProductPathName[] = L"Brave-Browser-Development";
#endif

const size_t kProductPathNameLength = _countof(kProductPathName) - 1;

const char kSafeBrowsingName[] = "chromium";

const char kDeviceManagementServerHostName[] = "";

#if defined(OFFICIAL_BUILD)
// Regarding to install switch, use same value in
// chrome/installer/mini_installer/configuration.cc
const InstallConstants kInstallModes[] = {
    // The primary install mode for stable Brave.
    {
        sizeof(kInstallModes[0]),
        STABLE_INDEX,  // The first mode is for stable/beta/dev.
        "",            // No install switch for the primary install mode.
        L"",           // Empty install_suffix for the primary install mode.
        L"",           // No logo suffix for the primary install mode.
        L"{AFE6A462-C574-4B8A-AF43-4CC60DF4563B}",
        L"Brave",                                   // A distinct base_app_name.
        L"Brave",                                   // A distinct base_app_id.
        L"BraveHTML",                               // ProgID prefix.
        L"Brave HTML Document",                     // ProgID description.
        L"{AFE6A462-C574-4B8A-AF43-4CC60DF4563B}",  // Active Setup GUID.
        L"{B1C5AAC5-2B7C-4C9D-9E96-774C53151B20}",  // CommandExecuteImpl CLSID.
        {0x6c9646d,
         0x2807,
         0x44c0,
         {0x97, 0xd2, 0x6d, 0xa0, 0xdb, 0x62, 0x3d,
          0xb4}},  // Toast activator CLSID.
        {0x576b31af,
         0x6369,
         0x4b6b,
         {0x85, 0x60, 0xe4, 0xb2, 0x3, 0xa9, 0x7a, 0x8b}},  // Elevator CLSID.
        {0xb7965c30,
         0x7d58,
         0x4d86,
         {0x9e, 0x18, 0x47, 0x94, 0x25, 0x64, 0x9, 0xee}},
        L"",  // The empty string means "stable".
        ChannelStrategy::FLOATING,
        true,  // Supports system-level installs.
        true,  // Supports in-product set as default browser UX.
        true,  // Supports retention experiments.
        icon_resources::kApplicationIndex,  // App icon resource index.
        IDR_MAINFRAME,                      // App icon resource id.
        L"S-1-15-2-3251537155-1984446955-2931258699-841473695-1938553385-"
        L"934012149-",  // App container sid prefix for sandbox.
    },
    // A secondary install mode for Brave Beta
    {
        sizeof(kInstallModes[0]),
        BETA_INDEX,     // The mode for the side-by-side beta channel.
        "chrome-beta",  // Install switch.
        L"-Beta",       // Install suffix.
        L"Beta",        // Logo suffix.
        L"{103BD053-949B-43A8-9120-2E424887DE11}",  // A distinct app GUID.
        L"Brave Beta",                              // A distinct base_app_name.
        L"BraveBeta",                               // A distinct base_app_id.
        L"BraveBHTML",                              // ProgID prefix.
        L"Brave Beta HTML Document",                // ProgID description.
        L"{103BD053-949B-43A8-9120-2E424887DE11}",  // Active Setup GUID.
        L"",                                        // CommandExecuteImpl CLSID.
        {0x9560028d,
         0xcca,
         0x49f0,
         {0x8d, 0x47, 0xef, 0x22, 0xbb, 0xc4, 0xb,
          0xa7}},  // Toast activator CLSID.
        {0x2313f1cd,
         0x41f3,
         0x4347,
         {0xbe, 0xc0, 0xd7, 0x22, 0xca, 0x41, 0x2c, 0x75}},  // Elevator CLSID.
        {0xd9d7b102,
         0xfc8a,
         0x4843,
         {0xac, 0x35, 0x1e, 0xbc, 0xc7, 0xed, 0x12, 0x3d}},
        L"beta",  // Forced channel name.
        ChannelStrategy::FIXED,
        true,  // Supports system-level installs.
        true,  // Supports in-product set as default browser UX.
        true,  // Supports retention experiments.
        icon_resources::kBetaApplicationIndex,  // App icon resource index.
        IDR_X005_BETA,                          // App icon resource id.
        L"S-1-15-2-3251537155-1984446955-2931258699-841473695-1938553385-"
        L"934012150-",  // App container sid prefix for sandbox.
    },
    // A secondary install mode for Brave Dev
    {
        sizeof(kInstallModes[0]),
        DEV_INDEX,     // The mode for the side-by-side dev channel.
        "chrome-dev",  // Install switch.
        L"-Dev",       // Install suffix.
        L"Dev",        // Logo suffix.
        L"{CB2150F2-595F-4633-891A-E39720CE0531}",  // A distinct app GUID.
        L"Brave Dev",                               // A distinct base_app_name.
        L"BraveDev",                                // A distinct base_app_id.
        L"BraveDHTML",                              // ProgID prefix.
        L"Brave Dev HTML Document",                 // ProgID description.
        L"{CB2150F2-595F-4633-891A-E39720CE0531}",  // Active Setup GUID.
        L"",                                        // CommandExecuteImpl CLSID.
        {0x20b22981,
         0xf63a,
         0x47a6,
         {0xa5, 0x47, 0x69, 0x1c, 0xc9, 0x4c, 0xae,
          0xe0}},  // Toast activator CLSID.
        {0x9129ed6a,
         0x11d3,
         0x43b7,
         {0xb7, 0x18, 0x8f, 0x82, 0x61, 0x45, 0x97, 0xa3}},  // Elevator CLSID.
        {0x9cf6868c,
         0x8c9f,
         0x4220,
         {0x95, 0xbe, 0x13, 0x99, 0x9d, 0xd9, 0x9b, 0x48}},
        L"dev",  // Forced channel name.
        ChannelStrategy::FIXED,
        true,  // Supports system-level installs.
        true,  // Supports in-product set as default browser UX.
        true,  // Supports retention experiments.
        icon_resources::kDevApplicationIndex,  // App icon resource index.
        IDR_X004_DEV,                          // App icon resource id.
        L"S-1-15-2-3251537155-1984446955-2931258699-841473695-1938553385-"
        L"934012151-",  // App container sid prefix for sandbox.
    },
    // A secondary install mode for Brave SxS (canary).
    {
        sizeof(kInstallModes[0]),
        NIGHTLY_INDEX,  // The mode for the side-by-side nightly channel.
        "chrome-sxs",   // Install switch.
        L"-Nightly",    // Install suffix.
        L"Canary",      // Logo suffix.
        L"{C6CB981E-DB30-4876-8639-109F8933582C}",  // A distinct app GUID.
        L"Brave Nightly",                           // A distinct base_app_name.
        L"BraveNightly",                            // A distinct base_app_id.
        L"BraveSSHTM",                              // ProgID prefix.
        L"Brave Nightly HTML Document",             // ProgID description.
        L"{C6CB981E-DB30-4876-8639-109F8933582C}",  // Active Setup GUID.
        L"{312ABB99-A176-4939-A39F-E8D34EA4D393}",  // CommandExecuteImpl CLSID.
        {0xf2edbc59,
         0x7217,
         0x4da5,
         {0xa2, 0x59, 0x3, 0x2, 0xda, 0x6a, 0x0,
          0xe1}},  // Toast activator CLSID.
        {0x1ce2f84f,
         0x70cb,
         0x4389,
         {0x87, 0xdb, 0xd0, 0x99, 0x48, 0x30, 0xbb, 0x17}},  // Elevator CLSID.
        {0x648b3c2b,
         0xe53,
         0x4085,
         {0xa9, 0x75, 0x11, 0x18, 0x1, 0x75, 0x8f, 0xe7}},
        L"nightly",  // Forced channel name.
        ChannelStrategy::FIXED,
        true,  // Support system-level installs.
        true,  // Support in-product set as default browser UX.
        true,  // Supports retention experiments.
        icon_resources::kSxSApplicationIndex,  // App icon resource index.
        IDR_SXS,                               // App icon resource id.
        L"S-1-15-2-3251537155-1984446955-2931258699-841473695-1938553385-"
        L"934012152-",  // App container sid prefix for sandbox.
    },
};
#else
const InstallConstants kInstallModes[] = {
    // The primary (and only) install mode for Brave developer build.
    {
        sizeof(kInstallModes[0]),
        DEVELOPER_INDEX,  // The one and only mode for developer mode.
        "",               // No install switch for the primary install mode.
        L"",              // Empty install_suffix for the primary install mode.
        L"",              // No logo suffix for the primary install mode.
        L"",            // Empty app_guid since no integraion with Brave Update.
        L"Brave Development",  // A distinct base_app_name.
        L"BraveDevelopment",   // A distinct base_app_id.
        L"BraveDevHTM",                             // ProgID prefix.
        L"Brave Development HTML Document",           // ProgID description.
        L"{D6527C63-5CDD-4EF3-9299-1504E17CBD18}",  // Active Setup GUID.
        L"{B2863926-AF5D-43A2-99CC-29EC43790C89}",  // CommandExecuteImpl CLSID.
        { 0xeb41c6e8,
          0xba35,
          0x4c06,
          { 0x96, 0xe8, 0x6f, 0x30, 0xf1, 0x8c, 0xa5,
            0x5c } },  // Toast activator CLSID.
        { 0x5693e62d,
          0xd6,
          0x4421,
          { 0xaf, 0xe8, 0x58, 0xf3, 0xc9, 0x47, 0x43,
            0x6a } },  // Elevator CLSID.
        { 0xedf6b466, 0x4efc, 0x4c88,
          { 0x83, 0x8c, 0x96, 0xb4, 0x39, 0x1c, 0x6a,
            0xe } },
        L"",    // Empty default channel name since no update integration.
        ChannelStrategy::UNSUPPORTED,
        true,   // Supports system-level installs.
        true,   // Supports in-product set as default browser UX.
        false,  // Does not support retention experiments.
        icon_resources::kApplicationIndex,  // App icon resource index.
        IDR_MAINFRAME,                      // App icon resource id.
        L"S-1-15-2-3251537155-1984446955-2931258699-841473695-1938553385-"
        L"934012148-",  // App container sid prefix for sandbox.
    },
};
#endif

static_assert(_countof(kInstallModes) == NUM_INSTALL_MODES,
              "Imbalance between kInstallModes and InstallConstantIndex");

}  // namespace install_static
