/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/page_info/brave_page_info_tab_switcher.h"

#include "brave/components/vector_icons/vector_icons.h"
#include "components/grit/brave_components_strings.h"
#include "components/strings/grit/components_strings.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/base/models/image_model.h"
#include "ui/color/color_id.h"
#include "ui/views/background.h"
#include "ui/views/controls/button/label_button.h"
#include "ui/views/controls/separator.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/layout/flex_layout.h"
#include "ui/views/view_class_properties.h"

namespace {

constexpr ui::ColorId kTabButtonColor = ui::kColorTabForeground;
constexpr ui::ColorId kTabButtonHighlightColor =
    ui::kColorTabForegroundSelected;

}  // namespace

BravePageInfoTabSwitcher::BravePageInfoTabSwitcher(
    TabButtonPressedCallback on_tab_button_pressed)
    : on_tab_button_pressed_(std::move(on_tab_button_pressed)) {
  // Use vertical layout to stack buttons and separator.
  SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical));

  // Create a container for the tab buttons with horizontal flex layout.
  auto button_container = std::make_unique<views::View>();
  auto* layout =
      button_container->SetLayoutManager(std::make_unique<views::FlexLayout>());
  layout->SetOrientation(views::LayoutOrientation::kHorizontal)
      .SetMainAxisAlignment(views::LayoutAlignment::kCenter)
      .SetCrossAxisAlignment(views::LayoutAlignment::kStretch)
      .SetInteriorMargin(gfx::Insets::VH(0, 16));

  // Add the "Brave Shields" button.
  shields_button_ = button_container->AddChildView(CreateTabButton(
      Tab::kShields, BravePageInfoViewID::kTabSwitcherShieldsButton));

  // Add the "Site Settings" button.
  site_settings_button_ = button_container->AddChildView(CreateTabButton(
      Tab::kSiteSettings, BravePageInfoViewID::kTabSwitcherSiteSettingsButton));

  // Add the button container to the main container.
  AddChildView(std::move(button_container));

  // Create the tab indicator bar (positioned manually under the active tab).
  // This view is excluded from layout management so we can position it freely.
  auto indicator = std::make_unique<views::View>();
  indicator->SetBackground(
      views::CreateSolidBackground(kTabButtonHighlightColor));
  indicator->SetProperty(views::kViewIgnoredByLayoutKey, true);
  tab_indicator_ = AddChildView(std::move(indicator));

  // Add a separator below the buttons.
  AddChildView(std::make_unique<views::Separator>());

  // Set the initial button styles.
  UpdateTabButtons();
}

BravePageInfoTabSwitcher::~BravePageInfoTabSwitcher() = default;

void BravePageInfoTabSwitcher::SetCurrentTab(Tab tab) {
  current_tab_ = tab;
  UpdateTabButtons();
  UpdateTabIndicator();
}

void BravePageInfoTabSwitcher::SetShieldsEnabled(bool enabled) {
  shields_enabled_ = enabled;
  UpdateTabButton(Tab::kShields);
}

void BravePageInfoTabSwitcher::Layout(PassKey pass_key) {
  LayoutSuperclass<views::View>(this);
  UpdateTabIndicator();
}

std::unique_ptr<views::LabelButton> BravePageInfoTabSwitcher::CreateTabButton(
    Tab tab,
    BravePageInfoViewID view_id) {
  int text_id = GetTabButtonText(tab);
  auto& icon = GetTabButtonIcon(tab);

  auto button = std::make_unique<views::LabelButton>(
      base::BindRepeating(on_tab_button_pressed_, tab),
      l10n_util::GetStringUTF16(text_id));

  button->SetID(static_cast<int>(view_id));
  button->SetImageModel(
      views::Button::STATE_NORMAL,
      ui::ImageModel::FromVectorIcon(icon, kTabButtonColor, 16));
  button->SetImageLabelSpacing(8);
  button->SetLabelStyle(views::style::STYLE_HEADLINE_5);
  button->SetBorder(views::CreateEmptyBorder(gfx::Insets(16)));
  button->SetHorizontalAlignment(gfx::ALIGN_CENTER);
  button->SetProperty(
      views::kFlexBehaviorKey,
      views::FlexSpecification(views::LayoutOrientation::kHorizontal,
                               views::MinimumFlexSizeRule::kScaleToZero,
                               views::MaximumFlexSizeRule::kUnbounded)
          .WithWeight(1));

  return button;
}

void BravePageInfoTabSwitcher::UpdateTabButtons() {
  UpdateTabButton(Tab::kSiteSettings);
  UpdateTabButton(Tab::kShields);
}

void BravePageInfoTabSwitcher::UpdateTabButton(Tab tab) {
  auto* button = GetButtonForTab(tab);
  const ui::ColorId color =
      tab == current_tab_ ? kTabButtonHighlightColor : kTabButtonColor;

  // Update text colors.
  button->SetEnabledTextColors(color);
  button->SetTextColor(views::Button::STATE_HOVERED, kTabButtonHighlightColor);

  // Update icon and icon colors.
  auto& icon = GetTabButtonIcon(tab);
  button->SetImageModel(views::Button::STATE_NORMAL,
                        ui::ImageModel::FromVectorIcon(icon, color, 16));
  button->SetImageModel(
      views::Button::STATE_HOVERED,
      ui::ImageModel::FromVectorIcon(icon, kTabButtonHighlightColor, 16));
}

views::LabelButton* BravePageInfoTabSwitcher::GetButtonForTab(Tab tab) {
  switch (tab) {
    case Tab::kShields:
      return shields_button_.get();
    case Tab::kSiteSettings:
      return site_settings_button_.get();
  }
}

void BravePageInfoTabSwitcher::UpdateTabIndicator() {
  CHECK(tab_indicator_);

  constexpr int kIndicatorHeight = 2;

  views::LabelButton* active_button = GetButtonForTab(current_tab_);
  CHECK(active_button);

  // Convert button bounds to indicator's parent coordinate space.
  gfx::Rect button_bounds = active_button->bounds();
  button_bounds = views::View::ConvertRectToTarget(
      active_button->parent(), tab_indicator_->parent(), button_bounds);

  // Position the indicator under the active button with full button width.
  tab_indicator_->SetBounds(button_bounds.x(),
                            button_bounds.bottom() - kIndicatorHeight,
                            button_bounds.width(), kIndicatorHeight);
}

int BravePageInfoTabSwitcher::GetTabButtonText(Tab tab) const {
  switch (tab) {
    case Tab::kShields:
      return IDS_BRAVE_SHIELDS;
    case Tab::kSiteSettings:
      return IDS_PAGE_INFO_SITE_SETTINGS_LINK;
  }
}

const gfx::VectorIcon& BravePageInfoTabSwitcher::GetTabButtonIcon(Tab tab) {
  switch (tab) {
    case Tab::kShields:
      return shields_enabled_ ? kLeoShieldDoneIcon
                              : kLeoShieldDisableFilledIcon;
    case Tab::kSiteSettings:
      return kLeoTuneSmallIcon;
  }
}

BEGIN_METADATA(BravePageInfoTabSwitcher)
END_METADATA
