/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/search_engines/normal_window_search_engine_provider_service.h"

#include "brave/browser/search_engines/search_engine_provider_util.h"

NormalWindowSearchEngineProviderService::
    NormalWindowSearchEngineProviderService(Profile* profile) {
  // No-op if default provider was set to prefs.
  brave::SetDefaultPrivateSearchProvider(profile);
}

NormalWindowSearchEngineProviderService::
    ~NormalWindowSearchEngineProviderService() = default;
