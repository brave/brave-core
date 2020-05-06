/* Copyright 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_FRAME_BRAVE_BROWSER_VIEW_H_
#define BRAVE_BROWSER_UI_VIEWS_FRAME_BRAVE_BROWSER_VIEW_H_

#include <string>

#include "brave/browser/ui/tabs/brave_tab_strip_model.h"
#include "chrome/browser/ui/views/frame/browser_view.h"

class BraveBrowserView : public BrowserView {
 public:
  BraveBrowserView(std::unique_ptr<Browser> browser);
  ~BraveBrowserView() override;

  void SetStarredState(bool is_starred) override;
  void ShowUpdateChromeDialog() override;
  ShowTranslateBubbleResult ShowTranslateBubble(
      content::WebContents* web_contents,
      translate::TranslateStep step,
      const std::string& source_language,
      const std::string& target_language,
      translate::TranslateErrors::Type error_type,
      bool is_user_gesture) override;

  void StartMRUCycling() override;

 private:
  class CtrlReleaseHandler : public ui::EventHandler {
   public:
    explicit CtrlReleaseHandler(BraveTabStripModel* model,
                                BraveBrowserView* browser_view);
    ~CtrlReleaseHandler() override;

   private:
    void OnKeyEvent(ui::KeyEvent* event) override;
    BraveTabStripModel* model_;
    BraveBrowserView* browser_view_;
    DISALLOW_COPY_AND_ASSIGN(CtrlReleaseHandler);
  };

  std::unique_ptr<CtrlReleaseHandler> ctrl_released_event_handler_;

  DISALLOW_COPY_AND_ASSIGN(BraveBrowserView);
};

#endif  // BRAVE_BROWSER_UI_VIEWS_FRAME_BRAVE_BROWSER_VIEW_H_
