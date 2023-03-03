/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_SEARCH_ENGINES_PRIVATE_WINDOW_SEARCH_ENGINE_PROVIDER_SERVICE_ANDROID_H_
#define BRAVE_BROWSER_SEARCH_ENGINES_PRIVATE_WINDOW_SEARCH_ENGINE_PROVIDER_SERVICE_ANDROID_H_

#include "base/memory/raw_ptr.h"
#include "base/scoped_observation.h"
#include "components/keyed_service/core/keyed_service.h"
#include "components/search_engines/template_url_service.h"
#include "components/search_engines/template_url_service_observer.h"

class Profile;

// Cache current default template url data to
// kSyncedDefaultPrivateSearchProviderData whenever private window's default
// provider. Although kSyncedDefaultPrivateSearchProviderData is stored in
// original profile's prefs, updating is done here because
// NormalWindowSearchEngineProviderServiceAndroid doesn't know about private
// window's default provider change.
class PrivateWindowSearchEngineProviderServiceAndroid
    : public TemplateURLServiceObserver,
      public KeyedService {
 public:
  explicit PrivateWindowSearchEngineProviderServiceAndroid(
      Profile* otr_profile);
  ~PrivateWindowSearchEngineProviderServiceAndroid() override;
  PrivateWindowSearchEngineProviderServiceAndroid(
      const PrivateWindowSearchEngineProviderServiceAndroid&) = delete;
  PrivateWindowSearchEngineProviderServiceAndroid& operator=(
      const PrivateWindowSearchEngineProviderServiceAndroid&) = delete;

 private:
  // TemplateURLServiceObserver overrides:
  void OnTemplateURLServiceChanged() override;

  // KeyedService overrides:
  void Shutdown() override;

  raw_ptr<Profile> otr_profile_ = nullptr;
  base::ScopedObservation<TemplateURLService, TemplateURLServiceObserver>
      observation_{this};
};

#endif  // BRAVE_BROWSER_SEARCH_ENGINES_PRIVATE_WINDOW_SEARCH_ENGINE_PROVIDER_SERVICE_ANDROID_H_
