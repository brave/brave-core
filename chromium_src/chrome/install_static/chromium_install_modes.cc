/* This Source Code Form is subject to the terms of the Mozilla Public
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

#if defined(OFFICIAL_BUILD)
const wchar_t kBinariesAppGuid[] = L"{F7526127-0B8A-406F-8998-282BEA40103A}";
#else
const wchar_t kBinariesAppGuid[] = L"";
#endif

// Brave integrates with Brave Update, so the app GUID above is used.
const wchar_t kBinariesPathName[] = L"";

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
        L"Brave",                           // A distinct base_app_name.
        L"Brave",                           // A distinct base_app_id.
        L"BraveHTML",                              // ProgID prefix.
        L"Brave HTML Document",                    // ProgID description.
        L"{AFE6A462-C574-4B8A-AF43-4CC60DF4563B}",  // Active Setup GUID.
        L"{B1C5AAC5-2B7C-4C9D-9E96-774C53151B20}",  // CommandExecuteImpl CLSID.
        { 0x6c9646d,
          0x2807,
          0x44c0,
          { 0x97, 0xd2, 0x6d, 0xa0, 0xdb, 0x62, 0x3d,
            0xb4 } },  // Toast activator CLSID.
        L"",  // The empty string means "stable".
        ChannelStrategy::ADDITIONAL_PARAMETERS,
        true,  // Supports system-level installs.
        true,  // Supports in-product set as default browser UX.
        true,  // Supports retention experiments.
        true,  // Supported multi-install.
        icon_resources::kApplicationIndex,  // App icon resource index.
        IDR_MAINFRAME,                      // App icon resource id.
    },
    // A secondary install mode for Brave Beta
    {
        sizeof(kInstallModes[0]),
        BETA_INDEX,     // The mode for the side-by-side beta channel.
        "chrome-beta",  // Install switch.
        L"-Beta",       // Install suffix.
        L"Beta",        // Logo suffix.
        L"{103BD053-949B-43A8-9120-2E424887DE11}",  // A distinct app GUID.
        L"Brave Beta",                      // A distinct base_app_name.
        L"BraveBeta",                              // A distinct base_app_id.
        L"BraveBHTML",                             // ProgID prefix.
        L"Brave Beta HTML Document",               // ProgID description.
        L"{103BD053-949B-43A8-9120-2E424887DE11}",  // Active Setup GUID.
        L"",                                        // CommandExecuteImpl CLSID.
        { 0x9560028d,
          0xcca,
          0x49f0,
          { 0x8d, 0x47, 0xef, 0x22, 0xbb, 0xc4, 0xb,
            0xa7 } },  // Toast activator CLSID.
        L"beta",                                    // Forced channel name.
        ChannelStrategy::FIXED,
        true,   // Supports system-level installs.
        true,   // Supports in-product set as default browser UX.
        true,   // Supports retention experiments.
        false,  // Did not support multi-install.
        icon_resources::kBetaApplicationIndex,  // App icon resource index.
        IDR_X005_BETA,                          // App icon resource id.
    },
    // A secondary install mode for Brave Dev
    {
        sizeof(kInstallModes[0]),
        DEV_INDEX,     // The mode for the side-by-side dev channel.
        "chrome-dev",  // Install switch.
        L"-Dev",       // Install suffix.
        L"Dev",        // Logo suffix.
        L"{CB2150F2-595F-4633-891A-E39720CE0531}",  // A distinct app GUID.
        L"Brave Dev",                       // A distinct base_app_name.
        L"BraveDev",                               // A distinct base_app_id.
        L"BraveDHTML",                             // ProgID prefix.
        L"Brave Dev HTML Document",                // ProgID description.
        L"{CB2150F2-595F-4633-891A-E39720CE0531}",  // Active Setup GUID.
        L"",                                        // CommandExecuteImpl CLSID.
        { 0x20b22981,
          0xf63a,
          0x47a6,
          { 0xa5, 0x47, 0x69, 0x1c, 0xc9, 0x4c, 0xae,
            0xe0 } },  // Toast activator CLSID.
        L"dev",                                     // Forced channel name.
        ChannelStrategy::FIXED,
        true,   // Supports system-level installs.
        true,   // Supports in-product set as default browser UX.
        true,   // Supports retention experiments.
        false,  // Did not support multi-install.
        icon_resources::kDevApplicationIndex,  // App icon resource index.
        IDR_X004_DEV,                          // App icon resource id.
    },
    // A secondary install mode for Brave SxS (canary).
    {
        sizeof(kInstallModes[0]),
        CANARY_INDEX,  // The mode for the side-by-side canary channel.
        "chrome-sxs",  // Install switch.
        L"-Canary",       // Install suffix.
        L"Canary",     // Logo suffix.
        L"{C6CB981E-DB30-4876-8639-109F8933582C}",  // A distinct app GUID.
        L"Brave Canary",                    // A distinct base_app_name.
        L"BraveCanary",                            // A distinct base_app_id.
        L"BraveSSHTM",                             // ProgID prefix.
        L"Brave Canary HTML Document",             // ProgID description.
        L"{C6CB981E-DB30-4876-8639-109F8933582C}",  // Active Setup GUID.
        L"{312ABB99-A176-4939-A39F-E8D34EA4D393}",  // CommandExecuteImpl CLSID.
        { 0xf2edbc59,
          0x7217,
          0x4da5,
          { 0xa2, 0x59, 0x3, 0x2, 0xda, 0x6a, 0x0,
            0xe1 } },  // Toast activator CLSID.
        L"canary",                                  // Forced channel name.
        ChannelStrategy::FIXED,
        false,  // Does not support system-level installs.
        false,  // Does not support in-product set as default browser UX.
        true,   // Supports retention experiments.
        false,  // Did not support multi-install.
        icon_resources::kSxSApplicationIndex,  // App icon resource index.
        IDR_SXS,                               // App icon resource id.
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
        L"",              // Empty app_guid since no integraion with Brave Update.
        L"Brave Developer",  // A distinct base_app_name.
        L"BraveDeveloper",   // A distinct base_app_id.
        L"BraveDevHTM",                             // ProgID prefix.
        L"Brave Developer HTML Document",           // ProgID description.
        L"{D6527C63-5CDD-4EF3-9299-1504E17CBD18}",  // Active Setup GUID.
        L"{B2863926-AF5D-43A2-99CC-29EC43790C89}",  // CommandExecuteImpl CLSID.
        { 0xeb41c6e8,
          0xba35,
          0x4c06,
          { 0x96, 0xe8, 0x6f, 0x30, 0xf1, 0x8c, 0xa5,
            0x5c } },  // Toast activator CLSID.
        L"",    // Empty default channel name since no update integration.
        ChannelStrategy::UNSUPPORTED,
        true,   // Supports system-level installs.
        true,   // Supports in-product set as default browser UX.
        false,  // Does not support retention experiments.
        true,   // Supported multi-install.
        icon_resources::kApplicationIndex,  // App icon resource index.
        IDR_MAINFRAME,                      // App icon resource id.
    },
};
#endif

static_assert(_countof(kInstallModes) == NUM_INSTALL_MODES,
              "Imbalance between kInstallModes and InstallConstantIndex");

}  // namespace install_static
