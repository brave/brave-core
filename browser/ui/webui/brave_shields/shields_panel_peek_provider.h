/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_WEBUI_BRAVE_SHIELDS_SHIELDS_PANEL_PEEK_PROVIDER_H_
#define BRAVE_BROWSER_UI_WEBUI_BRAVE_SHIELDS_SHIELDS_PANEL_PEEK_PROVIDER_H_

#include <memory>

#include "brave/browser/ui/peek/public/peek_shields_panel_provider.h"

// Provides the Shields panel (`ShieldsPanelUI`) to Peek.
class ShieldsPanelPeekProvider : public PeekShieldsPanelProvider {
 public:
  ShieldsPanelPeekProvider();
  ShieldsPanelPeekProvider(const ShieldsPanelPeekProvider&) = delete;
  ShieldsPanelPeekProvider& operator=(const ShieldsPanelPeekProvider&) = delete;
  ~ShieldsPanelPeekProvider() override;

  // PeekShieldsPanelProvider:
  std::unique_ptr<WebUIBubbleManager> CreateBubbleManager(
      BrowserWindowInterface& browser) override;
  void SetPanelTarget(content::WebContents* panel_contents,
                      content::WebContents* target) override;
};

#endif  // BRAVE_BROWSER_UI_WEBUI_BRAVE_SHIELDS_SHIELDS_PANEL_PEEK_PROVIDER_H_
