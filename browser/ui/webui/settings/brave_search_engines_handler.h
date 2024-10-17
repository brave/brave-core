/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_WEBUI_SETTINGS_BRAVE_SEARCH_ENGINES_HANDLER_H_
#define BRAVE_BROWSER_UI_WEBUI_SETTINGS_BRAVE_SEARCH_ENGINES_HANDLER_H_

#include "base/values.h"
#include "chrome/browser/ui/webui/settings/search_engines_handler.h"

namespace settings {

class BraveSearchEnginesHandler : public SearchEnginesHandler {
 public:
  explicit BraveSearchEnginesHandler(Profile* profile);
  ~BraveSearchEnginesHandler() override;
  BraveSearchEnginesHandler(const BraveSearchEnginesHandler&) = delete;
  BraveSearchEnginesHandler& operator=(const BraveSearchEnginesHandler&) =
      delete;

 private:
  // SearchEnginesHandler overrides:
  void RegisterMessages() override;
  void OnModelChanged() override;
  base::Value::Dict GetSearchEnginesList() override;

  base::Value::List GetPrivateSearchEnginesList();
  void HandleGetPrivateSearchEnginesList(const base::Value::List& args);

  void HandleSetDefaultPrivateSearchEngine(const base::Value::List& args);
};

}  // namespace settings

#endif  // BRAVE_BROWSER_UI_WEBUI_SETTINGS_BRAVE_SEARCH_ENGINES_HANDLER_H_
