/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_SEARCH_ENGINES_SEARCH_ENGINE_PROVIDER_SERVICE_H_
#define BRAVE_BROWSER_SEARCH_ENGINES_SEARCH_ENGINE_PROVIDER_SERVICE_H_

#include <memory>
#include <string>

#include "components/keyed_service/core/keyed_service.h"
#include "components/prefs/pref_member.h"

class Profile;
class TemplateURL;
class TemplateURLService;

class SearchEngineProviderService : public KeyedService {
 public:
  explicit SearchEngineProviderService(Profile* otr_profile);
  ~SearchEngineProviderService() override;

  SearchEngineProviderService(const SearchEngineProviderService&) = delete;
  SearchEngineProviderService& operator=(const SearchEngineProviderService&) =
      delete;

 protected:
  // If subclass want to know and configure according to prefs change, override
  // this.
  virtual void OnUseAlternativeSearchEngineProviderChanged() {}

  bool UseAlternativeSearchEngineProvider() const;
  void ChangeToAlternativeSearchEngineProvider();
  void ChangeToNormalWindowSearchEngineProvider();

  bool ShouldUseExtensionSearchProvider() const;
  void UseExtensionSearchProvider();

  // Points off the record profile.
  Profile* otr_profile_ = nullptr;
  // Service for original profile of |otr_profile_|.
  TemplateURLService* original_template_url_service_ = nullptr;
  // Service for off the record profile.
  TemplateURLService* otr_template_url_service_ = nullptr;

 private:
  void OnPreferenceChanged(const std::string& pref_name);
  bool CouldAddExtensionTemplateURL(const TemplateURL* url);

  std::unique_ptr<TemplateURL> alternative_search_engine_url_;
  BooleanPrefMember use_alternative_search_engine_provider_;
};

#endif  // BRAVE_BROWSER_SEARCH_ENGINES_SEARCH_ENGINE_PROVIDER_SERVICE_H_
