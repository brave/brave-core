/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/web_discovery/browser/web_discovery_service.h"

#include <utility>

#include "base/check.h"
#include "base/functional/bind.h"
#include "base/logging.h"
#include "brave/components/web_discovery/browser/content_scraper.h"
#include "brave/components/web_discovery/browser/payload_generator.h"
#include "brave/components/web_discovery/browser/privacy_guard.h"
#include "brave/components/web_discovery/browser/server_config_loader.h"
#include "brave/components/web_discovery/browser/url_extractor.h"
#include "brave/components/web_discovery/common/features.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "third_party/abseil-cpp/absl/strings/str_format.h"

namespace web_discovery {

namespace {
constexpr base::TimeDelta kAliveCheckInterval = base::Minutes(1);
constexpr size_t kMinPageCountForAliveMessage = 2;
}  // namespace

WebDiscoveryService::WebDiscoveryService(
    PrefService* local_state,
    PrefService* profile_prefs,
    base::FilePath user_data_dir,
    scoped_refptr<network::SharedURLLoaderFactory> shared_url_loader_factory,
    brave_search::BackupResultsService* backup_results_service)
    : local_state_(local_state),
      profile_prefs_(profile_prefs),
      user_data_dir_(user_data_dir),
      shared_url_loader_factory_(shared_url_loader_factory),
      backup_results_service_(backup_results_service) {
  CHECK(backup_results_service);
  pref_change_registrar_.Init(profile_prefs);
  pref_change_registrar_.Add(
      kWebDiscoveryEnabled,
      base::BindRepeating(&WebDiscoveryService::OnEnabledChange,
                          base::Unretained(this)));

  if (IsWebDiscoveryEnabled()) {
    Start();
  }
}

WebDiscoveryService::~WebDiscoveryService() = default;

void WebDiscoveryService::RegisterLocalStatePrefs(
    PrefRegistrySimple* registry) {
  registry->RegisterTimePref(kPatternsRetrievalTime, {});
}

void WebDiscoveryService::RegisterProfilePrefs(PrefRegistrySimple* registry) {
  registry->RegisterDictionaryPref(kAnonymousCredentialsDict);
  registry->RegisterStringPref(kCredentialRSAPrivateKey, {});
  registry->RegisterListPref(kScheduledDoubleFetches.value());
  registry->RegisterListPref(kScheduledReports.value());
  registry->RegisterDictionaryPref(kUsedBasenameCounts);
  registry->RegisterDictionaryPref(kPageCounts);
}

void WebDiscoveryService::Shutdown() {
  Stop();
  pref_change_registrar_.RemoveAll();
}

void WebDiscoveryService::Start() {
  if (!server_config_loader_) {
    server_config_loader_ = std::make_unique<ServerConfigLoader>(
        local_state_, user_data_dir_, shared_url_loader_factory_.get(),
        base::BindRepeating(&WebDiscoveryService::OnConfigChange,
                            base::Unretained(this)),
        base::BindRepeating(&WebDiscoveryService::OnPatternsLoaded,
                            base::Unretained(this)));
    server_config_loader_->LoadConfigs();
  }
  if (!credential_manager_) {
    credential_manager_ = std::make_unique<CredentialManager>(
        profile_prefs_, shared_url_loader_factory_.get(),
        server_config_loader_.get());
  }
  if (!url_extractor_) {
    url_extractor_ = std::make_unique<URLExtractor>();
  }
}

void WebDiscoveryService::Stop() {
  alive_message_timer_.Stop();
  reporter_ = nullptr;
  double_fetcher_ = nullptr;
  content_scraper_ = nullptr;
  credential_manager_ = nullptr;
  server_config_loader_ = nullptr;
  url_extractor_ = nullptr;
}

void WebDiscoveryService::ClearPrefs() {
  profile_prefs_->ClearPref(kAnonymousCredentialsDict);
  profile_prefs_->ClearPref(kCredentialRSAPrivateKey);
  profile_prefs_->ClearPref(kScheduledDoubleFetches.value());
  profile_prefs_->ClearPref(kScheduledReports.value());
  profile_prefs_->ClearPref(kUsedBasenameCounts);
  profile_prefs_->ClearPref(kPageCounts);
}

bool WebDiscoveryService::IsWebDiscoveryEnabled() const {
  return profile_prefs_->GetBoolean(kWebDiscoveryEnabled);
}

void WebDiscoveryService::OnEnabledChange() {
  if (IsWebDiscoveryEnabled()) {
    Start();
  } else {
    Stop();
    ClearPrefs();
  }
}

void WebDiscoveryService::OnConfigChange() {
  credential_manager_->JoinGroups();
}

void WebDiscoveryService::OnPatternsLoaded() {
  if (!content_scraper_) {
    content_scraper_ = ContentScraper::Create(server_config_loader_.get(),
                                              url_extractor_.get());
  }
  if (!double_fetcher_) {
    double_fetcher_ = std::make_unique<DoubleFetcher>(
        profile_prefs_.get(), shared_url_loader_factory_.get(),
        backup_results_service_.get(),
        base::BindRepeating(&WebDiscoveryService::OnDoubleFetched,
                            base::Unretained(this)));
  }
  if (!reporter_) {
    reporter_ = std::make_unique<Reporter>(
        profile_prefs_.get(), shared_url_loader_factory_.get(),
        credential_manager_.get(), server_config_loader_.get());
  }
  MaybeSendAliveMessage();
}

void WebDiscoveryService::OnDoubleFetched(
    const GURL& url,
    const base::Value& associated_data,
    std::optional<std::string> response_body) {
  if (!response_body) {
    return;
  }

  if (features::ShouldUseV2Patterns()) {
    content_scraper_->ParseAndScrapePageV2(
        url, *response_body,
        base::BindOnce(&WebDiscoveryService::OnContentScraped,
                       base::Unretained(this), true));
  } else {
    auto prev_scrape_result = PageScrapeResult::FromValue(associated_data);
    if (!prev_scrape_result) {
      return;
    }
    content_scraper_->ParseAndScrapePage(
        url, true, std::move(prev_scrape_result), *response_body,
        base::BindOnce(&WebDiscoveryService::OnContentScraped,
                       base::Unretained(this), true));
  }
}

bool WebDiscoveryService::ShouldExtractFromPage(
    const GURL& url,
    content::RenderFrameHost* render_frame_host) {
  if (!content_scraper_ || !url_extractor_) {
    return false;
  }

  bool result = false;
  bool should_update_page_count = false;

  if (features::ShouldUseV2Patterns()) {
    auto extract_result = url_extractor_->IdentifyURL(url);
    should_update_page_count =
        !extract_result || !extract_result->details->is_search_engine;
    if (extract_result) {
      VLOG(1) << "URL matched pattern "
              << RelevantSiteToID(extract_result->details->site).value_or("")
              << ": " << url;
      // Check for private query using the extracted query
      if (extract_result->details->is_search_engine) {
        result = extract_result->query &&
                 !IsPrivateQueryLikely(*extract_result->query);
      } else {
        result = !ShouldDropURL(url);
      }
    }
  } else {
    // Use patterns for v1
    const auto* matching_url_details =
        server_config_loader_->GetLastPatterns().GetMatchingURLPattern(url,
                                                                       false);
    should_update_page_count =
        !matching_url_details || !matching_url_details->is_search_engine;
    if (matching_url_details) {
      VLOG(1) << "URL matched pattern " << matching_url_details->id << ": "
              << url;
      result = !ShouldDropURL(url);
    }
  }

  // Update page count once if needed
  if (should_update_page_count && !current_page_count_hour_key_.empty()) {
    ScopedDictPrefUpdate page_count_update(profile_prefs_, kPageCounts);
    auto existing_count =
        page_count_update->FindInt(current_page_count_hour_key_).value_or(0);
    page_count_update->Set(current_page_count_hour_key_, existing_count + 1);
  }

  return result;
}

void WebDiscoveryService::StartExtractingFromPage(
    const GURL& url,
    mojo::Remote<mojom::DocumentExtractor> document_extractor) {
  auto remote_id =
      document_extractor_remotes_.Add(std::move(document_extractor));

  // For v2 patterns, immediately schedule double fetch
  if (features::ShouldUseV2Patterns()) {
    auto extract_result = url_extractor_->IdentifyURL(url);
    GURL double_fetch_url = url;
    if (extract_result && extract_result->query &&
        extract_result->details->is_search_engine) {
      double_fetch_url = GeneratePrivateSearchURL(
          url, *extract_result->query,
          extract_result->details->private_query_prefix);
    }
    double_fetcher_->ScheduleDoubleFetch(double_fetch_url, base::Value());
    return;
  }

  // We use a WeakPtr within the ContentScraper for callbacks from the renderer,
  // so using Unretained is fine here.
  CHECK(content_scraper_);
  content_scraper_->ScrapePage(
      url, false, document_extractor_remotes_.Get(remote_id),
      base::BindOnce(&WebDiscoveryService::OnContentScraped,
                     base::Unretained(this), false));
}

void WebDiscoveryService::OnContentScraped(
    bool is_strict,
    std::unique_ptr<PageScrapeResult> result) {
  if (!result) {
    return;
  }

  std::vector<base::Value::Dict> payloads;
  if (features::ShouldUseV2Patterns()) {
    CHECK(is_strict);
    const auto& patterns_group = server_config_loader_->GetLastV2Patterns();
    payloads =
        GenerateQueryPayloadsV2(server_config_loader_->GetLastServerConfig(),
                                patterns_group, std::move(result));
  } else {
    const auto& patterns = server_config_loader_->GetLastPatterns();
    auto* original_url_details =
        patterns.GetMatchingURLPattern(result->url, is_strict);
    if (!original_url_details) {
      return;
    }
    if (!is_strict && original_url_details->is_search_engine) {
      auto* strict_url_details =
          patterns.GetMatchingURLPattern(result->url, true);
      if (strict_url_details) {
        auto url = result->url;
        if (!result->query) {
          return;
        }
        if (IsPrivateQueryLikely(*result->query)) {
          return;
        }
        url = GeneratePrivateSearchURL(
            url, *result->query, strict_url_details->search_template_prefix);
        VLOG(1) << "Double fetching search page: " << url;
        double_fetcher_->ScheduleDoubleFetch(url, result->SerializeToValue());
      }
    }
    payloads =
        GenerateQueryPayloads(server_config_loader_->GetLastServerConfig(),
                              original_url_details, std::move(result));
  }
  for (auto& payload : payloads) {
    reporter_->ScheduleSend(std::move(payload));
  }
}

bool WebDiscoveryService::UpdatePageCountStartTime() {
  auto now = base::Time::Now();
  if (!current_page_count_start_time_.is_null() &&
      (now - current_page_count_start_time_) < base::Hours(1)) {
    return false;
  }
  base::Time::Exploded exploded;
  now.UTCExplode(&exploded);
  exploded.millisecond = 0;
  exploded.second = 0;
  exploded.minute = 0;
  if (!base::Time::FromUTCExploded(exploded, &current_page_count_start_time_)) {
    return false;
  }
  current_page_count_hour_key_ =
      absl::StrFormat("%04d%02d%02d%02d", exploded.year, exploded.month,
                      exploded.day_of_month, exploded.hour);
  return true;
}

void WebDiscoveryService::MaybeSendAliveMessage() {
  if (!alive_message_timer_.IsRunning()) {
    alive_message_timer_.Start(
        FROM_HERE, kAliveCheckInterval,
        base::BindRepeating(&WebDiscoveryService::MaybeSendAliveMessage,
                            base::Unretained(this)));
  }
  if (!UpdatePageCountStartTime()) {
    return;
  }
  ScopedDictPrefUpdate update(profile_prefs_, kPageCounts);
  for (auto it = update->begin(); it != update->end();) {
    if (it->first == current_page_count_hour_key_) {
      it++;
      continue;
    }
    if (it->second.is_int() && static_cast<size_t>(it->second.GetInt()) >=
                                   kMinPageCountForAliveMessage) {
      reporter_->ScheduleSend(GenerateAlivePayload(
          server_config_loader_->GetLastServerConfig(), it->first));
    }
    it = update->erase(it);
  }
}

}  // namespace web_discovery
