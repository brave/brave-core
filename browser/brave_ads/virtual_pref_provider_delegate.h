/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_ADS_VIRTUAL_PREF_PROVIDER_DELEGATE_H_
#define BRAVE_BROWSER_BRAVE_ADS_VIRTUAL_PREF_PROVIDER_DELEGATE_H_

#include <string>
#include <string_view>

#include "base/memory/raw_ref.h"
#include "brave/components/brave_ads/core/browser/virtual_pref/virtual_pref_provider.h"

class Profile;
class ProfileAttributesStorage;

namespace base {
class DictValue;
}  // namespace base

namespace brave_ads {

class VirtualPrefProviderDelegate final : public VirtualPrefProvider::Delegate {
 public:
  VirtualPrefProviderDelegate(
      Profile& profile,
      ProfileAttributesStorage& profile_attributes_storage);

  VirtualPrefProviderDelegate(const VirtualPrefProviderDelegate&) = delete;
  VirtualPrefProviderDelegate& operator=(const VirtualPrefProviderDelegate&) =
      delete;

  ~VirtualPrefProviderDelegate() override;

  // VirtualPrefProvider::Delegate:
  std::string_view GetChannel() const override;

  std::string GetDefaultSearchEngineName() const override;

  base::DictValue GetSerpMetrics() const override;

 private:
  const raw_ref<Profile> profile_;
  const raw_ref<ProfileAttributesStorage> profile_attributes_storage_;
};

}  // namespace brave_ads

#endif  // BRAVE_BROWSER_BRAVE_ADS_VIRTUAL_PREF_PROVIDER_DELEGATE_H_
