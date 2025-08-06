/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_WEBUI_SETTINGS_BRAVE_SEARCH_ENGINES_HANDLER_H_
#define BRAVE_BROWSER_UI_WEBUI_SETTINGS_BRAVE_SEARCH_ENGINES_HANDLER_H_

#include "base/memory/raw_ptr.h"
#include "base/scoped_observation.h"
#include "base/values.h"
#include "brave/components/brave_origin/brave_origin_state.h"
#include "chrome/browser/ui/webui/settings/search_engines_handler.h"
#include "components/prefs/pref_change_registrar.h"
#include "components/regional_capabilities/regional_capabilities_service.h"

namespace settings {

class BraveSearchEnginesHandler : public SearchEnginesHandler,
                                  public BraveOriginStateObserver {
 public:
  explicit BraveSearchEnginesHandler(
      Profile* profile,
      regional_capabilities::RegionalCapabilitiesService*
          regional_capabilities);
  ~BraveSearchEnginesHandler() override;
  BraveSearchEnginesHandler(const BraveSearchEnginesHandler&) = delete;
  BraveSearchEnginesHandler& operator=(const BraveSearchEnginesHandler&) =
      delete;

  // BraveOriginStateObserver:
  void OnBraveOriginStatusChanged() override;

 private:
  // SearchEnginesHandler overrides:
  void RegisterMessages() override;
  void OnModelChanged() override;
  base::Value::Dict GetSearchEnginesList() override;

  base::Value::List GetPrivateSearchEnginesList();
  void HandleGetPrivateSearchEnginesList(const base::Value::List& args);

  void HandleSetDefaultPrivateSearchEngine(const base::Value::List& args);

  void HandleGetIsWebDiscoveryHidden(const base::Value::List& args);
  void OnWebDiscoveryEnabledChanged();

  static bool IsWebDiscoveryHidden(Profile* profile);

  raw_ptr<regional_capabilities::RegionalCapabilitiesService>
      regional_capabilities_ = nullptr;
  PrefChangeRegistrar profile_pref_change_registrar_;
  base::ScopedObservation<BraveOriginState, BraveOriginStateObserver>
      brave_origin_state_observation_{this};
};

}  // namespace settings

#endif  // BRAVE_BROWSER_UI_WEBUI_SETTINGS_BRAVE_SEARCH_ENGINES_HANDLER_H_
