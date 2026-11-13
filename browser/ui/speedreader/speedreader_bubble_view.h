/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_SPEEDREADER_SPEEDREADER_BUBBLE_VIEW_H_
#define BRAVE_BROWSER_UI_SPEEDREADER_SPEEDREADER_BUBBLE_VIEW_H_

class BrowserWindowInterface;

namespace speedreader {

class SpeedreaderTabHelper;
enum class SpeedreaderBubbleLocation : int;

// Interface to display a Speedreader info bubble.
// This object is responsible for its own lifetime.
class SpeedreaderBubbleView {
 public:
  virtual ~SpeedreaderBubbleView() = default;

  // Shows the bubble
  virtual void Show() = 0;

  // Closes the bubble and prevents future calls into the controller
  virtual void Hide() = 0;
};

// Shows the Speedreader bubble for `tab_helper`'s tab in `browser`, anchored to
// the location bar or to the reader mode toolbar. Returns nullptr in windows
// without views, such as a WebUI browser window.
SpeedreaderBubbleView* ShowSpeedreaderBubble(
    BrowserWindowInterface& browser,
    SpeedreaderTabHelper& tab_helper,
    SpeedreaderBubbleLocation location);

}  // namespace speedreader

#endif  // BRAVE_BROWSER_UI_SPEEDREADER_SPEEDREADER_BUBBLE_VIEW_H_
