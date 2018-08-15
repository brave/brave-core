/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_WEBUI_BRAVE_NEW_TAB_UI_H_
#define BRAVE_BROWSER_UI_WEBUI_BRAVE_NEW_TAB_UI_H_

#include <memory>
#include "brave/browser/ui/webui/basic_ui.h"

class PrefChangeRegistrar;

class BraveNewTabUI : public BasicUI {
 public:
  BraveNewTabUI(content::WebUI* web_ui, const std::string& host);
  ~BraveNewTabUI() override;

 private:
  void RenderFrameCreated(content::RenderFrameHost* render_frame_host) override;
  void CustomizeNewTabWebUIProperties(content::RenderFrameHost* render_frame_host);
  void OnPreferenceChanged();

  std::unique_ptr<PrefChangeRegistrar> pref_change_registrar_;

  DISALLOW_COPY_AND_ASSIGN(BraveNewTabUI);
};

#endif  // BRAVE_BROWSER_UI_WEBUI_BRAVE_NEW_TAB_UI_H_
