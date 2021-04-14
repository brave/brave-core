/* This Source Code Form is subject to the terms of the Mozilla Public
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

// TODO(simonhong): Rename this to GuestWindowSearchEngineProviderService.
class SearchEngineProviderService : public KeyedService,
                                    public TemplateURLServiceObserver {
 public:
  explicit SearchEngineProviderService(Profile* otr_profile);
  ~SearchEngineProviderService() override;

  // TemplateURLServiceObserver overrides:
  void OnTemplateURLServiceChanged() override;

 private:
  // Points off the record profile.
  Profile* otr_profile_;
  TemplateURLService* template_url_service_;
  std::unique_ptr<TemplateURL> alternative_search_engine_url_;
  BooleanPrefMember use_alternative_search_engine_provider_;
  base::ScopedObservation<TemplateURLService, TemplateURLServiceObserver>
      observation_{this};
  bool ignore_template_url_service_changing_ = false;

  bool UseAlternativeSearchEngineProvider() const;
  void ChangeToAlternativeSearchEngineProvider();
  void ChangeToNormalWindowSearchEngineProvider();
  void OnPreferenceChanged(const std::string& pref_name);

  DISALLOW_COPY_AND_ASSIGN(SearchEngineProviderService);
};

#endif  // BRAVE_BROWSER_SEARCH_ENGINES_SEARCH_ENGINE_PROVIDER_SERVICE_H_
