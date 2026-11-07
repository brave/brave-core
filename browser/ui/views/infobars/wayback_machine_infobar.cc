// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/views/infobars/wayback_machine_infobar.h"

#include <cstddef>
#include <memory>
#include <string>
#include <utility>

#include "base/functional/bind.h"
#include "brave/browser/infobars/wayback_machine_infobar_creator.h"
#include "brave/browser/infobars/wayback_machine_infobar_delegate.h"
#include "brave/components/vector_icons/vector_icons.h"
#include "brave/ui/color/nala/nala_color_id.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/base/models/image_model.h"
#include "ui/base/ui_base_types.h"
#include "ui/gfx/font.h"
#include "ui/gfx/font_list.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/gfx/geometry/size.h"
#include "ui/gfx/range/range.h"
#include "ui/gfx/text_constants.h"
#include "ui/views/background.h"
#include "ui/views/controls/button/md_text_button.h"
#include "ui/views/controls/image_view.h"
#include "ui/views/controls/label.h"
#include "ui/views/controls/styled_label.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/layout/box_layout_view.h"
#include "ui/views/layout/flex_layout.h"
#include "ui/views/layout/flex_layout_types.h"
#include "ui/views/view_class_properties.h"

namespace {

constexpr int kInfobarHeight = 68;
constexpr int kIconBadgeSize = 40;
constexpr int kIconBadgeCornerRadius = 8;
constexpr int kIconSize = 24;
constexpr int kIconToTextSpacing = 16;
constexpr int kTextToButtonsSpacing = 24;
constexpr int kButtonsSpacing = 8;
constexpr int kDefaultFontSize = 14;
constexpr int kDefaultLineHeight = 22;
constexpr int kButtonSmallFontSize = 12;
constexpr int kRedirectButtonExtraWidth = 4;

gfx::FontList GetFont(int font_size, gfx::Font::Weight weight) {
  gfx::FontList font_list;
  return font_list.DeriveWithSizeDelta(font_size - font_list.GetFontSize())
      .DeriveWithWeight(weight);
}

class CustomFontMdTextButton : public views::MdTextButton {
  METADATA_HEADER(CustomFontMdTextButton, views::MdTextButton)

 public:
  using views::MdTextButton::MdTextButton;
  CustomFontMdTextButton(const CustomFontMdTextButton&) = delete;
  CustomFontMdTextButton& operator=(const CustomFontMdTextButton&) = delete;
  ~CustomFontMdTextButton() override = default;

  void SetFontList(const gfx::FontList& font_list) {
    label()->SetFontList(font_list);
  }
};

BEGIN_METADATA(CustomFontMdTextButton)
END_METADATA

}  // namespace

std::unique_ptr<infobars::InfoBar> CreateWaybackMachineInfoBar(
    std::unique_ptr<WaybackMachineInfoBarDelegate> delegate) {
  return std::make_unique<WaybackMachineInfoBar>(std::move(delegate));
}

