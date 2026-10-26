/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_PEEK_PUBLIC_PEEK_CONTROLLER_H_
#define BRAVE_BROWSER_UI_PEEK_PUBLIC_PEEK_CONTROLLER_H_

#include <memory>

#include "ui/base/unowned_user_data/scoped_unowned_user_data.h"

class BrowserWindowInterface;
class PeekShieldsPanelProvider;

namespace content {
struct OpenURLParams;
class WebContents;
}  // namespace content

// Per-window controller for Peek: a floating card over the active tab that
// shows a link without leaving the current page. Only one Peek is shown per
// window; opening another replaces it.
//
// Peek only hosts browsing. Anything that needs a tab, e.g. permission
// prompts, wallet panels, popups or fullscreen, moves the Peek into a tab
// first. See //brave/browser/ui/peek/SECURITY.md.
class PeekController {
 public:
  DECLARE_USER_DATA(PeekController);

  // Returns the controller of `browser`, or nullptr if Peek isn't available
  // for it (feature disabled or non-normal window).
  static PeekController* From(BrowserWindowInterface* browser);

  // Returns the controller showing `web_contents`, or nullptr if it isn't
  // shown in Peek.
  static PeekController* FromPeekContents(content::WebContents* web_contents);

  PeekController(const PeekController&) = delete;
  PeekController& operator=(const PeekController&) = delete;
  virtual ~PeekController();

  // Sets what the Shields button of the Peek card shows. Must be called before
  // the first `Open()`.
  virtual void SetShieldsPanelProvider(
      std::unique_ptr<PeekShieldsPanelProvider> provider) = 0;

  // Whether a link in `source` can be opened in Peek: it must be a tab of this
  // window, other than a sidebar web panel.
  virtual bool CanOpenFrom(content::WebContents* source) = 0;

  // Loads `params.url` from `source` in a new Peek. The disposition in
  // `params` is ignored. Returns false, without opening anything, unless
  // `CanOpenFrom(source)` and `peek::CanOpenInPeek(params)`.
  virtual bool Open(content::WebContents& source,
                    const content::OpenURLParams& params) = 0;

  // Moves the Peek contents into a tab right after the active tab.
  virtual void OpenAsTab() = 0;

  virtual void Close() = 0;
  virtual bool IsShowing() const = 0;
  virtual content::WebContents* GetWebContents() = 0;

 protected:
  explicit PeekController(BrowserWindowInterface& browser);

  // Records whether `web_contents` is shown by this controller, for
  // `FromPeekContents()`.
  void SetShowingContents(content::WebContents* web_contents, bool showing);

 private:
  ui::ScopedUnownedUserData<PeekController> scoped_unowned_user_data_;
};

#endif  // BRAVE_BROWSER_UI_PEEK_PUBLIC_PEEK_CONTROLLER_H_
