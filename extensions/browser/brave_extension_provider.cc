/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/extensions/browser/brave_extension_provider.h"

#include <algorithm>
#include <string>
#include <vector>

#include "base/strings/utf_string_conversions.h"
#include "brave/grit/generated_resources.h"
#include "ui/base/l10n/l10n_util.h"

namespace {

const char* brave_extension_id = "mnojpmjdmbbfmejpflffifhffcmidifd";

bool IsWhitelisted(const std::string& id) {
  static std::vector<std::string> whitelist({
    // Brave
    brave_extension_id,
    // CryptoTokenExtension
    "kmendfapggjehodndflmmgagdbamhnfd",
    // Cloud Print
    "mfehgcgbbipciphmccgaenjidiccnmng",
    // Chromium PDF Viewer
    "mhjfbmdgcfjbbpaeojofohoefgiehjai",
    // Web Store
    "ahfgeienlihckogmohjhadlkjgocpleb",
    // Grammarly for Chrome
    "kbfnbcaeplbcioakkpcpgfkobkghlhen",
    // Brave Ad Block Updater
    "cffkpbalmllkdoenhmdmpbkajipdjfam",
    // Brave Tracking Protection Updater
    "afalakplffnnnlkncjhbmahjfjhmlkal",
    // Brave HTTPS Everywhere Updater
    "oofiananboodjbbmdelgdommihjbkfag",
    // Test ID: Brave Ad Block Updater
    "naccapggpomhlhoifnlebfoocegenbol",
    // Test ID: Brave Tracking Protection Updater
    "eclbkhjphkhalklhipiicaldjbnhdfkc",
    // Test ID: Brave HTTPS Everywhere Updater
    "bhlmpjhncoojbkemjkeppfahkglffilp",
    // 1Password
    "aomjjhallfgjeglblehebfpbcfeobpgk",
    // Dashlane
    "fdjamakpfbbddfjaooikfcpapjohcfmg",
    // LastPass
    "hdokiejnpimakedhajhdlcegeplioahd",
    // Pocket
    "niloccemoadcdkdjlinkgdfekeahmflj",
    // Enpass
    "kmcfomidfpdkfieipokbalgegidffkal",
    // Vimium
    "dbepggeogbaibhgnhhndojpepiihcmeb",
    // bitwarden
    "nngceckbapebfimnlniiiahkandclblb",
    // Honey
    "bmnlcjabgnpnenekpadlanbbkooimhnj",
    // Pinterest
    "gpdjojdkbbmdfjfahjcgigfpmkopogic",
    // MetaMask
    "nkbihfbeogaeaoehlefnkodbefgpgknn",
    // Reddit Enhancement Suite
    "kbmfpngjjgdllneeigpgjifpgocmfgmb",
    // BetterTTV
    "ajopnjidmegmdimjlfnijceegpefgped",
  });
  return std::find(whitelist.begin(), whitelist.end(), id) != whitelist.end();
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
  if (!IsWhitelisted(extension->id())) {
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
  return extension->id() == brave_extension_id;
}

}  // namespace extensions
