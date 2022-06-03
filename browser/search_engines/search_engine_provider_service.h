/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_SEARCH_ENGINES_SEARCH_ENGINE_PROVIDER_SERVICE_H_
#define BRAVE_BROWSER_SEARCH_ENGINES_SEARCH_ENGINE_PROVIDER_SERVICE_H_

#include <string>

#include "base/memory/raw_ptr.h"
#include "components/keyed_service/core/keyed_service.h"

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
  bool ShouldUseExtensionSearchProvider() const;
  void UseExtensionSearchProvider();

  // Points off the record profile.
  raw_ptr<Profile> otr_profile_ = nullptr;
  // Service for original profile of |otr_profile_|.
  raw_ptr<TemplateURLService> original_template_url_service_ = nullptr;
  // Service for off the record profile.
  raw_ptr<TemplateURLService> otr_template_url_service_ = nullptr;

 private:
  void OnPreferenceChanged(const std::string& pref_name);
  bool CouldAddExtensionTemplateURL(const TemplateURL* url);
};

#endif  // BRAVE_BROWSER_SEARCH_ENGINES_SEARCH_ENGINE_PROVIDER_SERVICE_H_
