/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_PEEK_PUBLIC_PEEK_CONTROLLER_H_
#define BRAVE_BROWSER_UI_PEEK_PUBLIC_PEEK_CONTROLLER_H_

#include <memory>

#include "ui/base/unowned_user_data/scoped_unowned_user_data.h"

class BrowserWindowInterface;

namespace content {
struct OpenURLParams;
class WebContents;
}  // namespace content

// Per-window controller for Peek: a floating card over the active tab that
// shows a link without leaving the current page. Only one Peek is shown per
// window; opening another replaces it.
class PeekController {
 public:
  DECLARE_USER_DATA(PeekController);

  // Returns the controller of `browser`, or nullptr if Peek isn't available
  // for it (feature disabled or non-normal window).
  static PeekController* From(BrowserWindowInterface* browser);

  PeekController(const PeekController&) = delete;
  PeekController& operator=(const PeekController&) = delete;
  virtual ~PeekController();

  // Loads `params.url` in a new Peek. The disposition in `params` is ignored.
  virtual void Open(const content::OpenURLParams& params) = 0;

  // Shows `contents` in a new Peek. Used for contents the renderer already
  // created, e.g. from a target="_blank" link.
  virtual void OpenWebContents(
      std::unique_ptr<content::WebContents> contents) = 0;

  // Moves the Peek contents into a tab right after the active tab.
  virtual void OpenAsTab() = 0;

  virtual void Close() = 0;
  virtual bool IsShowing() const = 0;
  virtual content::WebContents* GetWebContents() = 0;

 protected:
  explicit PeekController(BrowserWindowInterface& browser);

 private:
  ui::ScopedUnownedUserData<PeekController> scoped_unowned_user_data_;
};

#endif  // BRAVE_BROWSER_UI_PEEK_PUBLIC_PEEK_CONTROLLER_H_
