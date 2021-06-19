/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_SEARCH_ENGINES_SEARCH_ENGINE_TRACKER_H_
#define BRAVE_BROWSER_SEARCH_ENGINES_SEARCH_ENGINE_TRACKER_H_

#include "base/memory/singleton.h"
#include "base/scoped_observer.h"
#include "components/keyed_service/content/browser_context_keyed_service_factory.h"
#include "components/keyed_service/core/keyed_service.h"
#include "components/prefs/pref_member.h"
#include "components/search_engines/template_url_service.h"
#include "components/search_engines/template_url_service_observer.h"
#include "url/gurl.h"

// Exposed for tests.
constexpr char kDefaultSearchEngineMetric[] = "Brave.Search.DefaultEngine.4";

// Note: append-only enumeration! Never remove any existing values, as this enum
// is used to bucket a UMA histogram, and removing values breaks that.
enum class SearchEngineP3A {
  kOther,
  kGoogle,
  kDuckDuckGo,
  kStartpage,
  kBing,
  kQwant,
  kYandex,
  kEcosia,
  kBrave,
  kMaxValue = kBrave,
};

class SearchEngineTrackerFactory : public BrowserContextKeyedServiceFactory {
 public:
  static SearchEngineTrackerFactory* GetInstance();

 private:
  friend struct base::DefaultSingletonTraits<SearchEngineTrackerFactory>;
  SearchEngineTrackerFactory();
  ~SearchEngineTrackerFactory() override;

  SearchEngineTrackerFactory(const SearchEngineTrackerFactory&) = delete;
  SearchEngineTrackerFactory& operator=(const SearchEngineTrackerFactory&) =
      delete;

  // BrowserContextKeyedServiceFactory overrides:
  KeyedService* BuildServiceInstanceFor(
      content::BrowserContext* context) const override;
  bool ServiceIsCreatedWithBrowserContext() const override;
};

// Records P3A metrics when default search engine changes.
class SearchEngineTracker : public KeyedService,
                            public TemplateURLServiceObserver {
 public:
  explicit SearchEngineTracker(TemplateURLService* template_url_service);
  ~SearchEngineTracker() override;

  SearchEngineTracker(const SearchEngineTracker&) = delete;
  SearchEngineTracker& operator=(const SearchEngineTracker&) = delete;

 private:
  // TemplateURLServiceObserver overrides:
  void OnTemplateURLServiceChanged() override;

  ScopedObserver<TemplateURLService, TemplateURLServiceObserver> observer_{
      this};

  // Keeping this to check for changes in |OnTemplateURLServiceChanged|.
  GURL default_search_url_;

  TemplateURLService* template_url_service_;
};

#endif  // BRAVE_BROWSER_SEARCH_ENGINES_SEARCH_ENGINE_TRACKER_H_
