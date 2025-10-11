/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/search_engines/search_engine_tracker.h"

#include <memory>

#include "base/check.h"
#include "base/functional/bind.h"
#include "base/metrics/histogram_macros.h"
#include "base/no_destructor.h"
#include "base/strings/string_util.h"
#include "base/time/time.h"
#include "brave/components/brave_ads/core/public/prefs/pref_names.h"
#include "brave/components/brave_search_conversion/p3a.h"
#include "brave/components/brave_search_conversion/utils.h"
#include "brave/components/constants/pref_names.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/search_engines/template_url_service_factory.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "components/pref_registry/pref_registry_syncable.h"

namespace {

// Preference name for last switch report timestamp (new location in local
// state).
constexpr char kLastSwitchReportPref[] = "brave.search.last_switch_report";
// Old preference name for switch events (for migration from profile prefs).
constexpr char kSwitchSearchEngineP3AStorage[] =
    "brave.search.p3a_default_switch";
// Report interval for P3A switch metrics (1 hour)
constexpr base::TimeDelta kReportInterval = base::Hours(1);
// Minimum time between reports (1 day)
constexpr base::TimeDelta kMinReportInterval = base::Days(1);
constexpr char kBraveDomain[] = "brave.com";
constexpr char kGoogleDomain[] = "google.com";
constexpr char kDDGDomain[] = "duckduckgo.com";

// Deduces the search engine from |type|, if nothing is found - from |url|.
// Not all engines added by Brave are present in |SearchEngineType| enumeration.
SearchEngineP3A GetSearchEngineProvider(const GURL& search_engine_url,
                                        SearchEngineType type) {
  SearchEngineP3A result = SearchEngineP3A::kOther;
  if (type == SEARCH_ENGINE_GOOGLE) {
    result = SearchEngineP3A::kGoogle;
  } else if (type == SEARCH_ENGINE_DUCKDUCKGO) {
    result = SearchEngineP3A::kDuckDuckGo;
  } else if (type == SEARCH_ENGINE_BING) {
    result = SearchEngineP3A::kBing;
  } else if (type == SEARCH_ENGINE_QWANT) {
    result = SearchEngineP3A::kQwant;
  } else if (type == SEARCH_ENGINE_YANDEX) {
    result = SearchEngineP3A::kYandex;
  } else if (type == SEARCH_ENGINE_ECOSIA) {
    result = SearchEngineP3A::kEcosia;
  } else if (type == SEARCH_ENGINE_DAUM) {
    result = SearchEngineP3A::kDaum;
  } else if (type == SEARCH_ENGINE_NAVER) {
    result = SearchEngineP3A::kNaver;
  } else if (type == SEARCH_ENGINE_YAHOO &&
             search_engine_url.host().ends_with(".jp")) {
    result = SearchEngineP3A::kYahooJP;
  } else if (type == SEARCH_ENGINE_BRAVE) {
    result = SearchEngineP3A::kBrave;
  } else if (type == SEARCH_ENGINE_STARTPAGE) {
    result = SearchEngineP3A::kStartpage;
  } else if (type == SEARCH_ENGINE_OTHER) {
    if (base::EndsWith(search_engine_url.host(), "brave.com",
                       base::CompareCase::INSENSITIVE_ASCII)) {
      result = SearchEngineP3A::kBrave;
    }
  }
  return result;
}

SearchEngineSwitchP3A SearchEngineSwitchP3AMapAnswer(const GURL& to,
                                                     const GURL& from) {
  SearchEngineSwitchP3A answer;

  DCHECK(from.is_valid());
  DCHECK(to.is_valid());

  if (from.DomainIs(kBraveDomain)) {
    // Switching away from Brave Search.
    if (to.DomainIs(kGoogleDomain)) {
      answer = SearchEngineSwitchP3A::kBraveToGoogle;
    } else if (to.DomainIs(kDDGDomain)) {
      answer = SearchEngineSwitchP3A::kBraveToDDG;
    } else {
      answer = SearchEngineSwitchP3A::kBraveToOther;
    }
  } else if (to.DomainIs(kBraveDomain)) {
    // Switching to Brave Search.
    if (from.DomainIs(kGoogleDomain)) {
      answer = SearchEngineSwitchP3A::kGoogleToBrave;
    } else if (from.DomainIs(kDDGDomain)) {
      answer = SearchEngineSwitchP3A::kDDGToBrave;
    } else {
      answer = SearchEngineSwitchP3A::kOtherToBrave;
    }
  } else {
    // Any other transition.
    answer = SearchEngineSwitchP3A::kOtherToOther;
  }

  return answer;
}

}  // namespace

