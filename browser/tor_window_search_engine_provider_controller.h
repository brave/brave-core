/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_TOR_WINDOW_SEARCH_ENGINE_PROVIDER_CONTROLLER_H_
#define BRAVE_BROWSER_TOR_WINDOW_SEARCH_ENGINE_PROVIDER_CONTROLLER_H_

#include "brave/browser/search_engine_provider_controller_base.h"
#include "components/prefs/pref_member.h"

class TorWindowSearchEngineProviderController
    : public SearchEngineProviderControllerBase {
 public:
  explicit TorWindowSearchEngineProviderController(Profile* profile);
  ~TorWindowSearchEngineProviderController() override;

 private:
  // TemplateURLServiceObserver overrides:
  void OnTemplateURLServiceChanged() override;

  void ConfigureSearchEngineProvider() override {}

  int GetInitialSearchEngineProvider() const;

  IntegerPrefMember alternative_search_engine_provider_in_tor_;

  DISALLOW_COPY_AND_ASSIGN(TorWindowSearchEngineProviderController);
};


#endif  // BRAVE_BROWSER_TOR_WINDOW_SEARCH_ENGINE_PROVIDER_CONTROLLER_H_
