/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_BRAVE_ADS_VIRTUAL_PREF_PROVIDER_DELEGATE_IOS_H_
#define BRAVE_IOS_BROWSER_BRAVE_ADS_VIRTUAL_PREF_PROVIDER_DELEGATE_IOS_H_

#include <string>
#include <string_view>

#include "base/memory/raw_ref.h"
#include "brave/components/brave_ads/core/browser/service/virtual_pref_provider.h"

class ProfileIOS;

namespace brave_ads {

class VirtualPrefProviderDelegateIOS : public VirtualPrefProvider::Delegate {
 public:
  explicit VirtualPrefProviderDelegateIOS(ProfileIOS& profile);

  VirtualPrefProviderDelegateIOS(const VirtualPrefProviderDelegateIOS&) =
      delete;
  VirtualPrefProviderDelegateIOS& operator=(
      const VirtualPrefProviderDelegateIOS&) = delete;

  ~VirtualPrefProviderDelegateIOS() override;

  std::string_view GetChannel() const override;

  std::string GetDefaultSearchEngineName() const override;

 private:
  const raw_ref<ProfileIOS> profile_;
};

}  // namespace brave_ads

#endif  // BRAVE_IOS_BROWSER_BRAVE_ADS_VIRTUAL_PREF_PROVIDER_DELEGATE_IOS_H_
