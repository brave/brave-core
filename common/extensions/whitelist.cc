/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/common/extensions/whitelist.h"

#include "brave/components/brave_component_updater/browser/local_data_files_service.h"
#include "extensions/common/constants.h"

// This is a hardcoded list of vetted extensions, mostly
// the built-in ones that ship with Brave or are used for
// unit tests.
// Don't add new extensions to this list. Add them to
// the files managed by the extension whitelist service.
const std::vector<std::string> kVettedExtensions{
    brave_extension_id,
    brave_rewards_extension_id,
    brave_webtorrent_extension_id,
    crl_set_extension_id,
    ethereum_remote_client_extension_id,
    hangouts_extension_id,
    widevine_extension_id,
    brave_component_updater::kLocalDataFilesComponentId,
    // Web Store
    "ahfgeienlihckogmohjhadlkjgocpleb",
    // Brave Automation Extension
    "aapnijgdinlhnhlmodcfapnahmbfebeb",
    // Test ID: Brave Default Ad Block Updater
    "naccapggpomhlhoifnlebfoocegenbol",
    // Test ID: Brave Regional Ad Block Updater
    // (9852EFC4-99E4-4F2D-A915-9C3196C7A1DE)
    "dlpmaigjliompnelofkljgcmlenklieh",
    // Test ID: Brave Tracking Protection Updater
    "eclbkhjphkhalklhipiicaldjbnhdfkc",
    // Test ID: PDFJS
    "kpbdcmcgkedhpbcpfndimofjnefgjidd",
    // Test ID: Brave HTTPS Everywhere Updater
    "bhlmpjhncoojbkemjkeppfahkglffilp",
    // Test ID: Brave Tor Client Updater, IPFS Client Updater
    "ngicbhhaldfdgmjhilmnleppfpmkgbbk",
    // Chromium PDF Viewer.
    "mhjfbmdgcfjbbpaeojofohoefgiehjai",
};