// static
SearchEngineTrackerFactory* SearchEngineTrackerFactory::GetInstance() {
  static base::NoDestructor<SearchEngineTrackerFactory> instance;
  return instance.get();
}

SearchEngineTrackerFactory::SearchEngineTrackerFactory()
    : BrowserContextKeyedServiceFactory(
          "SearchEngineTracker",
          BrowserContextDependencyManager::GetInstance()) {
  DependsOn(TemplateURLServiceFactory::GetInstance());
}

SearchEngineTrackerFactory::~SearchEngineTrackerFactory() = default;

SearchEngineTracker* SearchEngineTrackerFactory::GetForBrowserContext(
    content::BrowserContext* context) {
  return static_cast<SearchEngineTracker*>(
      GetInstance()->GetServiceForBrowserContext(context, true));
}

std::unique_ptr<KeyedService>
SearchEngineTrackerFactory::BuildServiceInstanceForBrowserContext(
    content::BrowserContext* context) const {
  auto* profile = Profile::FromBrowserContext(context);
  auto* template_url_service =
      TemplateURLServiceFactory::GetForProfile(profile);
  auto* profile_prefs = profile->GetPrefs();
  auto* local_state = g_browser_process->local_state();
  if (!template_url_service || !profile_prefs || !local_state) {
    return nullptr;
  }
  return std::make_unique<SearchEngineTracker>(template_url_service,
                                               profile_prefs, local_state);
}

bool SearchEngineTrackerFactory::ServiceIsCreatedWithBrowserContext() const {
  return true;
}

void SearchEngineTrackerFactory::RegisterLocalStatePrefs(
    PrefRegistrySimple* registry) {
  registry->RegisterTimePref(kLastSwitchReportPref, {});
}

void SearchEngineTrackerFactory::RegisterProfilePrefsForMigration(
    user_prefs::PrefRegistrySyncable* registry) {
  // Register old pref for migration
  registry->RegisterListPref(kSwitchSearchEngineP3AStorage);
}

SearchEngineTracker::SearchEngineTracker(
    TemplateURLService* template_url_service,
    PrefService* profile_prefs,
    PrefService* local_state)
    : local_state_(local_state),
      profile_prefs_(profile_prefs),
      template_url_service_(template_url_service) {
  DCHECK(template_url_service);
  DCHECK(profile_prefs);
  DCHECK(local_state);

  observer_.Observe(template_url_service_);

  // Migrate any old prefs
  MigrateObsoletePrefs();

  const TemplateURL* template_url =
      template_url_service_->GetDefaultSearchProvider();

  // Record the initial P3A.
  if (template_url) {
    const SearchTermsData& search_terms =
        template_url_service_->search_terms_data();

    const GURL url = template_url->GenerateSearchURL(search_terms);
    if (!url.is_empty()) {
      default_search_url_ = url;
      previous_search_url_ = url;
      current_default_engine_ = GetSearchEngineProvider(
          url, template_url->GetEngineType(search_terms));
      UMA_HISTOGRAM_ENUMERATION(kDefaultSearchEngineMetric,
                                current_default_engine_);
      RecordSwitchP3A(url);
    }
  }

#if BUILDFLAG(ENABLE_EXTENSIONS) || BUILDFLAG(ENABLE_WEB_DISCOVERY_NATIVE)
  RecordWebDiscoveryEnabledP3A();
  pref_change_registrar_.Init(profile_prefs);
  pref_change_registrar_.Add(
      kWebDiscoveryEnabled,
      base::BindRepeating(&SearchEngineTracker::RecordWebDiscoveryEnabledP3A,
                          base::Unretained(this)));
  pref_change_registrar_.Add(
      brave_ads::prefs::kOptedInToNotificationAds,
      base::BindRepeating(&SearchEngineTracker::RecordWebDiscoveryEnabledP3A,
                          base::Unretained(this)));
#endif
}

SearchEngineTracker::~SearchEngineTracker() = default;

