/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_WEBUI_BRAVE_ADBLOCK_UI_H_
#define BRAVE_BROWSER_UI_WEBUI_BRAVE_ADBLOCK_UI_H_

#include <memory>
#include <string>

#include "brave/browser/ui/webui/basic_ui.h"

class PrefChangeRegistrar;

class BraveAdblockUI : public BasicUI {
 public:
  BraveAdblockUI(content::WebUI* web_ui, const std::string& host);
  ~BraveAdblockUI() override;

 private:
  // BasicUI overrides:
  void UpdateWebUIProperties() override;

  void CustomizeWebUIProperties(content::RenderFrameHost* render_frame_host);
  void OnPreferenceChanged();

  std::unique_ptr<PrefChangeRegistrar> pref_change_registrar_;

  DISALLOW_COPY_AND_ASSIGN(BraveAdblockUI);
};

#endif  // BRAVE_BROWSER_UI_WEBUI_BRAVE_ADBLOCK_UI_H_
