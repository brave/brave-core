/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_ADS_VIRTUAL_PREF_PROVIDER_DELEGATE_H_
#define BRAVE_BROWSER_BRAVE_ADS_VIRTUAL_PREF_PROVIDER_DELEGATE_H_

#include <string>
#include <string_view>

#include "base/memory/raw_ref.h"
#include "brave/components/brave_ads/core/browser/service/virtual_pref_provider.h"

class Profile;

namespace brave_ads {

class VirtualPrefProviderDelegate : public VirtualPrefProvider::Delegate {
 public:
  explicit VirtualPrefProviderDelegate(Profile& profile);

  VirtualPrefProviderDelegate(const VirtualPrefProviderDelegate&) = delete;
  VirtualPrefProviderDelegate& operator=(const VirtualPrefProviderDelegate&) =
      delete;

  ~VirtualPrefProviderDelegate() override;

  std::string_view GetChannel() const override;

  std::string GetDefaultSearchEngineName() const override;

 private:
  const raw_ref<Profile> profile_;
};

}  // namespace brave_ads

#endif  // BRAVE_BROWSER_BRAVE_ADS_VIRTUAL_PREF_PROVIDER_DELEGATE_H_
