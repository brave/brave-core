/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_SPEEDREADER_SPEEDREADER_BUBBLE_CONTROLLER_H_
#define BRAVE_BROWSER_UI_SPEEDREADER_SPEEDREADER_BUBBLE_CONTROLLER_H_

#include "brave/browser/ui/views/speedreader/speedreader_bubble_global.h"
#include "content/public/browser/web_contents_user_data.h"

namespace content {
class WebContents;
}  // namespace content

namespace speedreader {

class SpeedreaderBubbleView;

class SpeedreaderBubbleController
    : public content::WebContentsUserData<SpeedreaderBubbleController> {
 public:
  SpeedreaderBubbleController(const SpeedreaderBubbleController&) = delete;
  SpeedreaderBubbleController& operator=(const SpeedreaderBubbleController&) =
      delete;

  static SpeedreaderBubbleController* Get(content::WebContents* web_contents);

  // Displays speedreader information
  void ShowBubble(bool is_enabled);

  // Hides speedreader information
  void HideBubble();

  // returns nullptr if no bubble corruntly shown
  SpeedreaderBubbleView* speedreader_bubble_view() const;

  // Handler for when the bubble is dismissed.
  void OnBubbleClosed();

 protected:
  explicit SpeedreaderBubbleController(content::WebContents* web_contents);

 private:
  SpeedreaderBubbleController();

  SpeedreaderBubbleView* speedreader_bubble_ = nullptr;
  friend class content::WebContentsUserData<SpeedreaderBubbleController>;

  content::WebContents* web_contents_;
  WEB_CONTENTS_USER_DATA_KEY_DECL();
};

}  // namespace speedreader

#endif  // BRAVE_BROWSER_UI_SPEEDREADER_SPEEDREADER_BUBBLE_CONTROLLER_H_
