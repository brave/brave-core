/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "base/files/file_path.h"
#include "brave/components/local_ai/buildflags/buildflags.h"
#include "components/soda/soda_installer.h"
#include "extensions/buildflags/buildflags.h"

#if BUILDFLAG(ENABLE_EXTENSIONS)
#include "brave/browser/extensions/brave_extensions_browser_client_impl.h"
#define ChromeExtensionsBrowserClient BraveExtensionsBrowserClientImpl
#endif

namespace speech {

// Brave installs its own on-device speech model rather than SODA, which it does
// not ship, so its installer takes the place of upstream's as the global
// SodaInstaller. The plaster for this source constructs it here.
#if BUILDFLAG(ENABLE_LOCAL_AI)
// Forward declared to avoid adding a compile-time dependency.
// Implementation is provided by //brave/browser/speech:chromium_impl.
std::unique_ptr<SodaInstaller> CreateBraveSodaInstaller();
#else
namespace {

// Builds that ship no model still need an installer, because
// OnDeviceSpeechRecognitionImpl dereferences SodaInstaller::GetInstance()
// without a null check. This one installs nothing: Brave does not ship SODA
// either, and availability reports on-device speech as unavailable here.
class StubSodaInstaller final : public SodaInstaller {
 public:
  // SodaInstaller:

  // Empty, so that `SpeechRecognition.install()` is refused outright.
  // `OnDeviceSpeechRecognitionImpl::Install` reads this through
  // `IsLanguageInstallable` before it commits to an install, and an install it
  // did commit to would park its reply forever, since nothing below ever
  // reports an outcome.
  std::vector<std::string> GetLiveCaptionEnabledLanguages() const override {
    return {};
  }
  std::vector<std::string> GetAvailableLanguages() const override { return {}; }

  // Empty paths. These builds ship no SODA library and no language pack.
  base::FilePath GetSodaBinaryPath() const override { return base::FilePath(); }
  base::FilePath GetLanguagePath(std::string_view language) const override {
    return base::FilePath();
  }

  // No-ops, so that nothing installs, uninstalls or records a SODA language
  // pack. `SodaInstaller::Init` runs at startup for every profile and
  // `LiveCaptionController` calls in from the caption UI, neither of them
  // through `SpeechRecognition.install()`.
  void Init(PrefService* profile_prefs, PrefService* global_prefs) override {}
  void InstallLanguage(std::string_view language,
                       PrefService* global_prefs) override {}
  void UninstallLanguage(std::string_view language,
                         PrefService* global_prefs) override {}
  void RegisterLanguage(std::string_view language,
                        PrefService* global_prefs) override {}
  void UnregisterLanguage(std::string_view language,
                          PrefService* global_prefs) override {}

 protected:
  // SodaInstaller:
  void InstallSoda(PrefService* global_prefs) override {}
  void UninstallSoda(PrefService* global_prefs) override {}
};

}  // namespace

std::unique_ptr<SodaInstaller> CreateBraveSodaInstaller() {
  return std::make_unique<StubSodaInstaller>();
}
#endif  // BUILDFLAG(ENABLE_LOCAL_AI)

}  // namespace speech

#include <chrome/browser/browser_process_impl.cc>
#if BUILDFLAG(ENABLE_EXTENSIONS)
#undef ChromeExtensionsBrowserClient
#endif
