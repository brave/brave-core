/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_FRAME_FOCUS_MODE_TITLE_BAR_VIEW_H_
#define BRAVE_BROWSER_UI_VIEWS_FRAME_FOCUS_MODE_TITLE_BAR_VIEW_H_

#include "base/callback_list.h"
#include "base/memory/raw_ptr.h"
#include "components/tabs/public/tab_interface.h"
#include "ui/base/metadata/metadata_header_macros.h"
#include "ui/gfx/text_constants.h"
#include "ui/views/view.h"

namespace views {
class ImageView;
class Label;
}  // namespace views

// Small title bar shown at the top of the browser window when Focus Mode is
// enabled. Displays the active tab's favicon followed by the formatted host
// (omits https://, trivial subdomains, and trims after the host).
class FocusModeTitleBarView : public views::View {
  METADATA_HEADER(FocusModeTitleBarView, views::View)

 public:
  FocusModeTitleBarView();
  FocusModeTitleBarView(const FocusModeTitleBarView&) = delete;
  FocusModeTitleBarView& operator=(const FocusModeTitleBarView&) = delete;
  ~FocusModeTitleBarView() override;

  // Points this view at the given tab. `tab` may be null, in which case the
  // display is cleared. The view automatically clears itself if `tab` is
  // destroyed.
  void SetTab(tabs::TabInterface* tab);

  bool IsFaviconVisibleForTesting() const;
  const views::Label* domain_label_for_testing() const { return domain_label_; }

 private:
  void Update();
  void OnTabWillDetach(tabs::TabInterface* tab,
                       tabs::TabInterface::DetachReason reason);

  base::CallbackListSubscription tab_will_detach_subscription_;
  base::CallbackListSubscription tab_ui_updated_subscription_;
  raw_ptr<views::ImageView> favicon_image_ = nullptr;
  raw_ptr<views::Label> domain_label_ = nullptr;
  raw_ptr<tabs::TabInterface> tab_ = nullptr;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_FRAME_FOCUS_MODE_TITLE_BAR_VIEW_H_
