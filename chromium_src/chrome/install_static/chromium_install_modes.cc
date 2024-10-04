/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

// Brand-specific constants and install modes for Brave.

#include <stdlib.h>

#include "chrome/app/chrome_dll_resource.h"
#include "chrome/common/chrome_icon_resources_win.h"
#include "chrome/install_static/install_modes.h"

namespace install_static {

const wchar_t kCompanyPathName[] = L"BraveSoftware";

#if defined(OFFICIAL_BUILD)
const wchar_t kProductPathName[] = L"Brave-Browser";
#else
const wchar_t kProductPathName[] = L"Brave-Browser-Development";
#endif

const size_t kProductPathNameLength = _countof(kProductPathName) - 1;

constexpr char kSafeBrowsingName[] = "chromium";

#if defined(OFFICIAL_BUILD)
// Regarding the install switch, use the same values that are in
// chrome/installer/mini_installer/configuration.cc
const InstallConstants kInstallModes[] = {
    // The primary install mode for stable Brave.
    {
        .size = sizeof(kInstallModes[0]),
        .index = STABLE_INDEX,  // The first mode is for stable/beta/dev.
        .install_switch =
            "",  // No install switch for the primary install mode.
        .install_suffix =
            L"",  // Empty install_suffix for the primary install mode.
        .logo_suffix = L"",  // No logo suffix for the primary install mode.
        .app_guid = L"{AFE6A462-C574-4B8A-AF43-4CC60DF4563B}",
        .base_app_name = L"Brave",               // A distinct base_app_name.
        .base_app_id = L"Brave",                 // A distinct base_app_id.
        .browser_prog_id_prefix = L"BraveHTML",  // Browser ProgID prefix.
        .browser_prog_id_description =
            L"Brave HTML Document",         // Browser ProgID description.
        .pdf_prog_id_prefix = L"BravePDF",  // PDF ProgID prefix.
        .pdf_prog_id_description =
            L"Brave PDF Document",  // PDF ProgID description.
        .active_setup_guid =
            L"{AFE6A462-C574-4B8A-AF43-4CC60DF4563B}",  // Active Setup GUID.
        .legacy_command_execute_clsid =
            L"{B1C5AAC5-2B7C-4C9D-9E96-774C53151B20}",  // CommandExecuteImpl
                                                        // CLSID.
        .toast_activator_clsid = {0x6c9646d,
                                  0x2807,
                                  0x44c0,
                                  {0x97, 0xd2, 0x6d, 0xa0, 0xdb, 0x62, 0x3d,
                                   0xb4}},  // Toast activator CLSID.
        .elevator_clsid = {0x576b31af,
                           0x6369,
                           0x4b6b,
                           {0x85, 0x60, 0xe4, 0xb2, 0x3, 0xa9, 0x7a,
                            0x8b}},  // Elevator CLSID.
        .elevator_iid = {0xf396861e,
                         0x0c8e,
                         0x4c71,
                         {0x82, 0x56, 0x2f, 0xae, 0x6d, 0x75, 0x9c, 0xe9}},
        .default_channel_name = L"",  // The empty string means "stable".
        .channel_strategy = ChannelStrategy::FLOATING,
        .supports_system_level = true,  // Supports system-level installs.
        .supports_set_as_default_browser =
            true,  // Supports in-product set as default browser UX.
        .app_icon_resource_index =
            icon_resources::kApplicationIndex,  // App icon resource index.
        .app_icon_resource_id = IDR_MAINFRAME,  // App icon resource id.
        .sandbox_sid_prefix =
            L"S-1-15-2-3251537155-1984446955-2931258699-841473695-1938553385-"
            L"934012149-",  // App container sid prefix for sandbox.
    },
    // A secondary install mode for Brave Beta
    {
        .size = sizeof(kInstallModes[0]),
        .index = BETA_INDEX,  // The mode for the side-by-side beta channel.
        .install_switch = "chrome-beta",  // Install switch.
        .install_suffix = L"-Beta",       // Install suffix.
        .logo_suffix = L"Beta",           // Logo suffix.
        .app_guid =
            L"{103BD053-949B-43A8-9120-2E424887DE11}",  // A distinct app GUID.
        .base_app_name = L"Brave Beta",           // A distinct base_app_name.
        .base_app_id = L"BraveBeta",              // A distinct base_app_id.
        .browser_prog_id_prefix = L"BraveBHTML",  // Browser ProgID prefix.
        .browser_prog_id_description =
            L"Brave Beta HTML Document",     // Browser ProgID description.
        .pdf_prog_id_prefix = L"BraveBPDF",  // PDF ProgID prefix.
        .pdf_prog_id_description =
            L"Brave Beta PDF Document",  // PDF ProgID description.
        .active_setup_guid =
            L"{103BD053-949B-43A8-9120-2E424887DE11}",  // Active Setup GUID.
        .legacy_command_execute_clsid = L"",  // CommandExecuteImpl CLSID.
        .toast_activator_clsid = {0x9560028d,
                                  0xcca,
                                  0x49f0,
                                  {0x8d, 0x47, 0xef, 0x22, 0xbb, 0xc4, 0xb,
                                   0xa7}},  // Toast activator CLSID.
        .elevator_clsid = {0x2313f1cd,
                           0x41f3,
                           0x4347,
                           {0xbe, 0xc0, 0xd7, 0x22, 0xca, 0x41, 0x2c,
                            0x75}},  // Elevator CLSID.
        .elevator_iid = {0x9ebad7ac,
                         0x6e1e,
                         0x4a1c,
                         {0xaa, 0x85, 0x1a, 0x70, 0xca, 0xda, 0x8d, 0x82}},
        .default_channel_name = L"beta",  // Forced channel name.
        .channel_strategy = ChannelStrategy::FIXED,
        .supports_system_level = true,  // Supports system-level installs.
        .supports_set_as_default_browser =
            true,  // Supports in-product set as default browser UX.
        .app_icon_resource_index =
            icon_resources::kBetaApplicationIndex,  // App icon resource index.
        .app_icon_resource_id = IDR_X005_BETA,      // App icon resource id.
        .sandbox_sid_prefix =
            L"S-1-15-2-3251537155-1984446955-2931258699-841473695-1938553385-"
            L"934012150-",  // App container sid prefix for sandbox.
    },
    // A secondary install mode for Brave Dev
    {
        .size = sizeof(kInstallModes[0]),
        .index = DEV_INDEX,  // The mode for the side-by-side dev channel.
        .install_switch = "chrome-dev",  // Install switch.
        .install_suffix = L"-Dev",       // Install suffix.
        .logo_suffix = L"Dev",           // Logo suffix.
        .app_guid =
            L"{CB2150F2-595F-4633-891A-E39720CE0531}",  // A distinct app GUID.
        .base_app_name = L"Brave Dev",            // A distinct base_app_name.
        .base_app_id = L"BraveDev",               // A distinct base_app_id.
        .browser_prog_id_prefix = L"BraveDHTML",  // Browser ProgID prefix.
        .browser_prog_id_description =
            L"Brave Dev HTML Document",      // Browser ProgID description.
        .pdf_prog_id_prefix = L"BraveDPDF",  // PDF ProgID prefix.
        .pdf_prog_id_description =
            L"Brave Dev PDF Document",  // PDF ProgID description.
        .active_setup_guid =
            L"{CB2150F2-595F-4633-891A-E39720CE0531}",  // Active Setup GUID.
        .legacy_command_execute_clsid = L"",  // CommandExecuteImpl CLSID.
        .toast_activator_clsid = {0x20b22981,
                                  0xf63a,
                                  0x47a6,
                                  {0xa5, 0x47, 0x69, 0x1c, 0xc9, 0x4c, 0xae,
                                   0xe0}},  // Toast activator CLSID.
        .elevator_clsid = {0x9129ed6a,
                           0x11d3,
                           0x43b7,
                           {0xb7, 0x18, 0x8f, 0x82, 0x61, 0x45, 0x97,
                            0xa3}},  // Elevator CLSID.
        .elevator_iid = {0x1e43c77b,
                         0x48e6,
                         0x4a4c,
                         {0x9d, 0xb2, 0xc2, 0x97, 0x17, 0x06, 0xc2, 0x55}},
        .default_channel_name = L"dev",  // Forced channel name.
        .channel_strategy = ChannelStrategy::FIXED,
        .supports_system_level = true,  // Supports system-level installs.
        .supports_set_as_default_browser =
            true,  // Supports in-product set as default browser UX.
        .app_icon_resource_index =
            icon_resources::kDevApplicationIndex,  // App icon resource index.
        .app_icon_resource_id = IDR_X004_DEV,      // App icon resource id.
        .sandbox_sid_prefix =
            L"S-1-15-2-3251537155-1984446955-2931258699-841473695-1938553385-"
            L"934012151-",  // App container sid prefix for sandbox.
    },
    // A secondary install mode for Brave SxS (canary).
    {
        .size = sizeof(kInstallModes[0]),
        .index =
            NIGHTLY_INDEX,  // The mode for the side-by-side nightly channel.
        .install_switch = "chrome-sxs",  // Install switch.
        .install_suffix = L"-Nightly",   // Install suffix.
        .logo_suffix = L"Canary",        // Logo suffix.
        .app_guid =
            L"{C6CB981E-DB30-4876-8639-109F8933582C}",  // A distinct app GUID.
        .base_app_name = L"Brave Nightly",        // A distinct base_app_name.
        .base_app_id = L"BraveNightly",           // A distinct base_app_id.
        .browser_prog_id_prefix = L"BraveSSHTM",  // Browser ProgID prefix.
        .browser_prog_id_description =
            L"Brave Nightly HTML Document",   // Browser ProgID description.
        .pdf_prog_id_prefix = L"BraveSSPDF",  // PDF ProgID prefix.
        .pdf_prog_id_description =
            L"Brave Nightly PDF Document",  // PDF ProgID description.
        .active_setup_guid =
            L"{C6CB981E-DB30-4876-8639-109F8933582C}",  // Active Setup GUID.
        .legacy_command_execute_clsid =
            L"{312ABB99-A176-4939-A39F-E8D34EA4D393}",  // CommandExecuteImpl
                                                        // CLSID.
        .toast_activator_clsid = {0xf2edbc59,
                                  0x7217,
                                  0x4da5,
                                  {0xa2, 0x59, 0x3, 0x2, 0xda, 0x6a, 0x0,
                                   0xe1}},  // Toast activator CLSID.
        .elevator_clsid = {0x1ce2f84f,
                           0x70cb,
                           0x4389,
                           {0x87, 0xdb, 0xd0, 0x99, 0x48, 0x30, 0xbb,
                            0x17}},  // Elevator CLSID.
        .elevator_iid = {0x1db2116f,
                         0x71b7,
                         0x49f0,
                         {0x89, 0x70, 0x33, 0xb1, 0xda, 0xcf, 0xb0, 0x72}},
        .default_channel_name = L"nightly",  // Forced channel name.
        .channel_strategy = ChannelStrategy::FIXED,
        .supports_system_level = true,  // Support system-level installs.
        .supports_set_as_default_browser =
            true,  // Support in-product set as default browser UX.
        .app_icon_resource_index =
            icon_resources::kSxSApplicationIndex,  // App icon resource index.
        .app_icon_resource_id = IDR_SXS,           // App icon resource id.
        .sandbox_sid_prefix =
            L"S-1-15-2-3251537155-1984446955-2931258699-841473695-1938553385-"
            L"934012152-",  // App container sid prefix for sandbox.
    },
};
#else
const InstallConstants kInstallModes[] = {
    // The primary (and only) install mode for Brave developer build.
    {
        .size = sizeof(kInstallModes[0]),
        .index = DEVELOPER_INDEX,  // The one and only mode for developer mode.
        .install_switch =
            "",  // No install switch for the primary install mode.
        .install_suffix =
            L"",  // Empty install_suffix for the primary install mode.
        .logo_suffix = L"",  // No logo suffix for the primary install mode.
        .app_guid =
            L"",  // Empty app_guid since no integraion with Brave Update.
        .base_app_name = L"Brave Development",     // A distinct base_app_name.
        .base_app_id = L"BraveDevelopment",        // A distinct base_app_id.
        .browser_prog_id_prefix = L"BraveDevHTM",  // Browser ProgID prefix.
        .browser_prog_id_description =
            L"Brave Development HTML Document",  // Browser ProgID description.
        .pdf_prog_id_prefix = L"BraveDevPDF",    // PDF ProgID prefix.
        .pdf_prog_id_description =
            L"Brave Development PDF Document",  // PDF ProgID description.
        .active_setup_guid =
            L"{D6527C63-5CDD-4EF3-9299-1504E17CBD18}",  // Active Setup GUID.
        .legacy_command_execute_clsid =
            L"{B2863926-AF5D-43A2-99CC-29EC43790C89}",  // CommandExecuteImpl
                                                        // CLSID.
        .toast_activator_clsid = {0xeb41c6e8,
                                  0xba35,
                                  0x4c06,
                                  {0x96, 0xe8, 0x6f, 0x30, 0xf1, 0x8c, 0xa5,
                                   0x5c}},  // Toast activator CLSID.
        .elevator_clsid = {0x5693e62d,
                           0xd6,
                           0x4421,
                           {0xaf, 0xe8, 0x58, 0xf3, 0xc9, 0x47, 0x43,
                            0x6a}},  // Elevator CLSID.
        .elevator_iid = {0x17239bf1,
                         0xa1dc,
                         0x4642,
                         {0x84, 0x6c, 0x1b, 0xac, 0x85, 0xf9, 0x6a, 0x10}},
        .default_channel_name =
            L"",  // Empty default channel name since no update integration.
        .channel_strategy = ChannelStrategy::UNSUPPORTED,
        .supports_system_level = true,  // Supports system-level installs.
        .supports_set_as_default_browser =
            true,  // Supports in-product set as default browser UX.
        .app_icon_resource_index =
            icon_resources::kApplicationIndex,  // App icon resource index.
        .app_icon_resource_id = IDR_MAINFRAME,  // App icon resource id.
        .sandbox_sid_prefix =
            L"S-1-15-2-3251537155-1984446955-2931258699-841473695-1938553385-"
            L"934012148-",  // App container sid prefix for sandbox.
    },
};
#endif

static_assert(_countof(kInstallModes) == NUM_INSTALL_MODES,
              "Imbalance between kInstallModes and InstallConstantIndex");

}  // namespace install_static
