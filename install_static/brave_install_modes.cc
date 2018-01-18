/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

// Brand-specific constants and install modes for Brave.

#include <stdlib.h>

#include "brave/common/brave_icon_resources_win.h"
#include "chrome/app/chrome_dll_resource.h"
#include "chrome/install_static/install_modes.h"

namespace install_static {

const wchar_t kCompanyPathName[] = L"BraveSoftware";

const wchar_t kProductPathName[] = L"Brave";

const size_t kProductPathNameLength = _countof(kProductPathName) - 1;

const wchar_t kBinariesAppGuid[] = L"{F7526127-0B8A-406F-8998-282BEA40103A}";

// Brave integrates with Brave Update, so the app GUID above is used.
const wchar_t kBinariesPathName[] = L"";

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
        "brave-beta",  // Install switch.
        L" Beta",       // Install suffix.
        L"Beta",        // Logo suffix.
        L"{103BD053-949B-43A8-9120-2E424887DE11}",  // A distinct app GUID.
        L"Brave Beta",                      // A distinct base_app_name.
        L"BraveBeta",                              // A distinct base_app_id.
        L"BraveHTML",                             // ProgID prefix.
        L"Brave Beta HTML Document",               // ProgID description.
        L"{103BD053-949B-43A8-9120-2E424887DE11}",  // Active Setup GUID.
        L"",                                        // CommandExecuteImpl CLSID.
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
        "brave-dev",  // Install switch.
        L" Dev",       // Install suffix.
        L"Dev",        // Logo suffix.
        L"{CB2150F2-595F-4633-891A-E39720CE0531}",  // A distinct app GUID.
        L"Brave Dev",                       // A distinct base_app_name.
        L"BraveDev",                               // A distinct base_app_id.
        L"BraveDHTML",                             // ProgID prefix.
        L"Brave Dev HTML Document",                // ProgID description.
        L"{CB2150F2-595F-4633-891A-E39720CE0531}",  // Active Setup GUID.
        L"",                                        // CommandExecuteImpl CLSID.
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
        "brave-sxs",  // Install switch.
        L" SxS",       // Install suffix.
        L"Canary",     // Logo suffix.
        L"{C6CB981E-DB30-4876-8639-109F8933582C}",  // A distinct app GUID.
        L"Brave Canary",                    // A distinct base_app_name.
        L"BraveCanary",                            // A distinct base_app_id.
        L"BraveSSHTM",                             // ProgID prefix.
        L"Brave Canary HTML Document",             // ProgID description.
        L"{C6CB981E-DB30-4876-8639-109F8933582C}",  // Active Setup GUID.
        L"{312ABB99-A176-4939-A39F-E8D34EA4D393}",  // CommandExecuteImpl CLSID.
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

static_assert(_countof(kInstallModes) == NUM_INSTALL_MODES,
              "Imbalance between kInstallModes and InstallConstantIndex");

}  // namespace install_static
