/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/web_discovery/browser/wdp_service.h"

#include <utility>

#include "base/functional/bind.h"
#include "brave/components/constants/pref_names.h"
#include "brave/components/web_discovery/browser/content_scraper.h"
#include "brave/components/web_discovery/browser/payload_generator.h"
#include "brave/components/web_discovery/browser/pref_names.h"
#include "brave/components/web_discovery/browser/privacy_guard.h"
#include "brave/components/web_discovery/browser/server_config_loader.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/render_frame_host.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/service_manager/public/cpp/interface_provider.h"

namespace web_discovery {

WDPService::WDPService(
    PrefService* local_state,
    PrefService* profile_prefs,
    base::FilePath user_data_dir,
    scoped_refptr<network::SharedURLLoaderFactory> shared_url_loader_factory)
    : local_state_(local_state),
      profile_prefs_(profile_prefs),
      user_data_dir_(user_data_dir),
      shared_url_loader_factory_(shared_url_loader_factory) {
  pref_change_registrar_.Init(profile_prefs);
  pref_change_registrar_.Add(kWebDiscoveryEnabled,
                             base::BindRepeating(&WDPService::OnEnabledChange,
                                                 base::Unretained(this)));

  if (profile_prefs_->GetBoolean(kWebDiscoveryEnabled)) {
    Start();
  }
}

WDPService::~WDPService() = default;

void WDPService::RegisterLocalStatePrefs(PrefRegistrySimple* registry) {
  registry->RegisterTimePref(kPatternsRetrievalTime, {});
}

void WDPService::RegisterProfilePrefs(PrefRegistrySimple* registry) {
  registry->RegisterDictionaryPref(kAnonymousCredentialsDict);
  registry->RegisterStringPref(kCredentialRSAPrivateKey, {});
  registry->RegisterStringPref(kCredentialRSAPublicKey, {});
  registry->RegisterListPref(kScheduledDoubleFetches);
  registry->RegisterListPref(kScheduledReports);
  registry->RegisterDictionaryPref(kUsedBasenameCounts);
}

void WDPService::Start() {
  credential_manager_ = std::make_unique<CredentialManager>(
      profile_prefs_, shared_url_loader_factory_.get(),
      &last_loaded_server_config_);
  server_config_loader_ = std::make_unique<ServerConfigLoader>(
      local_state_, user_data_dir_, shared_url_loader_factory_.get(),
      base::BindRepeating(&WDPService::OnConfigChange, base::Unretained(this)),
      base::BindRepeating(&WDPService::OnPatternsLoaded,
                          base::Unretained(this)));
}

void WDPService::Stop() {
  reporter_ = nullptr;
  double_fetcher_ = nullptr;
  content_scraper_ = nullptr;
  server_config_loader_ = nullptr;
  credential_manager_ = nullptr;
  last_loaded_server_config_ = nullptr;
}

void WDPService::OnEnabledChange() {
  if (profile_prefs_->GetBoolean(kWebDiscoveryEnabled)) {
    Start();
  } else {
    Stop();
  }
}

void WDPService::OnConfigChange(std::unique_ptr<ServerConfig> config) {
  last_loaded_server_config_ = std::move(config);
  credential_manager_->JoinGroups();
}

void WDPService::OnPatternsLoaded(std::unique_ptr<PatternsGroup> patterns) {
  last_loaded_patterns_ = std::move(patterns);
  content_scraper_ = std::make_unique<ContentScraper>(
      &last_loaded_server_config_, &last_loaded_patterns_, &regex_util_);
  double_fetcher_ = std::make_unique<DoubleFetcher>(
      profile_prefs_.get(), shared_url_loader_factory_.get(),
      base::BindRepeating(&WDPService::OnDoubleFetched,
                          base::Unretained(this)));
  reporter_ = std::make_unique<Reporter>(
      profile_prefs_.get(), shared_url_loader_factory_.get(),
      credential_manager_.get(), &regex_util_, &last_loaded_server_config_);
}

void WDPService::OnDoubleFetched(const GURL& url,
                                 const base::Value& associated_data,
                                 std::optional<std::string> response_body) {
  if (!response_body) {
    return;
  }
  auto prev_scrape_result = PageScrapeResult::FromValue(associated_data);
  if (!prev_scrape_result) {
    return;
  }
  content_scraper_->ParseAndScrapePage(
      url, true, std::move(prev_scrape_result), *response_body,
      base::BindOnce(&WDPService::OnContentScraped, base::Unretained(this),
                     true));
}

void WDPService::OnFinishNavigation(
    const GURL& url,
    content::RenderFrameHost* render_frame_host) {
  if (!content_scraper_) {
    return;
  }
  const auto* matching_url_details =
      last_loaded_patterns_->GetMatchingURLPattern(url, false);
  if (!matching_url_details) {
    return;
  }
  VLOG(1) << "URL matched pattern " << matching_url_details->id << ": " << url;
  if (IsPrivateURLLikely(regex_util_, url, matching_url_details)) {
    return;
  }
  mojo::Remote<mojom::DocumentExtractor> remote;
  render_frame_host->GetRemoteInterfaces()->GetInterface(
      remote.BindNewPipeAndPassReceiver());
  auto remote_id = document_extractor_remotes_.Add(std::move(remote));
  content_scraper_->ScrapePage(url, false,
                               document_extractor_remotes_.Get(remote_id),
                               base::BindOnce(&WDPService::OnContentScraped,
                                              base::Unretained(this), false));
}

void WDPService::OnContentScraped(bool is_strict,
                                  std::unique_ptr<PageScrapeResult> result) {
  if (!result) {
    return;
  }
  auto* original_url_details =
      last_loaded_patterns_->GetMatchingURLPattern(result->url, is_strict);
  if (!original_url_details) {
    return;
  }
  if (!is_strict && original_url_details->is_search_engine) {
    auto* strict_url_details =
        last_loaded_patterns_->GetMatchingURLPattern(result->url, true);
    if (strict_url_details) {
      auto url = result->url;
      if (!result->query) {
        return;
      }
      if (IsPrivateQueryLikely(regex_util_, *result->query)) {
        return;
      }
      url = GeneratePrivateSearchURL(url, *result->query, *strict_url_details);
      VLOG(1) << "Double fetching search page: " << url;
      double_fetcher_->ScheduleDoubleFetch(url, result->SerializeToValue());
    }
  }
  auto payloads = GeneratePayloads(*last_loaded_server_config_, regex_util_,
                                   original_url_details, std::move(result));
  for (auto& payload : payloads) {
    reporter_->ScheduleSend(std::move(payload));
  }
}

}  // namespace web_discovery
