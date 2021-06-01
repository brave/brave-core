/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/speedreader/speedreader_bubble_controller.h"

#include "brave/browser/ui/brave_browser_window.h"
#include "brave/browser/ui/speedreader/speedreader_bubble_view.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/browser/ui/views/location_bar/location_bar_bubble_delegate_view.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_user_data.h"

namespace speedreader {

// static
SpeedreaderBubbleController* SpeedreaderBubbleController::Get(
    content::WebContents* web_contents) {
  SpeedreaderBubbleController::CreateForWebContents(web_contents);
  SpeedreaderBubbleController* controller =
      SpeedreaderBubbleController::FromWebContents(web_contents);
  return controller;
}

SpeedreaderBubbleController::SpeedreaderBubbleController() = default;

SpeedreaderBubbleController::SpeedreaderBubbleController(
    content::WebContents* web_contents)
    : web_contents_(web_contents) {}

SpeedreaderBubbleView* SpeedreaderBubbleController::speedreader_bubble_view()
    const {
  return speedreader_bubble_;
}

void SpeedreaderBubbleController::OnBubbleClosed() {
  speedreader_bubble_ = nullptr;
}

// Displays speedreader information
void SpeedreaderBubbleController::ShowBubble(bool is_enabled) {
  Browser* browser = chrome::FindBrowserWithWebContents(web_contents_);
  DCHECK(browser);
  speedreader_bubble_ =
      static_cast<BraveBrowserWindow*>(browser->window())
          ->ShowSpeedreaderBubble(web_contents_, this, is_enabled);
}

// Hides speedreader information
void SpeedreaderBubbleController::HideBubble() {
  if (speedreader_bubble_) {
    speedreader_bubble_->Hide();
    speedreader_bubble_ = nullptr;
  }
}

WEB_CONTENTS_USER_DATA_KEY_IMPL(SpeedreaderBubbleController)

}  // namespace speedreader
