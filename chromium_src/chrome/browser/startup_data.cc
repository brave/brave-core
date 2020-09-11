/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/macros.h"
#include "brave/browser/browser_context_keyed_service_factories.h"
#include "chrome/browser/profiles/chrome_browser_main_extra_parts_profiles.h"

namespace {

class BraveBrowserMainExtraPartsProfiles
    : public ChromeBrowserMainExtraPartsProfiles {
 public:
  BraveBrowserMainExtraPartsProfiles() = default;
  ~BraveBrowserMainExtraPartsProfiles() override = default;

  static void EnsureBrowserContextKeyedServiceFactoriesBuilt() {
    ChromeBrowserMainExtraPartsProfiles::
        EnsureBrowserContextKeyedServiceFactoriesBuilt();
    brave::EnsureBrowserContextKeyedServiceFactoriesBuilt();
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(BraveBrowserMainExtraPartsProfiles);
};

}  // namespace

#define ChromeBrowserMainExtraPartsProfiles BraveBrowserMainExtraPartsProfiles
#include "../../../../chrome/browser/startup_data.cc"
#undef ChromeBrowserMainExtraPartsProfiles
