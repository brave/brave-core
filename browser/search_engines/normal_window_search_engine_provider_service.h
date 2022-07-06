/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_SEARCH_ENGINES_NORMAL_WINDOW_SEARCH_ENGINE_PROVIDER_SERVICE_H_
#define BRAVE_BROWSER_SEARCH_ENGINES_NORMAL_WINDOW_SEARCH_ENGINE_PROVIDER_SERVICE_H_

#include "base/callback_list.h"
#include "components/keyed_service/core/keyed_service.h"

class Profile;

// Set default prefs for private search provider as it's stored in normal
// profile.
class NormalWindowSearchEngineProviderService : public KeyedService {
 public:
  explicit NormalWindowSearchEngineProviderService(Profile* profile);
  ~NormalWindowSearchEngineProviderService() override;

  NormalWindowSearchEngineProviderService(
      const NormalWindowSearchEngineProviderService&) = delete;
  NormalWindowSearchEngineProviderService& operator=(
      const NormalWindowSearchEngineProviderService&) = delete;

 private:
  void OnTemplateURLServiceLoaded(Profile* profile);

  base::CallbackListSubscription template_url_service_subscription_;
};

#endif  // BRAVE_BROWSER_SEARCH_ENGINES_NORMAL_WINDOW_SEARCH_ENGINE_PROVIDER_SERVICE_H_
