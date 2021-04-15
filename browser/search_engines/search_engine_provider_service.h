/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_SEARCH_ENGINES_SEARCH_ENGINE_PROVIDER_SERVICE_H_
#define BRAVE_BROWSER_SEARCH_ENGINES_SEARCH_ENGINE_PROVIDER_SERVICE_H_

#include <memory>
#include <string>

#include "base/scoped_observation.h"
#include "components/keyed_service/core/keyed_service.h"
#include "components/prefs/pref_member.h"
#include "components/search_engines/template_url_service.h"
#include "components/search_engines/template_url_service_observer.h"

class Profile;
class TemplateURL;
class TemplateURLService;

// Handles DDG toggle button in NTP of guest window.
class SearchEngineProviderService : public KeyedService,
                                    public TemplateURLServiceObserver {
 public:
  explicit SearchEngineProviderService(Profile* profile);
  ~SearchEngineProviderService() override;
  SearchEngineProviderService(const SearchEngineProviderService&) = delete;
  SearchEngineProviderService& operator=(const SearchEngineProviderService&) =
      delete;

  // TemplateURLServiceObserver overrides:
  void OnTemplateURLServiceChanged() override;

 private:
  Profile* profile_;
  TemplateURLService* template_url_service_;
  std::unique_ptr<TemplateURL> alternative_search_engine_url_;
  BooleanPrefMember use_alternative_search_engine_provider_;
  base::ScopedObservation<TemplateURLService, TemplateURLServiceObserver>
      observation_{this};
  bool ignore_template_url_service_changing_ = false;

  bool UseAlternativeSearchEngineProvider() const;
  void ChangeToAlternativeSearchEngineProvider();
  void ChangeToDefaultSearchEngineProvider();
  void OnPreferenceChanged(const std::string& pref_name);
};

#endif  // BRAVE_BROWSER_SEARCH_ENGINES_SEARCH_ENGINE_PROVIDER_SERVICE_H_
