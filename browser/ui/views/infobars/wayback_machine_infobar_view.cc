// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/views/infobars/wayback_machine_infobar_view.h"

#include <algorithm>
#include <string>
#include <utility>

#include "base/functional/bind.h"
#include "base/i18n/time_formatting.h"
#include "base/strings/strcat.h"
#include "base/time/time.h"
#include "brave/browser/ui/views/page_action/wayback_machine_infobar_delegate.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/browser/ui/views/chrome_layout_provider.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/l10n/time_format.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/base/window_open_disposition_utils.h"
#include "ui/events/event.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/gfx/range/range.h"
#include "ui/strings/grit/ui_strings.h"
#include "ui/views/controls/button/md_text_button.h"
#include "ui/views/controls/label.h"
#include "ui/views/controls/styled_label.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/layout/box_layout_view.h"
#include "ui/views/layout/flex_layout.h"
#include "ui/views/style/typography.h"
#include "ui/views/view_class_properties.h"

// wayback_machine_infobar_delegate.cc has decls.
std::unique_ptr<infobars::InfoBar> CreateWaybackMachineInfoBar(
    std::unique_ptr<WaybackMachineInfoBarDelegate> delegate) {
  return std::make_unique<WaybackMachineInfoBarView>(std::move(delegate));
}

namespace {

std::u16string GetMessageText(base::Time snapshot_time) {
  if (snapshot_time.is_null()) {
    return l10n_util::GetStringUTF16(
        IDS_BRAVE_WAYBACK_MACHINE_INFOBAR_MESSAGE_NO_DATE);
  }
  const base::TimeDelta age =
      std::max(base::Time::Now() - snapshot_time, base::TimeDelta());
  return l10n_util::GetStringFUTF16(
      IDS_BRAVE_WAYBACK_MACHINE_INFOBAR_MESSAGE,
      base::TimeFormatShortDate(snapshot_time),
      ui::TimeFormat::SimpleWithMonthAndYear(ui::TimeFormat::FORMAT_ELAPSED,
                                             ui::TimeFormat::LENGTH_LONG, age,
                                             /*use_month_and_year=*/true));
}

std::unique_ptr<views::MdTextButton> CreateButton(
    views::Button::PressedCallback callback,
    const std::u16string& text,
    ui::ButtonStyle style) {
  auto* provider = ChromeLayoutProvider::Get();
  auto button =
      std::make_unique<views::MdTextButton>(std::move(callback), text);
  button->SetCustomPadding(gfx::Insets::VH(
      provider->GetDistanceMetric(DISTANCE_INFOBAR_BUTTON_VERTICAL_PADDING),
      provider->GetDistanceMetric(DISTANCE_INFOBAR_BUTTON_HORIZONTAL_PADDING)));
  button->SetStyle(style);
  button->SetProperty(
      views::kFlexBehaviorKey,
      views::FlexSpecification(views::MinimumFlexSizeRule::kPreferred,
                               views::MaximumFlexSizeRule::kPreferred));
  return button;
}

}  // namespace

