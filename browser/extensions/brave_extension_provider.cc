/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/extensions/brave_extension_provider.h"

#include <algorithm>
#include <string>
#include <vector>

#include "base/strings/utf_string_conversions.h"
#include "brave/browser/brave_browser_process_impl.h"
#include "brave/common/extensions/extension_constants.h"
#include "brave/components/brave_shields/browser/extension_whitelist_service.h"
#include "brave/components/brave_shields/browser/local_data_files_service.h"
#include "brave/grit/brave_generated_resources.h"
#include "ui/base/l10n/l10n_util.h"

namespace {

bool IsBlacklisted(const extensions::Extension* extension) {
  // This is a hardcoded list of extensions to block.
  // Don't add new extensions to this list. Add them to
  // the files managed by the extension whitelist service.
  static std::vector<std::string> blacklisted_extensions(
      {// Used for tests, corresponds to
       // brave/test/data/should-be-blocked-extension.
       "mlklomjnahgiddgfdgjhibinlfibfffc",
     });

  if (std::find(blacklisted_extensions.begin(), blacklisted_extensions.end(),
                extension->id()) != blacklisted_extensions.end())
    return true;

  return g_brave_browser_process->extension_whitelist_service()->IsBlacklisted(
      extension->id());
}

}  // namespace

namespace extensions {

bool BraveExtensionProvider::IsVetted(const Extension* extension) {
  // This is a hardcoded list of vetted extensions, mostly
  // the built-in ones that ship with Brave or are used for
  // unit tests.
  // Don't add new extensions to this list. Add them to
  // the files managed by the extension whitelist service.
  return BraveExtensionProvider::IsVetted(extension->id());
}

bool BraveExtensionProvider::IsVetted(const std::string id) {
  static std::vector<std::string> vetted_extensions({
      brave_extension_id,
      brave_rewards_extension_id,
      brave_sync_extension_id,
      brave_webtorrent_extension_id,
      crl_set_extension_id,
      hangouts_extension_id,
      widevine_extension_id,
      brave_shields::kLocalDataFilesComponentId,
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
      // Test ID: Brave Tor Client Updater
      "ngicbhhaldfdgmjhilmnleppfpmkgbbk",
      // Chromium PDF Viewer.
      "mhjfbmdgcfjbbpaeojofohoefgiehjai",
  });
  if (std::find(vetted_extensions.begin(), vetted_extensions.end(), id) !=
      vetted_extensions.end())
    return true;

  return g_brave_browser_process->extension_whitelist_service()->IsWhitelisted(
      id);
}

BraveExtensionProvider::BraveExtensionProvider() {}

BraveExtensionProvider::~BraveExtensionProvider() {}

std::string BraveExtensionProvider::GetDebugPolicyProviderName() const {
#if defined(NDEBUG)
  NOTREACHED();
  return std::string();
#else
  return "Brave Extension Provider";
#endif
}

bool BraveExtensionProvider::UserMayLoad(const Extension* extension,
                                         base::string16* error) const {
  if (IsBlacklisted(extension)) {
    if (error) {
      *error = l10n_util::GetStringFUTF16(IDS_EXTENSION_CANT_INSTALL_ON_BRAVE,
                                          base::UTF8ToUTF16(extension->name()),
                                          base::UTF8ToUTF16(extension->id()));
    }
    DVLOG(1) << "Extension will not install "
             << " ID: " << base::UTF8ToUTF16(extension->id()) << ", "
             << " Name: " << base::UTF8ToUTF16(extension->name());
    return false;
  }
  return true;
}

bool BraveExtensionProvider::MustRemainInstalled(const Extension* extension,
                                                 base::string16* error) const {
  return extension->id() == brave_extension_id ||
         extension->id() == brave_rewards_extension_id ||
         extension->id() == brave_sync_extension_id;
}

}  // namespace extensions
