/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_SEARCH_ENGINES_TOR_WINDOW_SEARCH_ENGINE_PROVIDER_SERVICE_H_
#define BRAVE_BROWSER_SEARCH_ENGINES_TOR_WINDOW_SEARCH_ENGINE_PROVIDER_SERVICE_H_

#include <memory>

#include "base/scoped_observation.h"
#include "brave/browser/search_engines/search_engine_provider_service.h"
#include "components/search_engines/template_url_service.h"
#include "components/search_engines/template_url_service_observer.h"

class PrefService;
struct TemplateURLData;

// The purpose of this service for tor is making user changed search engine
// provider persist across the sessions.
// Also, BraveProfileManager::SetNonPersonalProfilePrefs() overrides for it.
class TorWindowSearchEngineProviderService : public SearchEngineProviderService,
                                             public TemplateURLServiceObserver {
 public:
  explicit TorWindowSearchEngineProviderService(Profile* otr_profile);
  ~TorWindowSearchEngineProviderService() override;

  TorWindowSearchEngineProviderService(
      const TorWindowSearchEngineProviderService&) = delete;
  TorWindowSearchEngineProviderService& operator=(
      const TorWindowSearchEngineProviderService&) = delete;

  // SearchEngineProviderService overrides:
  void Shutdown() override;

  // TemplateURLServiceObserver overrides:
  void OnTemplateURLServiceChanged() override;

 private:
  std::unique_ptr<TemplateURLData> GetInitialSearchEngineProvider(
      PrefService* prefs) const;
  void ConfigureSearchEngineProvider();

  std::unique_ptr<TemplateURL> alternative_search_engine_url_for_tor_;
  base::ScopedObservation<TemplateURLService, TemplateURLServiceObserver>
      observation_{this};
};

#endif  // BRAVE_BROWSER_SEARCH_ENGINES_TOR_WINDOW_SEARCH_ENGINE_PROVIDER_SERVICE_H_
