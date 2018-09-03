/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/renderer_host/brave_navigation_ui_data.h"

#include <memory>

#include "base/memory/ptr_util.h"

BraveNavigationUIData::BraveNavigationUIData()
    : ChromeNavigationUIData(),
      tor_profile_service_(nullptr) {}

BraveNavigationUIData::BraveNavigationUIData(
    NavigationHandle* navigation_handle)
    : ChromeNavigationUIData(navigation_handle),
      tor_profile_service_(nullptr) {}

BraveNavigationUIData::~BraveNavigationUIData() {}

std::unique_ptr<content::NavigationUIData> BraveNavigationUIData::Clone()
    const {
      content::NavigationUIData* chrome_copy =
    (ChromeNavigationUIData::Clone().release());
  BraveNavigationUIData* copy =
    static_cast<BraveNavigationUIData*>(chrome_copy);

  copy->tor_profile_service_ = tor_profile_service_;

  return base::WrapUnique(copy);
}

void BraveNavigationUIData::SetTorProfileService(
    TorProfileService* tor_profile_service) {
  tor_profile_service_ = tor_profile_service;
}

TorProfileService* BraveNavigationUIData::GetTorProfileService() const {
  return tor_profile_service_;
}
