/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_SEARCH_ENGINES_PRIVATE_WINDOW_SEARCH_ENGINE_PROVIDER_SERVICE_H_
#define BRAVE_BROWSER_SEARCH_ENGINES_PRIVATE_WINDOW_SEARCH_ENGINE_PROVIDER_SERVICE_H_

#include <string>

#include "base/scoped_observation.h"
#include "brave/browser/search_engines/private_window_search_engine_provider_service_base.h"
#include "components/prefs/pref_member.h"
#include "components/search_engines/template_url_service.h"
#include "components/search_engines/template_url_service_observer.h"

class PrivateWindowSearchEngineProviderService
    : public PrivateWindowSearchEngineProviderServiceBase,
      public TemplateURLServiceObserver {
 public:
  explicit PrivateWindowSearchEngineProviderService(Profile* otr_profile);
  ~PrivateWindowSearchEngineProviderService() override;
  PrivateWindowSearchEngineProviderService(
      const PrivateWindowSearchEngineProviderService&) = delete;
  PrivateWindowSearchEngineProviderService& operator=(
      const PrivateWindowSearchEngineProviderService&) = delete;

 private:
  // Configure appropriate provider and prefs.
  void UpdateExtensionPrefsAndProvider();

  // TemplateURLServiceObserver overrides:
  void OnTemplateURLServiceChanged() override;

  // PrivateWindowSearchEngineProviderServiceBase overrides:
  void Initialize() override;
  void Shutdown() override;

  void OnPreferenceChanged(const std::string& pref_name);
  void ConfigurePrivateWindowSearchEngineProvider();

  StringPrefMember private_search_provider_guid_;
  base::ScopedObservation<TemplateURLService, TemplateURLServiceObserver>
      observation_{this};
};

#endif  // BRAVE_BROWSER_SEARCH_ENGINES_PRIVATE_WINDOW_SEARCH_ENGINE_PROVIDER_SERVICE_H_
