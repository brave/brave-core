/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_PRIVATE_WINDOW_SEARCH_ENGINE_PRIVATE_CONTROLLER_H_
#define BRAVE_BROWSER_PRIVATE_WINDOW_SEARCH_ENGINE_PRIVATE_CONTROLLER_H_

#include "brave/browser/search_engine_provider_controller_base.h"

class PrivateWindowSearchEngineProviderController
    : public SearchEngineProviderControllerBase {
 public:
  explicit PrivateWindowSearchEngineProviderController(Profile* profile);
  ~PrivateWindowSearchEngineProviderController() override;

 private:
  // TemplateURLServiceObserver overrides:
  void OnTemplateURLServiceChanged() override;

  // SearchEngineProviderControllerBase overrides:
  void ConfigureSearchEngineProvider() override;

  void ChangeToAlternativeSearchEngineProvider();
  void ChangeToNormalWindowSearchEngineProvider();

  DISALLOW_COPY_AND_ASSIGN(PrivateWindowSearchEngineProviderController);
};

#endif  // BRAVE_BROWSER_PRIVATE_WINDOW_SEARCH_ENGINE_PROVIDER_CONTROLLER_H_
