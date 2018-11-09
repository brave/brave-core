/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_GUEST_WINDOW_SEARCH_ENGINE_PROVIDER_CONTROLLER_H_
#define BRAVE_BROWSER_GUEST_WINDOW_SEARCH_ENGINE_PROVIDER_CONTROLLER_H_

#include "brave/browser/search_engine_provider_controller_base.h"

// This controller is only used by non Qwant region.
// This class manage's newtab's ddg toggle button state.
// Toggle button state should be reflected setting value.
// Ex, when user changeds from ddg to others, toggle button shouldbe off and
// vice versa.
class GuestWindowSearchEngineProviderController
    : public SearchEngineProviderControllerBase {
 public:
  explicit GuestWindowSearchEngineProviderController(Profile* otr_profile);
  ~GuestWindowSearchEngineProviderController() override;

 private:
  // TemplateURLServiceObserver overrides:
  void OnTemplateURLServiceChanged() override;

  void ConfigureSearchEngineProvider() override;

  bool ignore_template_url_service_changing_ = false;

  DISALLOW_COPY_AND_ASSIGN(GuestWindowSearchEngineProviderController);
};


#endif  // BRAVE_BROWSER_GUEST_WINDOW_SEARCH_ENGINE_PROVIDER_CONTROLLER_H_
