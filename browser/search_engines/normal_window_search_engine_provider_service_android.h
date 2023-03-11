/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_SEARCH_ENGINES_NORMAL_WINDOW_SEARCH_ENGINE_PROVIDER_SERVICE_ANDROID_H_
#define BRAVE_BROWSER_SEARCH_ENGINES_NORMAL_WINDOW_SEARCH_ENGINE_PROVIDER_SERVICE_ANDROID_H_

#include "base/callback_list.h"
#include "base/memory/raw_ptr.h"
#include "components/keyed_service/core/keyed_service.h"
#include "components/prefs/pref_member.h"

class Profile;

// When previously used private default search provider is not included
// in the updated default provider list, this class add it to TemplateURLService
// to make usable.
class NormalWindowSearchEngineProviderServiceAndroid : public KeyedService {
 public:
  explicit NormalWindowSearchEngineProviderServiceAndroid(Profile* profile);
  ~NormalWindowSearchEngineProviderServiceAndroid() override;

  NormalWindowSearchEngineProviderServiceAndroid(
      const NormalWindowSearchEngineProviderServiceAndroid&) = delete;
  NormalWindowSearchEngineProviderServiceAndroid& operator=(
      const NormalWindowSearchEngineProviderServiceAndroid&) = delete;

 private:
  // KeyedService overrides:
  void Shutdown() override;

  void OnTemplateURLServiceLoaded();
  void PrepareInitialPrivateSearchProvider();

  raw_ptr<Profile> profile_ = nullptr;
  base::CallbackListSubscription template_url_service_subscription_;
};

#endif  // BRAVE_BROWSER_SEARCH_ENGINES_NORMAL_WINDOW_SEARCH_ENGINE_PROVIDER_SERVICE_ANDROID_H_
