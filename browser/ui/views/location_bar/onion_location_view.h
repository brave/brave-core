/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_LOCATION_BAR_ONION_LOCATION_VIEW_H_
#define BRAVE_BROWSER_UI_VIEWS_LOCATION_BAR_ONION_LOCATION_VIEW_H_

#include "ui/gfx/geometry/size.h"
#include "ui/views/controls/button/label_button.h"
#include "ui/views/controls/button/label_button_border.h"

class Profile;

namespace content {
class WebContents;
}  // namespace content

class OnionLocationView : public views::View {
 public:
  explicit OnionLocationView(Profile* profile);
  ~OnionLocationView() override;

  void Update(content::WebContents* web_contents);

  views::LabelButton* GetButton() { return button_; }

 private:
  views::LabelButton* button_ = nullptr;

  OnionLocationView(const OnionLocationView&) = delete;
  OnionLocationView& operator=(const OnionLocationView&) = delete;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_LOCATION_BAR_ONION_LOCATION_VIEW_H_
