// Copyright (c) 2019 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_WEBUI_BRAVE_NEW_TAB_UI_H_
#define BRAVE_BROWSER_UI_WEBUI_BRAVE_NEW_TAB_UI_H_

#include <memory>
#include <string>

#include "brave/browser/ui/webui/basic_ui.h"

namespace content {
class RenderViewHost;
}

class BraveNewTabUI : public BasicUI {
 public:
  BraveNewTabUI(content::WebUI* web_ui, const std::string& host);
  ~BraveNewTabUI() override;
  void OnPreferencesChanged();
  void OnPrivatePropertiesChanged();
  void OnStatsChanged();

 private:
  // BasicUI overrides
  void UpdateWebUIProperties() override;
  void SetPreferencesWebUIProperties(content::RenderViewHost* render_view_host);
  void SetStatsWebUIProperties(content::RenderViewHost* render_view_host);
  void SetPrivateWebUIProperties(content::RenderViewHost* render_view_host);

  DISALLOW_COPY_AND_ASSIGN(BraveNewTabUI);
};

#endif  // BRAVE_BROWSER_UI_WEBUI_BRAVE_NEW_TAB_UI_H_
