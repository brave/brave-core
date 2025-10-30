/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/ios/browser/brave_ads/virtual_pref_provider_delegate_ios.h"

#include "base/strings/utf_string_conversions.h"
#include "base/version_info/channel.h"
#include "components/search_engines/template_url_data.h"
#include "components/search_engines/template_url_prepopulate_data.h"
#include "components/search_engines/template_url_prepopulate_data_resolver.h"
#include "ios/chrome/browser/search_engines/model/template_url_prepopulate_data_resolver_factory.h"
#include "ios/chrome/browser/shared/model/profile/profile_ios.h"
#include "ios/chrome/common/channel_info.h"

namespace brave_ads {

VirtualPrefProviderDelegateIOS::VirtualPrefProviderDelegateIOS(
    ProfileIOS& profile)
    : profile_(profile) {}

VirtualPrefProviderDelegateIOS::~VirtualPrefProviderDelegateIOS() = default;

std::string_view VirtualPrefProviderDelegateIOS::GetChannel() const {
  return version_info::GetChannelString(::GetChannel());
}

std::string VirtualPrefProviderDelegateIOS::GetDefaultSearchEngineName() const {
  auto* prepopulate_data_resolver =
      ios::TemplateURLPrepopulateDataResolverFactory::GetForProfile(&*profile_);
  const auto template_url_data = prepopulate_data_resolver->GetFallbackSearch();
  const std::u16string& default_search_engine_name =
      template_url_data ? template_url_data->short_name() : u"";
  return base::UTF16ToUTF8(default_search_engine_name);
}

}  // namespace brave_ads