WaybackMachineInfoBarView::WaybackMachineInfoBarView(
    std::unique_ptr<WaybackMachineInfoBarDelegate> delegate)
    : InfoBarView(std::move(delegate)) {
  auto* provider = ChromeLayoutProvider::Get();

  content_container()
      ->SetLayoutManager(std::make_unique<views::FlexLayout>())
      ->SetOrientation(views::LayoutOrientation::kHorizontal)
      .SetCrossAxisAlignment(views::LayoutAlignment::kCenter);

  auto text_column = std::make_unique<views::BoxLayoutView>();
  text_column->SetOrientation(views::BoxLayout::Orientation::kVertical);
  text_column->SetCrossAxisAlignment(
      views::BoxLayout::CrossAxisAlignment::kStart);
  text_column->SetProperty(
      views::kFlexBehaviorKey,
      views::FlexSpecification(views::MinimumFlexSizeRule::kScaleToZero,
                               views::MaximumFlexSizeRule::kPreferred)
          .WithWeight(1));
  text_column->SetProperty(
      views::kMarginsKey,
      gfx::Insets::TLBR(0, 0, 0,
                        provider->GetDistanceMetric(
                            views::DISTANCE_UNRELATED_CONTROL_HORIZONTAL)));

  auto title = CreateLabel(
      l10n_util::GetStringUTF16(IDS_BRAVE_WAYBACK_MACHINE_INFOBAR_TITLE));
  title->SetTextStyle(views::style::STYLE_EMPHASIZED);
  title->SetProperty(views::kMarginsKey, gfx::Insets());
  text_column->AddChildView(std::move(title));

  const std::u16string message = GetMessageText(GetDelegate()->snapshot_time());
  const std::u16string link_text = GetDelegate()->GetLinkText();
  auto body = CreateStyledLabel(base::StrCat({message, u" ", link_text}));
  const size_t link_start = message.size() + 1;
  body->AddStyleRange(
      gfx::Range(link_start, link_start + link_text.size()),
      views::StyledLabel::RangeStyleInfo::CreateForLink(
          base::BindRepeating(&WaybackMachineInfoBarView::OnLearnMoreClicked,
                              base::Unretained(this))));
  body->SetProperty(views::kMarginsKey, gfx::Insets());
  text_column->AddChildView(std::move(body));

  auto* text_column_ptr = AddContentChildView(std::move(text_column));

  cancel_button_ = AddContentChildView(CreateButton(
      base::BindRepeating(&WaybackMachineInfoBarView::OnCancelButtonPressed,
                          base::Unretained(this)),
      l10n_util::GetStringUTF16(IDS_APP_CANCEL), ui::ButtonStyle::kText));

  redirect_button_ = AddContentChildView(CreateButton(
      base::BindRepeating(&WaybackMachineInfoBarView::OnRedirectButtonPressed,
                          base::Unretained(this)),
      std::u16string(), ui::ButtonStyle::kProminent));
  redirect_button_->SetProperty(
      views::kMarginsKey,
      gfx::Insets::TLBR(0,
                        provider->GetDistanceMetric(
                            views::DISTANCE_RELATED_BUTTON_HORIZONTAL),
                        0, 0));
  UpdateRedirectButtonText();

  GetDelegate()->SetCountdownChangedCallback(
      base::BindRepeating(&WaybackMachineInfoBarView::UpdateRedirectButtonText,
                          base::Unretained(this)));

  const int vertical_padding =
      provider->GetDistanceMetric(DISTANCE_TOAST_LABEL_VERTICAL);
  SetTargetHeight(std::max(
      provider->GetDistanceMetric(DISTANCE_INFOBAR_HEIGHT),
      text_column_ptr->GetPreferredSize().height() + 2 * vertical_padding));
}

WaybackMachineInfoBarView::~WaybackMachineInfoBarView() = default;

WaybackMachineInfoBarDelegate* WaybackMachineInfoBarView::GetDelegate() {
  return static_cast<WaybackMachineInfoBarDelegate*>(delegate());
}

void WaybackMachineInfoBarView::UpdateRedirectButtonText() {
  redirect_button_->SetText(l10n_util::GetPluralStringFUTF16(
      IDS_BRAVE_WAYBACK_MACHINE_INFOBAR_REDIRECT_BUTTON,
      GetDelegate()->seconds_remaining()));
}

void WaybackMachineInfoBarView::OnCancelButtonPressed() {
  if (!owner()) {
    return;
  }
  GetDelegate()->Dismiss();
}

void WaybackMachineInfoBarView::OnRedirectButtonPressed() {
  if (!owner()) {
    return;
  }
  GetDelegate()->LoadNow();
}

void WaybackMachineInfoBarView::OnLearnMoreClicked(const ui::Event& event) {
  if (!owner()) {
    return;
  }
  if (GetDelegate()->LinkClicked(
          ui::DispositionFromEventFlags(event.flags()))) {
    RemoveSelf();
  }
}

BEGIN_METADATA(WaybackMachineInfoBarView)
END_METADATA
