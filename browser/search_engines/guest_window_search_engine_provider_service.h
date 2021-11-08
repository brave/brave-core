/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_SEARCH_ENGINES_GUEST_WINDOW_SEARCH_ENGINE_PROVIDER_SERVICE_H_
#define BRAVE_BROWSER_SEARCH_ENGINES_GUEST_WINDOW_SEARCH_ENGINE_PROVIDER_SERVICE_H_

#include "brave/browser/search_engines/search_engine_provider_service.h"
#include "components/search_engines/template_url_service_observer.h"

// This controller is only used by non Qwant region.
// This class manage's newtab's ddg toggle button state.
// Toggle button state should be reflected setting value.
// Ex, when user changes from ddg to others, toggle button should be off and
// vice versa.
class GuestWindowSearchEngineProviderService
    : public SearchEngineProviderService,
      public TemplateURLServiceObserver {
 public:
  explicit GuestWindowSearchEngineProviderService(Profile* otr_profile);
  GuestWindowSearchEngineProviderService(
      const GuestWindowSearchEngineProviderService&) = delete;
  GuestWindowSearchEngineProviderService& operator=(
      const GuestWindowSearchEngineProviderService&) = delete;
  ~GuestWindowSearchEngineProviderService() override;

 private:
  FRIEND_TEST_ALL_PREFIXES(SearchEngineProviderServiceTest,
                           GuestWindowControllerTest);

  // TemplateURLServiceObserver overrides:
  void OnTemplateURLServiceChanged() override;

  void OnUseAlternativeSearchEngineProviderChanged() override;

  bool ignore_template_url_service_changing_ = false;
};


#endif  // BRAVE_BROWSER_SEARCH_ENGINES_GUEST_WINDOW_SEARCH_ENGINE_PROVIDER_SERVICE_H_
