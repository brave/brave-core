/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_PEEK_PUBLIC_PEEK_SHIELDS_PANEL_PROVIDER_H_
#define BRAVE_BROWSER_UI_PEEK_PUBLIC_PEEK_SHIELDS_PANEL_PROVIDER_H_

#include <memory>

class BrowserWindowInterface;
class WebUIBubbleManager;

namespace content {
class WebContents;
}  // namespace content

// Supplies the Shields panel shown by Peek's Shields button, so that Peek
// doesn't depend on the Shields WebUI.
class PeekShieldsPanelProvider {
 public:
  virtual ~PeekShieldsPanelProvider() = default;

  virtual std::unique_ptr<WebUIBubbleManager> CreateBubbleManager(
      BrowserWindowInterface& browser) = 0;

  // Makes the panel hosted in `panel_contents` control `target`. Called right
  // after the panel is shown, before its page finishes loading.
  virtual void SetPanelTarget(content::WebContents* panel_contents,
                              content::WebContents* target) = 0;
};

#endif  // BRAVE_BROWSER_UI_PEEK_PUBLIC_PEEK_SHIELDS_PANEL_PROVIDER_H_
