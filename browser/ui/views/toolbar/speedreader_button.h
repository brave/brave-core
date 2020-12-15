/* Copyright 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_TOOLBAR_SPEEDREADER_BUTTON_H_
#define BRAVE_BROWSER_UI_VIEWS_TOOLBAR_SPEEDREADER_BUTTON_H_

#include "chrome/browser/ui/views/toolbar/toolbar_button.h"
#include "components/prefs/pref_change_registrar.h"

class PrefService;

namespace content {
class WebContents;
}

// Enables/disables speedreader in prefs. Also shows if the current page was
// distilled.
class SpeedreaderButton : public ToolbarButton {
 public:
  explicit SpeedreaderButton(PressedCallback callback, PrefService* prefs);
  ~SpeedreaderButton() override;

  SpeedreaderButton(const SpeedreaderButton&) = delete;
  SpeedreaderButton& operator=(const SpeedreaderButton&) = delete;

  void Update(content::WebContents* active_contents);
  void UpdateImage();

  // ToolbarButton:
  base::string16 GetTooltipText(const gfx::Point& p) const override;
  const char* GetClassName() const override;
  void GetAccessibleNodeData(ui::AXNodeData* node_data) override;

 private:
  // Highlights the ink drop for the icon, used when the corresponding widget
  // is visible.
  void SetHighlighted(bool bubble_visible);

  void OnPreferenceChanged();

  bool on_ = false;
  PrefService* prefs_ = nullptr;
  PrefChangeRegistrar pref_change_registrar_;

  // Can be true even if |on_| is false, but it doesn't affect us.
  bool active_ = false;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_TOOLBAR_SPEEDREADER_BUTTON_H_
