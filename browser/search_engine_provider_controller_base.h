/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_SEARCH_ENGINE_PROVIDER_CONTROLLER_BASE_H_
#define BRAVE_BROWSER_SEARCH_ENGINE_PROVIDER_CONTROLLER_BASE_H_

#include <memory>
#include <string>

#include "base/macros.h"
#include "components/prefs/pref_member.h"
#include "components/search_engines/template_url_service_observer.h"

class Profile;
class TemplateURL;
class TemplateURLService;

class SearchEngineProviderControllerBase : public TemplateURLServiceObserver {
 public:
  explicit SearchEngineProviderControllerBase(Profile* profile);
  ~SearchEngineProviderControllerBase() override;

 protected:
  virtual void ConfigureSearchEngineProvider() = 0;

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

  // |destroyer_| controls the lifecycle of this class and it is destroyed
  // by itself.
  class Destroyer;
  Destroyer* destroyer_ = nullptr;

  BooleanPrefMember use_alternative_search_engine_provider_;

  DISALLOW_COPY_AND_ASSIGN(SearchEngineProviderControllerBase);
};

#endif  // BRAVE_BROWSER_SEARCH_ENGINE_PROVIDER_CONTROLLER_BASE_H_
