/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_ads/virtual_pref_provider_delegate.h"

#include "base/strings/utf_string_conversions.h"
#include "base/version_info/channel.h"
#include "chrome/browser/search_engines/template_url_prepopulate_data_resolver_factory.h"
#include "chrome/common/channel_info.h"
#include "components/search_engines/template_url_data.h"
#include "components/search_engines/template_url_prepopulate_data.h"
#include "components/search_engines/template_url_prepopulate_data_resolver.h"

namespace brave_ads {

VirtualPrefProviderDelegate::VirtualPrefProviderDelegate(Profile& profile)
    : profile_(profile) {}

VirtualPrefProviderDelegate::~VirtualPrefProviderDelegate() = default;

std::string_view VirtualPrefProviderDelegate::GetChannel() const {
  return version_info::GetChannelString(chrome::GetChannel());
}

std::string VirtualPrefProviderDelegate::GetDefaultSearchEngineName() const {
  auto* prepopulate_data_resolver =
      TemplateURLPrepopulateData::ResolverFactory::GetForProfile(&*profile_);
  const auto template_url_data = prepopulate_data_resolver->GetFallbackSearch();
  const std::u16string& default_search_engine_name =
      template_url_data ? template_url_data->short_name() : u"";
  return base::UTF16ToUTF8(default_search_engine_name);
}

}  // namespace brave_ads
