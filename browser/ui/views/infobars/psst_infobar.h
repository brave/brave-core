/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_INFOBARS_PSST_INFOBAR_H_
#define BRAVE_BROWSER_UI_VIEWS_INFOBARS_PSST_INFOBAR_H_

#include <memory>

#include "base/memory/raw_ptr.h"
#include "brave/browser/ui/views/infobars/brave_confirm_infobar.h"
#include "ui/base/metadata/metadata_header_macros.h"

namespace psst {
class PsstInfoBarDelegate;
}  // namespace psst

namespace ui {
class Event;
}  // namespace ui

namespace views {
class StyledLabel;
}  // namespace views

// Customized BraveConfirmInfoBar with a two-line message:
//   [icon]  Title (bold)                        [ok_button] [x]
//           Subtitle text ...  _Learn more_
class PsstInfoBar : public BraveConfirmInfoBar {
  METADATA_HEADER(PsstInfoBar, BraveConfirmInfoBar)

 public:
  explicit PsstInfoBar(std::unique_ptr<psst::PsstInfoBarDelegate> delegate);
  PsstInfoBar(const PsstInfoBar&) = delete;
  PsstInfoBar& operator=(const PsstInfoBar&) = delete;
  ~PsstInfoBar() override;

  // BraveConfirmInfoBar:
  void Layout(PassKey) override;

  views::StyledLabel* subtitle_for_testing() const { return subtitle_.get(); }

 private:
  // Base's `label_` is repurposed as the bold title and `link_` is hidden:
  // the "Learn more" link lives inline in `subtitle_` instead.
  void LearnMoreLinkClicked(const ui::Event& event);

  raw_ptr<views::StyledLabel> subtitle_ = nullptr;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_INFOBARS_PSST_INFOBAR_H_
