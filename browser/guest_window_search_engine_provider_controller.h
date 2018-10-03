/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_GUEST_WINDOW_SEARCH_ENGINE_PROVIDER_CONTROLLER_H_
#define BRAVE_BROWSER_GUEST_WINDOW_SEARCH_ENGINE_PROVIDER_CONTROLLER_H_

#include "brave/browser/search_engine_provider_controller_base.h"

class GuestWindowSearchEngineProviderController
    : public SearchEngineProviderControllerBase {
 public:
  explicit GuestWindowSearchEngineProviderController(Profile* profile);
  ~GuestWindowSearchEngineProviderController() override;

 private:
  // TemplateURLServiceObserver overrides:
  void OnTemplateURLServiceChanged() override;

  void ConfigureSearchEngineProvider() override;

  bool ignore_template_url_service_changing_ = false;

  DISALLOW_COPY_AND_ASSIGN(GuestWindowSearchEngineProviderController);
};


#endif  // BRAVE_BROWSER_GUEST_WINDOW_SEARCH_ENGINE_PROVIDER_CONTROLLER_H_
