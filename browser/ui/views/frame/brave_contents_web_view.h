/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_FRAME_BRAVE_CONTENTS_WEB_VIEW_H_
#define BRAVE_BROWSER_UI_VIEWS_FRAME_BRAVE_CONTENTS_WEB_VIEW_H_

#include "chrome/browser/ui/views/frame/contents_web_view.h"

class ViewShadow;

class BraveContentsWebView : public ContentsWebView {
 public:
  explicit BraveContentsWebView(content::BrowserContext* browser_context);
  ~BraveContentsWebView() override;

 protected:
  void RenderViewReady() override;

 private:
  std::unique_ptr<ViewShadow> shadow_;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_FRAME_BRAVE_CONTENTS_WEB_VIEW_H_
