// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/components/brave_search/browser/brave_search_default_host.h"

#include <string>
#include <utility>

#include "base/logging.h"
#include "brave/components/brave_search/browser/brave_search_fallback_host.h"
#include "brave/components/brave_search/browser/prefs.h"
#include "brave/components/brave_search/common/features.h"
#include "brave/components/brave_search_conversion/types.h"
#include "brave/components/brave_search_conversion/utils.h"
#include "brave/components/time_period_storage/daily_storage.h"
#include "build/build_config.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "components/search_engines/template_url.h"
#include "components/search_engines/template_url_service.h"

using brave_search_conversion::ConversionType;
using brave_search_conversion::GetConversionType;
using brave_search_conversion::IsNTPPromotionEnabled;

namespace {

bool IsSearchPromotionEnabled(PrefService* prefs, TemplateURLService* service) {
  return (GetConversionType(prefs, service) != ConversionType::kNone) ||
         IsNTPPromotionEnabled(prefs, service);
}

TemplateURL* GetSearchTemplateForSite(TemplateURLService* service,
                                      const std::string& host) {
  // Prefer built-in entries, then offer site-provided entries,
  // but ignore extension-provided entries.
  TemplateURL::TemplateURLVector urls = service->GetTemplateURLs();
  TemplateURL* other_entry = nullptr;
  for (TemplateURL* template_url : urls) {
    if (template_url->url_ref().GetHost(SearchTermsData()) != host) {
      continue;
    }
    if (service->ShowInDefaultList(template_url)) {
      return template_url;
    } else if (template_url->type() != TemplateURL::OMNIBOX_API_EXTENSION &&
               other_entry == nullptr) {
      other_entry = template_url;
    }
  }
  return other_entry;
}

}  // namespace

namespace brave_search {

// static
void BraveSearchDefaultHost::RegisterProfilePrefs(
    PrefRegistrySimple* registry) {
  registry->RegisterListPref(brave_search::prefs::kDailyAsked);
  registry->RegisterIntegerPref(brave_search::prefs::kTotalAsked, 0);
#if BUILDFLAG(IS_ANDROID)
  registry->RegisterBooleanPref(brave_search::prefs::kFetchFromNative, false);
#endif
}

BraveSearchDefaultHost::BraveSearchDefaultHost(
    const std::string& host,
    TemplateURLService* template_url_service,
    PrefService* prefs)
    : host_(host), template_url_service_(template_url_service), prefs_(prefs) {}

BraveSearchDefaultHost::~BraveSearchDefaultHost() = default;

bool BraveSearchDefaultHost::CanSetDefaultSearchProvider(TemplateURL* provider,
                                                         bool is_historic) {
  // Return false if:
  //   - current site is default OR
  //   - has asked more than X times in Y period OR
  //   - search setting is controlled by extension OR
  //   - search setting is controlled by group policy OR
  //   - is incognito / private OR
  // Otherwise return true
  if (template_url_service_->IsExtensionControlledDefaultSearch()) {
    VLOG(1) << "CanSetDefaultSearchProvider: "
            << "Extension is controlling search engine.";
    return false;
  }
  if (template_url_service_->is_default_search_managed()) {
    VLOG(1) << "CanSetDefaultSearchProvider: "
            << "Group Policy is controlling search engine.";
    return false;
  }
  if (!provider) {
    VLOG(1) << "CanSetDefaultSearchProvider: "
            << "Site has not created search engine: " << host_;
    return false;
  }
  const auto* default_provider =
      template_url_service_->GetDefaultSearchProvider();
  const bool has_default_provider = (default_provider != nullptr);
  bool is_default =
      (has_default_provider && default_provider->id() == provider->id());
  if (is_default) {
    return false;
  }
  if (!template_url_service_->CanMakeDefault(provider)) {
    VLOG(1) << "CanSetDefaultSearchProvider: "
            << "Not allowed to make site search engine the default: " << host_;
    return false;
  }

  // Don't check 24h limit.
  if (can_always_set_default_)
    return true;

  if (!is_historic) {
    // Limit how often user can be asked. This is not site-specific
    // since this API has only 1 intentional public site at the moment.
    DailyStorage daily_storage(prefs_, prefs::kDailyAsked);
    // If we're verifying that the user was allowed, then
    auto daily_count = daily_storage.GetLast24HourSum();
    if (daily_count >= GetMaxDailyCanAskCount()) {
      // Cannot ask since we've asked too many times recently
      VLOG(1) << "CanSetDefaultSearchProvider: "
              << "Asked too many times last 24 hours: " << daily_count;
      return false;
    }
    auto total_count =
        static_cast<uint64_t>(prefs_->GetInteger(prefs::kTotalAsked));
    if (total_count >= GetMaxTotalCanAskCount()) {
      VLOG(1) << "CanSetDefaultSearchProvider: "
              << "Asked too many total times: " << total_count;
      return false;
    }
    // Assume that the user has been asked, and record it against the limit.
    daily_storage.RecordValueNow(1u);
    prefs_->SetInteger(prefs::kTotalAsked, total_count + 1);
  }
  return true;
}

uint64_t BraveSearchDefaultHost::GetMaxDailyCanAskCount() {
  return static_cast<uint64_t>(
      features::kBraveSearchDefaultAPIDailyLimit.Get());
}

uint64_t BraveSearchDefaultHost::GetMaxTotalCanAskCount() {
  return static_cast<uint64_t>(
      features::kBraveSearchDefaultAPITotalLimit.Get());
}

void BraveSearchDefaultHost::SetCanAlwaysSetDefault() {
  // We have 24h limit if search promotion url is not loaded.
  // When renderer detected that current url is for promotion,
  // renderer requests to remove this limit. In that case,
  // limit is removed when promotion is enabled.
  can_always_set_default_ =
      IsSearchPromotionEnabled(prefs_, template_url_service_);
}

void BraveSearchDefaultHost::GetCanSetDefaultSearchProvider(
    GetCanSetDefaultSearchProviderCallback callback) {
  auto* provider = GetSearchTemplateForSite(template_url_service_, host_);
  auto can_set = CanSetDefaultSearchProvider(provider, false);
  // Store a token so that if SetIsDefaultSearchProvider is called,
  // we can check that this function previously returned `true`.
  can_set_default_ = can_set;
  std::move(callback).Run(can_set);
}

void BraveSearchDefaultHost::SetIsDefaultSearchProvider() {
  // Verify previously successful call to GetCanSetDefaultSearchProvider
  if (!can_set_default_) {
    return;
  }
  // Reset token
  can_set_default_ = false;
  // Verify desired engine is already in list of search engines.
  auto* provider = GetSearchTemplateForSite(template_url_service_, host_);
  // Validate provider
  if (!CanSetDefaultSearchProvider(provider, true)) {
    return;
  }
  // TODO(petemill): Consider showing a confirmation dialog to user.
  // For now we assume the confirmation UI is within the content.
  template_url_service_->SetUserSelectedDefaultSearchProvider(provider);
  // TODO(sergz): A workaround for Android to avoid default se overwrite on
  // Settings menu open.
#if BUILDFLAG(IS_ANDROID)
  prefs_->SetBoolean(prefs::kFetchFromNative, true);
#endif
}

}  // namespace brave_search
