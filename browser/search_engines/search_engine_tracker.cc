/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/search_engines/search_engine_tracker.h"

#include <memory>

#include "base/functional/bind.h"
#include "base/metrics/histogram_macros.h"
#include "base/no_destructor.h"
#include "base/strings/string_util.h"
#include "brave/components/brave_ads/core/public/prefs/pref_names.h"
#include "brave/components/brave_search_conversion/features.h"
#include "brave/components/brave_search_conversion/p3a.h"
#include "brave/components/brave_search_conversion/utils.h"
#include "brave/components/constants/pref_names.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/search_engines/template_url_service_factory.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "components/pref_registry/pref_registry_syncable.h"

namespace {

// Preference name switch events are stored under.
constexpr char kSwitchSearchEngineP3AStorage[] =
    "brave.search.p3a_default_switch";
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
  } else if (type == SEARCH_ENGINE_BRAVE) {
    result = SearchEngineP3A::kBrave;
  } else if (type == SEARCH_ENGINE_OTHER) {
    if (base::EndsWith(search_engine_url.host(), "startpage.com",
                       base::CompareCase::INSENSITIVE_ASCII)) {
      result = SearchEngineP3A::kStartpage;
    } else if (base::EndsWith(search_engine_url.host(), "brave.com",
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

void SearchEngineTrackerFactory::RegisterProfilePrefs(
    user_prefs::PrefRegistrySyncable* registry) {
  registry->RegisterListPref(kSwitchSearchEngineP3AStorage);
}

SearchEngineTracker::SearchEngineTracker(
    TemplateURLService* template_url_service,
    PrefService* profile_prefs,
    PrefService* local_state)
    : switch_record_(profile_prefs, kSwitchSearchEngineP3AStorage),
      local_state_(local_state),
      profile_prefs_(profile_prefs),
      template_url_service_(template_url_service) {
  DCHECK(template_url_service);
  DCHECK(profile_prefs);
  DCHECK(local_state);

  observer_.Observe(template_url_service_);
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

#if BUILDFLAG(ENABLE_EXTENSIONS)
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
    }
    RecordSwitchP3A(url);
  }
}

#if BUILDFLAG(ENABLE_EXTENSIONS)
void SearchEngineTracker::RecordWebDiscoveryEnabledP3A() {
  UMA_HISTOGRAM_BOOLEAN(kWebDiscoveryEnabledMetric,
                        profile_prefs_->GetBoolean(kWebDiscoveryEnabled));
  UMA_HISTOGRAM_BOOLEAN(kWebDiscoveryAndAdsMetric,
                        profile_prefs_->GetBoolean(kWebDiscoveryEnabled) &&
                            profile_prefs_->GetBoolean(
                                brave_ads::prefs::kOptedInToNotificationAds));
}
#endif

void SearchEngineTracker::RecordSwitchP3A(const GURL& url) {
  // Default to the last recorded switch so when we're called
  // at start-up we initialize the histogram with whatever we
  // remember from the previous run.
  auto answer = SearchEngineSwitchP3A::kNoSwitch;
  auto last = switch_record_.GetLatest();
  if (last) {
    answer = static_cast<SearchEngineSwitchP3A>(last.value());
    DCHECK(answer <= SearchEngineSwitchP3A::kMaxValue);
  }

  if (url.is_valid() && url != previous_search_url_) {
    // The default url has been switched, record that instead.
    answer = SearchEngineSwitchP3AMapAnswer(url, previous_search_url_);
    previous_search_url_ = url;
    switch_record_.Add(static_cast<int>(answer));

    if (url.DomainIs(kBraveDomain)) {
      brave_search_conversion::p3a::RecordDefaultEngineConversion(local_state_);
    }
  }

  if (brave_search_conversion::IsBraveSearchConversionFeatureEnabled() ||
      base::FeatureList::IsEnabled(brave_search_conversion::features::kNTP)) {
    // Do not report if search conversion promo is enabled, to prevent metric
    // overlap with conversion metrics.
    UMA_HISTOGRAM_EXACT_LINEAR(kSwitchSearchEngineMetric, INT_MAX - 1, 8);
    return;
  }
  UMA_HISTOGRAM_ENUMERATION(kSwitchSearchEngineMetric, answer);
}
