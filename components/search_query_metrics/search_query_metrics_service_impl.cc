// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/search_query_metrics/search_query_metrics_service_impl.h"

#include <algorithm>
#include <string_view>
#include <utility>
#include <vector>

#include "base/json/json_writer.h"
#include "base/logging.h"
#include "base/metrics/field_trial.h"
#include "base/notreached.h"
#include "base/rand_util.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/task/sequenced_task_runner.h"
#include "base/time/time.h"
#include "base/types/cxx23_to_underlying.h"
#include "base/uuid.h"
#include "brave/components/search_query_metrics/locale/locale_util.h"
#include "brave/components/search_query_metrics/network_client/network_client_callback.h"
#include "brave/components/search_query_metrics/pref_names.h"
#include "brave/components/search_query_metrics/search_engine/search_engine_info.h"
#include "brave/components/search_query_metrics/search_engine/search_engine_util.h"
#include "brave/components/search_query_metrics/search_query_metrics_allowed_lists.h"
#include "brave/components/search_query_metrics/search_query_metrics_environment_util.h"
#include "brave/components/search_query_metrics/search_query_metrics_feature.h"
#include "brave/components/search_query_metrics/search_query_metrics_queue_item_info.h"
#include "brave/components/search_query_metrics/search_query_metrics_url_util.h"
#include "brave/components/version_info/version_info.h"
#include "build/build_config.h"
#include "components/prefs/pref_service.h"
#include "components/search_engines/default_search_manager.h"
#include "components/variations/pref_names.h"
#include "net/http/http_request_headers.h"
#include "net/http/http_status_code.h"
#include "url/gurl.h"

