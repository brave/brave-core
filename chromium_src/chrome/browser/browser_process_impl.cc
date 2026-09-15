/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "base/files/file_path.h"
#include "components/soda/soda_installer.h"
#include "extensions/buildflags/buildflags.h"

#if BUILDFLAG(ENABLE_EXTENSIONS)
#include "brave/browser/extensions/brave_extensions_browser_client_impl.h"
#define ChromeExtensionsBrowserClient BraveExtensionsBrowserClientImpl
#endif

namespace speech {

namespace {

// Brave ships no SODA, so the global installer reports no languages, which
// keeps the Web Speech `available()` answer unavailable and `install()`
// refused. Upstream's would report SODA's en-US as downloadable and then park
// an `install()` reply forever on a download that never arrives. `Init` is a
// no-op for the same reason, so nothing registers a language pack at startup.
//
// An installer still has to exist, because
// `OnDeviceSpeechRecognitionImpl::Available` dereferences
// `SodaInstaller::GetInstance()` unconditionally.
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

}  // namespace speech

#include <chrome/browser/browser_process_impl.cc>
#if BUILDFLAG(ENABLE_EXTENSIONS)
#undef ChromeExtensionsBrowserClient
#endif
