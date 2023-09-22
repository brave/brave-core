/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/request_otr/browser/request_otr_service.h"

#include <utility>

#include "base/containers/contains.h"
#include "base/containers/flat_set.h"
#include "base/logging.h"
#include "brave/components/request_otr/browser/request_otr_component_installer.h"
#include "brave/components/request_otr/common/pref_names.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "net/base/registry_controlled_domains/registry_controlled_domain.h"
#include "net/base/url_util.h"

namespace request_otr {

RequestOTRService::RequestOTRService() {}

RequestOTRService::~RequestOTRService() = default;

void RequestOTRService::OnRulesReady(const std::string& json_content) {
  auto parsed_rules = RequestOTRRule::ParseRules(json_content);
  if (!parsed_rules.has_value()) {
    DVLOG(1) << "Error: no rules parsed. " << parsed_rules.error();
    return;
  }
  rules_.clear();
  host_cache_.clear();
  rules_ = std::move(parsed_rules.value().first);
  host_cache_ = parsed_rules.value().second;
  DVLOG(1) << host_cache_.size() << " unique hosts, " << rules_.size()
           << " rules parsed from " << kRequestOTRConfigFile;
}

bool RequestOTRService::ShouldOfferOTR(const GURL& url) {
  // Check host cache
  const std::string etldp1 = RequestOTRRule::GetETLDForRequestOTR(url.host());
  if (!base::Contains(host_cache_, etldp1)) {
    return false;
  }

  for (const std::unique_ptr<RequestOTRRule>& rule : rules_) {
    if (rule->ShouldBlock(url)) {
      return true;
    }
  }

  return false;
}

bool RequestOTRService::OfferedOTR(const GURL& url) {
  if (!url.SchemeIsHTTPOrHTTPS()) {
    return false;
  }
  return base::Contains(offered_otr_cache_,
                        net::URLToEphemeralStorageDomain(url));
}

void RequestOTRService::SetOfferedOTR(const GURL& url) {
  if (!url.SchemeIsHTTPOrHTTPS()) {
    return;
  }

  offered_otr_cache_[net::URLToEphemeralStorageDomain(url)] =
      url::Origin::Create(url);
}

bool RequestOTRService::IsOTR(const GURL& url) {
  if (!url.SchemeIsHTTPOrHTTPS()) {
    return false;
  }
  return base::Contains(requested_otr_cache_,
                        net::URLToEphemeralStorageDomain(url));
}

void RequestOTRService::SetOTR(const GURL& url, bool enabled) {
  if (enabled) {
    RequestOTR(url);
  } else {
    WithdrawOTR(url);
  }
}

void RequestOTRService::RequestOTR(const GURL& url) {
  if (!url.SchemeIsHTTPOrHTTPS()) {
    return;
  }

  requested_otr_cache_[net::URLToEphemeralStorageDomain(url)] =
      url::Origin::Create(url);
}

void RequestOTRService::WithdrawOTR(const GURL& url) {
  if (!url.SchemeIsHTTPOrHTTPS()) {
    return;
  }
  std::string domain = net::URLToEphemeralStorageDomain(url);
  offered_otr_cache_.erase(domain);
  requested_otr_cache_.erase(domain);
}

// static
void RequestOTRService::RegisterProfilePrefs(PrefRegistrySimple* registry) {
  registry->RegisterIntegerPref(kRequestOTRActionOption,
                                static_cast<int>(RequestOTRActionOption::kAsk));
}

}  // namespace request_otr