namespace metrics {

namespace {

constexpr base::TimeDelta kMaxRetryJitter = base::Minutes(1);

constexpr std::string_view kBookmarkEntryPoint = "bookmark";
constexpr std::string_view kDirectEntryPoint = "direct";
constexpr std::string_view kNTPEntryPoint = "ntp";
constexpr std::string_view kOmniboxHistoryEntryPoint = "omnibox_history";
constexpr std::string_view kOmniboxSuggestionEntryPoint = "omnibox_suggestion";
constexpr std::string_view kOmniboxSearchEntryPoint = "omnibox_search";
constexpr std::string_view kQuickSearchEntryPoint = "quick_search";
constexpr std::string_view kShortcutEntryPoint = "shortcut";
constexpr std::string_view kTopSiteEntryPoint = "top_site";

constexpr std::string_view kPayloadBuildChannelKey = "buildChannel";
constexpr char kPayloadAnonymizedBuildChannelValue[] = "unknown";

constexpr std::string_view kPayloadCountryKey = "country";
constexpr char kPayloadAnonymizedCountryValue[] = "--";

constexpr std::string_view kPayloadDefaultSearchEngineKey =
    "defaultSearchEngine";
constexpr char kPayloadAnonymizedDefaultSearchEngineValue[] = "Other";

constexpr std::string_view kPayloadEntryPointKey = "entryPoint";
constexpr char kPayloadAnonymizedEntryPointValue[] = "Other";

constexpr std::string_view kPayloadIsDefaultBrowserKey = "isDefaultBrowser";

constexpr std::string_view kPayloadIsFirstQueryKey = "isFirstQuery";

constexpr std::string_view kPayloadLanguageKey = "language";
constexpr char kPayloadAnonymizedLanguageValue[] = "--";

constexpr std::string_view kPayloadPlatformKey = "platform";
constexpr char kPayloadAnonymizedPlatformValue[] = "other";

constexpr std::string_view kPayloadSearchEngineKey = "searchEngine";
constexpr char kPayloadAnonymizedSearchEngineValue[] = "Other";

constexpr std::string_view kPayloadStudiesKey = "studies";
constexpr std::string_view kStudyPrefix = "BraveSearch.";

constexpr std::string_view kPayloadTransactionIdKey = "transactionId";

constexpr std::string_view kPayloadTypeKey = "type";
constexpr std::string_view kPayloadTypeValue = "query";

constexpr std::string_view kPayloadVersionNumberKey = "versionNumber";

std::optional<std::string_view> EntryPointTypeToString(
    SearchQueryMetricsEntryPointType entry_point_type) {
  switch (entry_point_type) {
    case SearchQueryMetricsEntryPointType::kBookmark: {
      return kBookmarkEntryPoint;
    }

    case SearchQueryMetricsEntryPointType::kDirect: {
      return kDirectEntryPoint;
    }

    case SearchQueryMetricsEntryPointType::kNTP: {
      return kNTPEntryPoint;
    }

    case SearchQueryMetricsEntryPointType::kOmniboxHistory: {
      return kOmniboxHistoryEntryPoint;
    }

    case SearchQueryMetricsEntryPointType::kOmniboxSuggestion: {
      return kOmniboxSuggestionEntryPoint;
    }

    case SearchQueryMetricsEntryPointType::kOmniboxSearch: {
      return kOmniboxSearchEntryPoint;
    }

    case SearchQueryMetricsEntryPointType::kQuickSearch: {
      return kQuickSearchEntryPoint;
    }

    case SearchQueryMetricsEntryPointType::kShortcut: {
      return kShortcutEntryPoint;
    }

    case SearchQueryMetricsEntryPointType::kTopSite: {
      return kTopSiteEntryPoint;
    }

    case SearchQueryMetricsEntryPointType::kOther: {
      return std::nullopt;
    }
  }

  NOTREACHED() << "Unexpected value for entry_point_type: "
               << base::to_underlying(entry_point_type);
}

std::optional<std::string_view> Language() {
  const std::string& language = CurrentLanguageCode();
  if (!kAllowedLanguages.contains(language)) {
    return std::nullopt;
  }

  return language;
}

std::optional<std::string_view> Platform() {
#if BUILDFLAG(IS_MAC)
  return "macos";
#elif BUILDFLAG(IS_WIN)
  return "windows";
#elif BUILDFLAG(IS_LINUX)
  return "linux";
#elif BUILDFLAG(IS_ANDROID)
  return "android";
#elif BUILDFLAG(IS_IOS)
  return "ios";
#else
  return std::nullopt;
#endif
}

std::optional<std::string_view> SearchEngine(const GURL& url) {
  std::optional<SearchEngineInfo> search_engine = MaybeGetSearchEngine(url);
  if (!search_engine) {
    return std::nullopt;
  }

  if (kAllowedSearchEngines.find(search_engine->name) ==
      kAllowedSearchEngines.cend()) {
    return std::nullopt;
  }

  return search_engine->name;
}

std::string TransactionId() {
  return base::Uuid::GenerateRandomV4().AsLowercaseString();
}

std::string VersionNumber() {
  std::vector<std::string> parts = base::SplitString(
      version_info::GetBraveVersionWithoutChromiumMajorVersion(), ".",
      base::KEEP_WHITESPACE, base::SPLIT_WANT_ALL);
  parts.reserve(3);

  parts.resize(3, "0");
  parts[2] = "0";

  return base::JoinString(parts, ".");
}

base::Value::Dict Studies() {
  base::FieldTrial::ActiveGroups active_field_trial_groups;
  base::FieldTrialList::GetActiveFieldTrialGroups(&active_field_trial_groups);

  base::Value::Dict studies;
  for (const auto& field_trial_group : active_field_trial_groups) {
    if (field_trial_group.trial_name.starts_with(kStudyPrefix)) {
      studies.Set(field_trial_group.trial_name, field_trial_group.group_name);
    }
  }

  return studies;
}

}  // namespace

SearchQueryMetricsServiceImpl::SearchQueryMetricsServiceImpl(
    PrefService& prefs,
    PrefService& local_state,
    TemplateURLService* template_url_service,
    std::unique_ptr<NetworkClient> network_client,
    std::unique_ptr<SearchQueryMetricsServiceDelegate> delegate)
    : prefs_(prefs),
      local_state_(local_state),
      template_url_service_(template_url_service),
      network_client_(std::move(network_client)),
      delegate_(std::move(delegate)) {
  CHECK(network_client_);
}

SearchQueryMetricsServiceImpl::~SearchQueryMetricsServiceImpl() = default;

void SearchQueryMetricsServiceImpl::MaybeReport(
    const GURL& url,
    SearchQueryMetricsEntryPointType entry_point_type) {
  const std::string payload = BuildPayload(url, entry_point_type);
  QueueReport(payload);
}

//////////////////////////////////////////////////////////////////////////////

void SearchQueryMetricsServiceImpl::QueueReport(const std::string& payload) {
  prefs_->SetTime(prefs::kLastReportedAt, base::Time::Now());

  QueueItemInfo queue_item;
  queue_item.payload = payload;
  queue_item.retry_count = 0;

  Report(std::move(queue_item));
}

void SearchQueryMetricsServiceImpl::Report(QueueItemInfo queue_item) {
  prefs_->SetTime(prefs::kLastReportedAt, base::Time::Now());

  const GURL url = GetUrl(ShouldUseStagingEnvironment());
  const std::vector<std::string> headers = {"accept: application/json"};
  const std::string content_type = "application/json";
  const std::string method = net::HttpRequestHeaders::kPostMethod;

  VLOG(1) << "[METRIC] URL Request:\n"
          << "  URL: " << url << "\n"
          << "  Content: " << queue_item.payload << "\n"
          << "  Content Type: " << content_type << "\n"
          << "  Method: " << method;

  const std::string content = queue_item.payload;

  network_client_->SendRequest(
      url, headers, content, content_type, method,
      base::BindOnce(&SearchQueryMetricsServiceImpl::ReportCallback,
                     weak_ptr_factory_.GetWeakPtr(), std::move(queue_item)));
}

void SearchQueryMetricsServiceImpl::ReportCallback(
    QueueItemInfo queue_item,
    const GURL& url,
    int response_code,
    const std::string& response_body,
    const base::flat_map<std::string, std::string>& /*response_headers*/) {
  VLOG(1) << "[METRIC] URL Response:\n"
          << "  URL: " << url << "\n"
          << "  Response Code: " << response_code << "\n"
          << "  Response: " << response_body;

  const bool success =
      response_code == net::HTTP_OK || response_code == net::HTTP_NO_CONTENT;
  if (success) {
    VLOG(1) << "[METRIC] Successfully reported search query metric";
    return;
  }

  VLOG(0) << "[METRIC] Failed to report search query metric";

  const int response_code_class = response_code / 100;
  const bool should_retry = kShouldRetryFailedReports.Get() &&
                            (response_code == net::HTTP_UNPROCESSABLE_CONTENT ||
                             response_code_class != 4);
  if (should_retry) {
    return MaybeRetry(std::move(queue_item));
  }
}

void SearchQueryMetricsServiceImpl::MaybeRetry(QueueItemInfo queue_item) {
  if (!kShouldRetryFailedReports.Get()) {
    return;
  }

  const base::TimeDelta delay =
      std::min(kInitialBackoffDelay.Get() * (1 << queue_item.retry_count),
               kMaxBackoffDelay.Get());

  queue_item.retry_count++;
  if (queue_item.retry_count > kMaxRetryCount.Get()) {
    VLOG(1) << "[METRIC] Reached maximum retry attempts. Dropping metric.";
    return;
  }

  VLOG(0) << "[METRIC] Retry reporting search query metric at "
          << base::Time::Now() + delay << " (attempt " << queue_item.retry_count
          << " of " << kMaxRetryCount.Get() << ")";

  // Randomized delay to prevent timing correlation.
  const base::TimeDelta randomized_delay =
      delay + base::RandTimeDeltaUpTo(kMaxRetryJitter);
  base::SequencedTaskRunner::GetCurrentDefault()->PostDelayedTask(
      FROM_HERE,
      base::BindOnce(&SearchQueryMetricsServiceImpl::Report,
                     weak_ptr_factory_.GetWeakPtr(), std::move(queue_item)),
      randomized_delay);
}

std::string SearchQueryMetricsServiceImpl::BuildPayload(
    const GURL& url,
    SearchQueryMetricsEntryPointType entry_point_type) const {
  std::string json;
  CHECK(base::JSONWriter::Write(
      base::Value::Dict()
          .Set(kPayloadBuildChannelKey,
               BuildChannel().value_or(kPayloadAnonymizedBuildChannelValue))
          .Set(kPayloadCountryKey,
               Country().value_or(kPayloadAnonymizedCountryValue))
          .Set(kPayloadDefaultSearchEngineKey,
               DefaultSearchEngine().value_or(
                   kPayloadAnonymizedDefaultSearchEngineValue))
          .Set(kPayloadEntryPointKey,
               EntryPointTypeToString(entry_point_type)
                   .value_or(kPayloadAnonymizedEntryPointValue))
          .Set(kPayloadIsDefaultBrowserKey, IsDefaultBrowser())
          .Set(kPayloadIsFirstQueryKey, IsFirstQuery())
          .Set(kPayloadLanguageKey,
               Language().value_or(kPayloadAnonymizedLanguageValue))
          .Set(kPayloadPlatformKey,
               Platform().value_or(kPayloadAnonymizedPlatformValue))
          .Set(kPayloadSearchEngineKey,
               SearchEngine(url).value_or(kPayloadAnonymizedSearchEngineValue))
          .Set(kPayloadStudiesKey, Studies())
          .Set(kPayloadTransactionIdKey, TransactionId())
          .Set(kPayloadTypeKey, kPayloadTypeValue)
          .Set(kPayloadVersionNumberKey, VersionNumber()),
      &json));
  return json;
}

std::optional<std::string> SearchQueryMetricsServiceImpl::BuildChannel() const {
  if (!delegate_) {
    return std::nullopt;
  }

  return delegate_->GetBuildChannelName();
}

std::optional<std::string> SearchQueryMetricsServiceImpl::Country() const {
  const std::string& country = base::ToUpperASCII(
      local_state_->GetString(variations::prefs::kVariationsCountry));
  if (!kAllowedCountries.contains(country)) {
    return std::nullopt;
  }

  return country;
}

std::optional<std::string> SearchQueryMetricsServiceImpl::DefaultSearchEngine()
    const {
  if (!template_url_service_) {
    return std::nullopt;
  }

  const TemplateURL* const template_url =
      template_url_service_->GetDefaultSearchProvider();
  if (!template_url) {
    // No default search engine.
    return std::nullopt;
  }

  std::string short_name = base::UTF16ToUTF8(template_url->short_name());
  if (!kAllowedDefaultSearchEngines.contains(short_name)) {
    return std::nullopt;
  }

  return short_name;
}

bool SearchQueryMetricsServiceImpl::IsDefaultBrowser() const {
  return delegate_ && delegate_->IsDefaultBrowser();
}

bool SearchQueryMetricsServiceImpl::IsFirstQuery() const {
  if (!prefs_->HasPrefPath(prefs::kLastReportedAt)) {
    // First search query ever.
    return true;
  }

  base::Time::Exploded now_exploded;
  base::Time::Now().UTCExplode(&now_exploded);

  base::Time::Exploded last_reported_at_exploded;
  prefs_->GetTime(prefs::kLastReportedAt)
      .UTCExplode(&last_reported_at_exploded);

  return now_exploded.year != last_reported_at_exploded.year ||
         now_exploded.month != last_reported_at_exploded.month ||
         now_exploded.day_of_month != last_reported_at_exploded.day_of_month;
}

void SearchQueryMetricsServiceImpl::Shutdown() {
  if (network_client_) {
    network_client_->CancelRequests();
  }

  weak_ptr_factory_.InvalidateWeakPtrs();
}

}  // namespace metrics