WaybackMachineInfoBar::WaybackMachineInfoBar(
    std::unique_ptr<WaybackMachineInfoBarDelegate> delegate)
    : InfoBarView(std::move(delegate)) {
  auto* delegate_ptr = GetDelegate();
  // Unretained() is safe because this infobar owns the delegate.
  delegate_ptr->SetCountdownChangedCallback(base::BindRepeating(
      &WaybackMachineInfoBar::OnCountdownChanged, base::Unretained(this)));

  auto* layout = content_container()->SetLayoutManager(
      std::make_unique<views::FlexLayout>());
  layout->SetOrientation(views::LayoutOrientation::kHorizontal)
      .SetCrossAxisAlignment(views::LayoutAlignment::kCenter);

  const views::FlexSpecification kRigidFlex(
      views::MinimumFlexSizeRule::kPreferred,
      views::MaximumFlexSizeRule::kPreferred);

  auto icon_badge = std::make_unique<views::BoxLayoutView>();
  icon_badge->SetMainAxisAlignment(
      views::BoxLayout::MainAxisAlignment::kCenter);
  icon_badge->SetCrossAxisAlignment(
      views::BoxLayout::CrossAxisAlignment::kCenter);
  icon_badge->SetPreferredSize(gfx::Size(kIconBadgeSize, kIconBadgeSize));
  icon_badge->SetBackground(views::CreateRoundedRectBackground(
      nala::kColorDesktopbrowserToolbarButtonActive, kIconBadgeCornerRadius));
  icon_badge->SetProperty(views::kFlexBehaviorKey, kRigidFlex);
  icon_badge->SetProperty(views::kMarginsKey,
                          gfx::Insets::TLBR(0, 0, 0, kIconToTextSpacing));

  auto* icon = icon_badge->AddChildView(std::make_unique<views::ImageView>());
  icon->SetImage(ui::ImageModel::FromVectorIcon(
      kLeoInternetArchiveIcon, nala::kColorIconDefault, kIconSize));
  AddContentChildView(std::move(icon_badge));

  auto text_column = std::make_unique<views::BoxLayoutView>();
  text_column->SetOrientation(views::BoxLayout::Orientation::kVertical);
  text_column->SetCrossAxisAlignment(
      views::BoxLayout::CrossAxisAlignment::kStart);
  text_column->SetProperty(
      views::kFlexBehaviorKey,
      views::FlexSpecification(views::MinimumFlexSizeRule::kScaleToZero,
                               views::MaximumFlexSizeRule::kPreferred)
          .WithWeight(1));
  text_column->SetProperty(views::kMarginsKey,
                           gfx::Insets::TLBR(0, 0, 0, kTextToButtonsSpacing));

  auto* title_label = text_column->AddChildView(std::make_unique<views::Label>(
      delegate_ptr->GetTitleText(),
      views::Label::CustomFont{
          GetFont(kDefaultFontSize, gfx::Font::Weight::SEMIBOLD)}));
  title_label->SetLineHeight(kDefaultLineHeight);
  title_label->SetEnabledColor(nala::kColorTextPrimary);
  title_label->SetAutoColorReadabilityEnabled(false);
  title_label->SetHorizontalAlignment(gfx::ALIGN_LEFT);

  size_t learn_more_offset = 0;
  const std::u16string description_text =
      delegate_ptr->GetDescriptionText(&learn_more_offset);
  const std::u16string learn_more_text = delegate_ptr->GetLearnMoreText();

  auto* description_label =
      text_column->AddChildView(std::make_unique<views::StyledLabel>());
  description_label->SetText(description_text);
  description_label->SetLineHeight(kDefaultLineHeight);
  description_label->SetDefaultEnabledColorId(nala::kColorTextSecondary);
  description_label->SetAutoColorReadabilityEnabled(false);
  description_label->SetHorizontalAlignment(gfx::ALIGN_LEFT);

  const gfx::FontList body_font =
      GetFont(kDefaultFontSize, gfx::Font::Weight::NORMAL);
  if (learn_more_offset > 0) {
    views::StyledLabel::RangeStyleInfo prefix_style;
    prefix_style.custom_font = body_font;
    prefix_style.override_color_id = nala::kColorTextSecondary;
    description_label->AddStyleRange(gfx::Range(0, learn_more_offset),
                                     prefix_style);
  }

  views::StyledLabel::RangeStyleInfo link_style =
      views::StyledLabel::RangeStyleInfo::CreateForLink(base::BindRepeating(
          &WaybackMachineInfoBar::OnLearnMoreClicked, base::Unretained(this)));
  link_style.override_color_id = nala::kColorTextSecondary;
  description_label->AddStyleRange(
      gfx::Range(learn_more_offset,
                 learn_more_offset + learn_more_text.length()),
      link_style);

  AddContentChildView(std::move(text_column));

  auto cancel_button = std::make_unique<CustomFontMdTextButton>(
      base::BindRepeating(&WaybackMachineInfoBar::OnCancelClicked,
                          base::Unretained(this)),
      delegate_ptr->GetCancelButtonText());
  cancel_button->SetStyle(ui::ButtonStyle::kText);
  cancel_button->SetFontList(
      GetFont(kButtonSmallFontSize, gfx::Font::Weight::SEMIBOLD));
  cancel_button->SetEnabledTextColors(nala::kColorTextSecondary);
  cancel_button->SetProperty(views::kFlexBehaviorKey, kRigidFlex);
  cancel_button->SetProperty(views::kMarginsKey,
                             gfx::Insets::TLBR(0, 0, 0, kButtonsSpacing));
  AddContentChildView(std::move(cancel_button));

  auto redirect_button = std::make_unique<views::MdTextButton>(
      base::BindRepeating(&WaybackMachineInfoBar::OnRedirectClicked,
                          base::Unretained(this)),
      delegate_ptr->GetRedirectButtonText());
  redirect_button->SetStyle(ui::ButtonStyle::kProminent);
  redirect_button->SetMinSize(gfx::Size(
      redirect_button->GetPreferredSize().width() + kRedirectButtonExtraWidth,
      0));
  redirect_button->SetProperty(views::kFlexBehaviorKey, kRigidFlex);
  redirect_button_ = AddContentChildView(std::move(redirect_button));

  SetTargetHeight(kInfobarHeight);
}

WaybackMachineInfoBar::~WaybackMachineInfoBar() = default;

WaybackMachineInfoBarDelegate* WaybackMachineInfoBar::GetDelegate() {
  return static_cast<WaybackMachineInfoBarDelegate*>(delegate());
}

void WaybackMachineInfoBar::OnCountdownChanged() {
  redirect_button_->SetText(GetDelegate()->GetRedirectButtonText());
  redirect_button_->SetMinSize(
      gfx::Size(redirect_button_->GetPreferredSize().width(), 0));
  InvalidateLayout();
}

void WaybackMachineInfoBar::OnRedirectClicked() {
  if (!owner()) {
    return;
  }
  GetDelegate()->Accept();
  RemoveSelf();
}

void WaybackMachineInfoBar::OnCancelClicked() {
  if (!owner()) {
    return;
  }
  GetDelegate()->Cancel();
  RemoveSelf();
}

void WaybackMachineInfoBar::OnLearnMoreClicked() {
  if (!owner()) {
    return;
  }
  GetDelegate()->OpenLearnMore();
  RemoveSelf();
}

BEGIN_METADATA(WaybackMachineInfoBar)
END_METADATA
