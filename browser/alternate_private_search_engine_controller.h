/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_ALTERNATE_PRIVATE_SEARCH_ENGINE_CONTROLLER_H_
#define BRAVE_BROWSER_ALTERNATE_PRIVATE_SEARCH_ENGINE_CONTROLLER_H_

#include <memory>
#include <string>

#include "base/macros.h"
#include "components/prefs/pref_member.h"
#include "components/search_engines/template_url_service_observer.h"

class Profile;
class TemplateURL;
class TemplateURLService;

class AlternatePrivateSearchEngineController
    : public TemplateURLServiceObserver {
 public:
  static void Create(Profile* profile);

 private:
  explicit AlternatePrivateSearchEngineController(Profile* profile);
  ~AlternatePrivateSearchEngineController() override;

  void SetAlternateDefaultPrivateSearchEngine();
  void SetNormalModeDefaultSearchEngineAsDefaultPrivateSearchProvider();

  void ConfigureAlternatePrivateSearchEngineProvider();

  void OnPreferenceChanged(const std::string& pref_name);

  // TemplateURLServiceObserver overrides:
  void OnTemplateURLServiceChanged() override;
  void OnTemplateURLServiceShuttingDown() override;

  std::unique_ptr<TemplateURL> private_search_engine_url_;
  BooleanPrefMember use_alternate_private_search_engine_enabled_;

  Profile* profile_;
  TemplateURLService* template_url_service_;

  DISALLOW_COPY_AND_ASSIGN(AlternatePrivateSearchEngineController);
};


#endif  // BRAVE_BROWSER_ALTERNATE_PRIVATE_SEARCH_ENGINE_CONTROLLER_H_
