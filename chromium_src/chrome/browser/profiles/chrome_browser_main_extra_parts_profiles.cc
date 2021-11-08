/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#define AddProfilesExtraParts AddProfilesExtraParts_ChromiumImpl
#include "src/chrome/browser/profiles/chrome_browser_main_extra_parts_profiles.cc"
#undef AddProfilesExtraParts

#include "brave/browser/browser_context_keyed_service_factories.h"

namespace {

class BraveBrowserMainExtraPartsProfiles
    : public ChromeBrowserMainExtraPartsProfiles {
 public:
  BraveBrowserMainExtraPartsProfiles()
      : ChromeBrowserMainExtraPartsProfiles() {}

  BraveBrowserMainExtraPartsProfiles(
      const BraveBrowserMainExtraPartsProfiles&) = delete;
  BraveBrowserMainExtraPartsProfiles& operator=(
      const BraveBrowserMainExtraPartsProfiles&) = delete;

  void PreProfileInit() override {
    ChromeBrowserMainExtraPartsProfiles::PreProfileInit();
    brave::EnsureBrowserContextKeyedServiceFactoriesBuilt();
  }
};

}  // namespace

namespace chrome {

void AddProfilesExtraParts(ChromeBrowserMainParts* main_parts) {
  main_parts->AddParts(std::make_unique<BraveBrowserMainExtraPartsProfiles>());
}

}  // namespace chrome
