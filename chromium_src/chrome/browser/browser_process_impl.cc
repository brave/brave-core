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
#include "build/build_config.h"
#include "extensions/buildflags/buildflags.h"

#if BUILDFLAG(ENABLE_EXTENSIONS)
#include "brave/browser/extensions/brave_extensions_browser_client_impl.h"
#define ChromeExtensionsBrowserClient BraveExtensionsBrowserClientImpl
#endif

#if !BUILDFLAG(IS_ANDROID) && !BUILDFLAG(IS_CHROMEOS)
#include "components/soda/soda_installer.h"

namespace speech {

#if BUILDFLAG(ENABLE_LOCAL_AI)
// Forward declared to avoid adding a compile-time dependency.
// Implementation is provided by //brave/browser/speech:chromium_impl.
std::unique_ptr<SodaInstaller> CreateBraveSodaInstaller();
#else
namespace {

// Builds that ship no model still replace upstream's global installer.
// Reporting no languages keeps Web Speech's `available()` unavailable and
// `install()` refused, where upstream's reports en-US as downloadable and then
// parks the `install()` reply on a download that never arrives. `Init` is a
// no-op so nothing registers a language pack at startup.
//
// A stub rather than no installer at all, because `Install` dereferences
// `SodaInstaller::GetInstance()` without a null check.
class StubSodaInstaller final : public SodaInstaller {
 public:
  // SodaInstaller:
  std::vector<std::string> GetLiveCaptionEnabledLanguages() const override {
    return {};
  }
  std::vector<std::string> GetAvailableLanguages() const override { return {}; }
  base::FilePath GetSodaBinaryPath() const override { return base::FilePath(); }
  base::FilePath GetLanguagePath(std::string_view language) const override {
    return base::FilePath();
  }
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
#endif  // !BUILDFLAG(IS_ANDROID) && !BUILDFLAG(IS_CHROMEOS)

#include <chrome/browser/browser_process_impl.cc>
#if BUILDFLAG(ENABLE_EXTENSIONS)
#undef ChromeExtensionsBrowserClient
#endif
