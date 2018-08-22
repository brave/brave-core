/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/extensions/brave_extension_provider.h"

#include <algorithm>
#include <string>
#include <vector>

#include "base/strings/utf_string_conversions.h"
#include "brave/common/extensions/extension_constants.h"
#include "brave/grit/brave_generated_resources.h"
#include "ui/base/l10n/l10n_util.h"

namespace {

bool IsWhitelisted(const extensions::Extension* extension) {
  // Allow PWAs to run
  if (extension->GetType() == extensions::Manifest::TYPE_HOSTED_APP) {
    return true;
  }
  static std::vector<std::string> whitelist({
    brave_extension_id,
    brave_webtorrent_extension_id,
    pdfjs_extension_id,
    // 1Password
    "aomjjhallfgjeglblehebfpbcfeobpgk",
    // BetterTTV
    "ajopnjidmegmdimjlfnijceegpefgped",
    // Cloud Print
    "mfehgcgbbipciphmccgaenjidiccnmng",
    // CryptoTokenExtension
    "kmendfapggjehodndflmmgagdbamhnfd",
    // Bitwarden
    "nngceckbapebfimnlniiiahkandclblb",
    // Brave Ad Block Updater
    "cffkpbalmllkdoenhmdmpbkajipdjfam",
    // Brave Tracking Protection Updater
    "afalakplffnnnlkncjhbmahjfjhmlkal",
    // Brave HTTPS Everywhere Updater
    "oofiananboodjbbmdelgdommihjbkfag",
    // Brave Tor Client Updater (Windows)
    "cpoalefficncklhjfpglfiplenlpccdb",
    // Brave Tor Client Updater (Mac)
    "cldoidikboihgcjfkhdeidbpclkineef",
    // Brave Tor Client Updater (Linux)
    "biahpgbdmdkfgndcmfiipgcebobojjkp",
    // Dashlane
    "fdjamakpfbbddfjaooikfcpapjohcfmg",
    // Enpass
    "kmcfomidfpdkfieipokbalgegidffkal",
    // Grammarly for Chrome
    "kbfnbcaeplbcioakkpcpgfkobkghlhen",
    // Honey
    "bmnlcjabgnpnenekpadlanbbkooimhnj",
    // LastPass
    "hdokiejnpimakedhajhdlcegeplioahd",
    // MetaMask
    "nkbihfbeogaeaoehlefnkodbefgpgknn",
    // Pinterest
    "gpdjojdkbbmdfjfahjcgigfpmkopogic",
    // Pocket
    "niloccemoadcdkdjlinkgdfekeahmflj",
    // Vimium
    "dbepggeogbaibhgnhhndojpepiihcmeb",
    // Reddit Enhancement Suite
    "kbmfpngjjgdllneeigpgjifpgocmfgmb",
    // Web Store
    "ahfgeienlihckogmohjhadlkjgocpleb",
    // Brave Automation Extension
    "aapnijgdinlhnhlmodcfapnahmbfebeb",
    // Test ID: Brave Default Ad Block Updater
    "naccapggpomhlhoifnlebfoocegenbol",
    // Test ID: Brave Regional Ad Block Updater (9852EFC4-99E4-4F2D-A915-9C3196C7A1DE)
    "dlpmaigjliompnelofkljgcmlenklieh",
    // Test ID: Brave Tracking Protection Updater
    "eclbkhjphkhalklhipiicaldjbnhdfkc",
    // Test ID: PDFJS
    "kpbdcmcgkedhpbcpfndimofjnefgjidd",
    // Test ID: Brave HTTPS Everywhere Updater
    "bhlmpjhncoojbkemjkeppfahkglffilp",
    // Test ID: Brave Tor Client Updater
    "ngicbhhaldfdgmjhilmnleppfpmkgbbk"
  });
  return std::find(whitelist.begin(), whitelist.end(),
      extension->id()) != whitelist.end();
}

}  // namespace

namespace extensions {

BraveExtensionProvider::BraveExtensionProvider() {
}

BraveExtensionProvider::~BraveExtensionProvider() {
}

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
  if (!IsWhitelisted(extension)) {
    if (error) {
      *error =
        l10n_util::GetStringFUTF16(IDS_EXTENSION_CANT_INSTALL_ON_BRAVE,
                                   base::UTF8ToUTF16(extension->name()),
                                   base::UTF8ToUTF16(extension->id()));
    }
#if defined(NDEBUG)
    LOG(ERROR) << "Extension will not install "
      << " ID: " << base::UTF8ToUTF16(extension->id()) << ", "
      << " Name: " << base::UTF8ToUTF16(extension->name());
#endif
    return false;
  }
  return true;
}

bool BraveExtensionProvider::MustRemainInstalled(const Extension* extension,
                                                 base::string16* error) const {
  return extension->id() == brave_extension_id ||
    extension->id() == brave_webtorrent_extension_id;
}

}  // namespace extensions
