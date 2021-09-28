/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/search_engines/search_engine_tracker.h"

#include "base/metrics/histogram_macros.h"
#include "base/strings/string_util.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/search_engines/template_url_service_factory.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"

namespace {

// Deduces the search engine from |type|, if nothing is found - from |url|.
// Not all engines added by Brave are present in |SearchEngineType| enumeration.
void RecordSearchEngineP3A(const GURL& search_engine_url,
                           SearchEngineType type) {
  SearchEngineP3A answer = SearchEngineP3A::kOther;

  if (type == SEARCH_ENGINE_GOOGLE) {
    answer = SearchEngineP3A::kGoogle;
  } else if (type == SEARCH_ENGINE_DUCKDUCKGO) {
    answer = SearchEngineP3A::kDuckDuckGo;
  } else if (type == SEARCH_ENGINE_BING) {
    answer = SearchEngineP3A::kBing;
  } else if (type == SEARCH_ENGINE_QWANT) {
    answer = SearchEngineP3A::kQwant;
  } else if (type == SEARCH_ENGINE_YANDEX) {
    answer = SearchEngineP3A::kYandex;
  } else if (type == SEARCH_ENGINE_ECOSIA) {
    answer = SearchEngineP3A::kEcosia;
  } else if (type == SEARCH_ENGINE_OTHER) {
    if (base::EndsWith(search_engine_url.host(), "startpage.com",
                       base::CompareCase::INSENSITIVE_ASCII)) {
      answer = SearchEngineP3A::kStartpage;
    } else if (base::EndsWith(search_engine_url.host(), "brave.com",
                              base::CompareCase::INSENSITIVE_ASCII)) {
      answer = SearchEngineP3A::kBrave;
    }
  }

  UMA_HISTOGRAM_ENUMERATION(kDefaultSearchEngineMetric, answer);
}

SearchEngineSwitchP3A SearchEngineSwitchP3AMapAnswer(const GURL& to,
                                                     const GURL& from) {
  SearchEngineSwitchP3A answer;

  DCHECK(from.is_valid());
  DCHECK(to.is_valid());

  if (from.DomainIs("brave.com")) {
    // Switching away from Brave Search.
    if (to.DomainIs("google.com")) {
      answer = SearchEngineSwitchP3A::kBraveToGoogle;
    } else if (to.DomainIs("duckduckgo.com")) {
      answer = SearchEngineSwitchP3A::kBraveToDDG;
    } else {
      answer = SearchEngineSwitchP3A::kBraveToOther;
    }
  } else if (to.DomainIs("brave.com")) {
    // Switching to Brave Search.
    if (from.DomainIs("google.com")) {
      answer = SearchEngineSwitchP3A::kGoogleToBrave;
    } else if (from.DomainIs("duckduckgo.com")) {
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
  return base::Singleton<SearchEngineTrackerFactory>::get();
}

SearchEngineTrackerFactory::SearchEngineTrackerFactory()
    : BrowserContextKeyedServiceFactory(
          "SearchEngineTracker",
          BrowserContextDependencyManager::GetInstance()) {
  DependsOn(TemplateURLServiceFactory::GetInstance());
}

SearchEngineTrackerFactory::~SearchEngineTrackerFactory() {}

KeyedService* SearchEngineTrackerFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  auto* template_url_service = TemplateURLServiceFactory::GetForProfile(
      Profile::FromBrowserContext(context));
  if (template_url_service) {
    return new SearchEngineTracker(template_url_service);
  }
  return nullptr;
}

bool SearchEngineTrackerFactory::ServiceIsCreatedWithBrowserContext() const {
  return true;
}

SearchEngineTracker::SearchEngineTracker(
    TemplateURLService* template_url_service)
    : template_url_service_(template_url_service) {
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
      RecordSearchEngineP3A(url, template_url->GetEngineType(search_terms));
      RecordSwitchP3A(url);
    }
  }
}

SearchEngineTracker::~SearchEngineTracker() {}

void SearchEngineTracker::OnTemplateURLServiceChanged() {
  const TemplateURL* template_url =
      template_url_service_->GetDefaultSearchProvider();
  if (template_url) {
    const SearchTermsData& search_terms =
        template_url_service_->search_terms_data();
    const GURL& url = template_url->GenerateSearchURL(search_terms);
    if (url != default_search_url_) {
      RecordSearchEngineP3A(url, template_url->GetEngineType(search_terms));
    }
    RecordSwitchP3A(url);
  }
}

void SearchEngineTracker::RecordSwitchP3A(const GURL& url) {
  auto answer = SearchEngineSwitchP3A::kNoSwitch;

  if (url.is_valid() && url != previous_search_url_) {
    answer = SearchEngineSwitchP3AMapAnswer(url, previous_search_url_);
    previous_search_url_ = url;
  }

  UMA_HISTOGRAM_ENUMERATION(kSwitchSearchEngineMetric, answer);
}
