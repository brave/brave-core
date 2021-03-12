/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_LOCATION_BAR_IPFS_LOCATION_VIEW_H_
#define BRAVE_BROWSER_UI_VIEWS_LOCATION_BAR_IPFS_LOCATION_VIEW_H_

#include "ui/gfx/geometry/size.h"
#include "ui/views/controls/button/label_button.h"
#include "ui/views/controls/button/label_button_border.h"

class Profile;

namespace content {
class WebContents;
}  // namespace content

class IPFSLocationView : public views::View {
 public:
  explicit IPFSLocationView(Profile* profile);
  ~IPFSLocationView() override;

  void Update(content::WebContents* web_contents);

  views::LabelButton* GetButton() { return button_; }

 private:
  views::LabelButton* button_ = nullptr;

  IPFSLocationView(const IPFSLocationView&) = delete;
  IPFSLocationView& operator=(const IPFSLocationView&) = delete;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_LOCATION_BAR_IPFS_LOCATION_VIEW_H_