void SearchEngineTracker::RecordLocationBarQuery() {
  if (current_default_engine_ == SearchEngineP3A::kBrave) {
    brave_search_conversion::p3a::RecordLocationBarQuery(local_state_);
  }
}

void SearchEngineTracker::OnTemplateURLServiceChanged() {
  const TemplateURL* template_url =
      template_url_service_->GetDefaultSearchProvider();
  if (template_url) {
    const SearchTermsData& search_terms =
        template_url_service_->search_terms_data();
    const GURL& url = template_url->GenerateSearchURL(search_terms);
    if (url != default_search_url_) {
      SearchEngineP3A last_default_engine = current_default_engine_;
      current_default_engine_ = GetSearchEngineProvider(
          url, template_url->GetEngineType(search_terms));

      UMA_HISTOGRAM_ENUMERATION(kDefaultSearchEngineMetric,
                                current_default_engine_);

      default_search_url_ = url;

      if (last_default_engine != current_default_engine_ &&
          last_default_engine == SearchEngineP3A::kBrave) {
        brave_search_conversion::p3a::RecordDefaultEngineChurn(local_state_);
      }

#if BUILDFLAG(ENABLE_EXTENSIONS) || BUILDFLAG(ENABLE_WEB_DISCOVERY_NATIVE)
      // Update web discovery default engine metric when search engine changes
      RecordWebDiscoveryEnabledP3A();
#endif
    }
    RecordSwitchP3A(url);
  }
}

#if BUILDFLAG(ENABLE_EXTENSIONS) || BUILDFLAG(ENABLE_WEB_DISCOVERY_NATIVE)
void SearchEngineTracker::RecordWebDiscoveryEnabledP3A() {
  bool enabled = profile_prefs_->GetBoolean(kWebDiscoveryEnabled);
  UMA_HISTOGRAM_BOOLEAN(kWebDiscoveryEnabledMetric, enabled);
  UMA_HISTOGRAM_BOOLEAN(
      kWebDiscoveryAndAdsMetric,
      enabled && profile_prefs_->GetBoolean(
                     brave_ads::prefs::kOptedInToNotificationAds));

  // Record web discovery default engine metric
  int answer = INT_MAX - 1;
  if (enabled) {
    answer = static_cast<int>(current_default_engine_);
  }
  UMA_HISTOGRAM_EXACT_LINEAR(kWebDiscoveryDefaultEngineMetric, answer,
                             static_cast<int>(SearchEngineP3A::kMaxValue) + 1);
}
#endif

void SearchEngineTracker::RecordSwitchP3A(const GURL& url) {
  const base::Time now = base::Time::Now();
  const base::Time last_report = local_state_->GetTime(kLastSwitchReportPref);

  // Determine the appropriate "no switch" value based on current search engine
  auto answer = url.is_valid() && url.DomainIs(kBraveDomain)
                    ? SearchEngineSwitchP3A::kNoSwitchBrave
                    : SearchEngineSwitchP3A::kNoSwitchNonBrave;
  bool should_report = false;

  if (previous_search_url_.is_valid() && url.is_valid() &&
      url != previous_search_url_) {
    // The default url has been switched, record that.
    answer = SearchEngineSwitchP3AMapAnswer(url, previous_search_url_);
    previous_search_url_ = url;
    should_report = true;

    if (url.DomainIs(kBraveDomain)) {
      brave_search_conversion::p3a::RecordDefaultEngineConversion(local_state_);
    }
  } else if (last_report.is_null() ||
             (now - last_report) >= kMinReportInterval) {
    // Report if we haven't reported before or it's been >= 1 day
    should_report = true;
  }

  // Set up timer for next report regardless of whether we report now
  switch_report_timer_.Start(
      FROM_HERE, now + kReportInterval,
      base::BindOnce(&SearchEngineTracker::RecordSwitchP3A,
                     base::Unretained(this), url));

  if (should_report) {
    // Update the last report timestamp
    local_state_->SetTime(kLastSwitchReportPref, now);

    UMA_HISTOGRAM_ENUMERATION(kSwitchSearchEngineMetric, answer);
  }
}

void SearchEngineTracker::MigrateObsoletePrefs() {
  profile_prefs_->ClearPref(kSwitchSearchEngineP3AStorage);
}
