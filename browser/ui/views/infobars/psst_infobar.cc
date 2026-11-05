/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/infobars/psst_infobar.h"

#include <algorithm>
#include <utility>

#include "base/functional/bind.h"
#include "brave/browser/psst/psst_infobar_delegate.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/browser/ui/color/chrome_color_id.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/base/window_open_disposition_utils.h"
#include "ui/events/event.h"
#include "ui/gfx/font_list.h"
#include "ui/gfx/geometry/point.h"
#include "ui/gfx/range/range.h"
#include "ui/views/controls/label.h"
#include "ui/views/controls/styled_label.h"
#include "ui/views/style/typography.h"

namespace {

// Vertical gap between the title and subtitle rows.
constexpr int kTitleSubtitleSpacing = 2;

}  // namespace

std::unique_ptr<infobars::InfoBar> CreatePsstInfoBar(
    std::unique_ptr<psst::PsstInfoBarDelegate> delegate) {
  return std::make_unique<PsstInfoBar>(std::move(delegate));
}

PsstInfoBar::PsstInfoBar(std::unique_ptr<psst::PsstInfoBarDelegate> delegate)
    : BraveConfirmInfoBar(std::move(delegate)) {
  // The base label acts as the bold title; emphasize it to match.
  label_->SetFontList(
      label_->font_list().DeriveWithWeight(gfx::Font::Weight::SEMIBOLD));

  // The base link is unused here: "Learn more" is inline in `subtitle_`
  // rather than a separate trailing control.
  link_->SetVisible(false);

  const std::u16string learn_more_text =
      l10n_util::GetStringUTF16(IDS_BRAVE_PSST_INFOBAR_LEARN_MORE);
  const std::u16string subtitle_text =
      l10n_util::GetStringUTF16(IDS_BRAVE_PSST_INFOBAR_SUBTITLE);
  const std::u16string full_text = subtitle_text + u" " + learn_more_text;

  auto subtitle = std::make_unique<views::StyledLabel>();
  subtitle->SetText(full_text);
  subtitle->SetTextContext(views::style::CONTEXT_DIALOG_BODY_TEXT);
  subtitle->SetDisplayedOnBackgroundColor(kColorInfoBarBackground);
  subtitle->AddStyleRange(
      gfx::Range(full_text.size() - learn_more_text.size(), full_text.size()),
      views::StyledLabel::RangeStyleInfo::CreateForLink(base::BindRepeating(
          &PsstInfoBar::LearnMoreLinkClicked, base::Unretained(this))));

  subtitle_ = AddChildView(std::move(subtitle));
  subtitle_->SizeToFit(0);

  SetTargetHeight(target_height() + subtitle_->height() +
                  kTitleSubtitleSpacing);
}

PsstInfoBar::~PsstInfoBar() = default;

void PsstInfoBar::Layout(PassKey) {
  LayoutSuperclass<BraveConfirmInfoBar>(this);

  subtitle_->SizeToFit(label_->width());

  // Center the title+subtitle block the same way OffsetY() centers a single
  // view, so it stays anchored consistently with the icon/buttons during the
  // show/hide slide animation.
  const int block_height =
      label_->height() + kTitleSubtitleSpacing + subtitle_->height();
  const int top = std::max((target_height() - block_height) / 2, 0) -
                  (target_height() - height());

  label_->SetPosition(gfx::Point(label_->x(), top));
  subtitle_->SetPosition(
      gfx::Point(label_->x(), top + label_->height() + kTitleSubtitleSpacing));
}

void PsstInfoBar::LearnMoreLinkClicked(const ui::Event& event) {
  if (!owner()) {
    return;  // We're closing; don't call anything, it might access the owner.
  }
  if (delegate()->LinkClicked(ui::DispositionFromEventFlags(event.flags()))) {
    RemoveSelf();
  }
}

BEGIN_METADATA(PsstInfoBar)
END_METADATA
