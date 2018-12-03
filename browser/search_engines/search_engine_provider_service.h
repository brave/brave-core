/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_SEARCH_ENGINES_SEARCH_ENGINE_PROVIDER_SERVICE_H_
#define BRAVE_BROWSER_SEARCH_ENGINES_SEARCH_ENGINE_PROVIDER_SERVICE_H_

#include <memory>
#include <string>

#include "base/macros.h"
#include "components/keyed_service/core/keyed_service.h"
#include "components/prefs/pref_member.h"

class Profile;
class TemplateURL;
class TemplateURLService;

class SearchEngineProviderService : public KeyedService {
 public:
  explicit SearchEngineProviderService(Profile* otr_profile);
  ~SearchEngineProviderService() override;

 protected:
  // If subclass want to know and configure according to prefs change, override
  // this.
  virtual void OnUseAlternativeSearchEngineProviderChanged() {}

  bool UseAlternativeSearchEngineProvider() const;
  void ChangeToAlternativeSearchEngineProvider();
  void ChangeToNormalWindowSearchEngineProvider();

  // Points off the record profile.
  Profile* otr_profile_;
  // Service for original profile of |otr_profile_|.
  TemplateURLService* original_template_url_service_;
  // Service for off the record profile.
  TemplateURLService* otr_template_url_service_;

  std::unique_ptr<TemplateURL> alternative_search_engine_url_;

 private:
  void OnPreferenceChanged(const std::string& pref_name);

  BooleanPrefMember use_alternative_search_engine_provider_;

  DISALLOW_COPY_AND_ASSIGN(SearchEngineProviderService);
};

#endif  // BRAVE_BROWSER_SEARCH_ENGINES_SEARCH_ENGINE_PROVIDER_SERVICE_H_
