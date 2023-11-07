// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/views/tabs/brave_tab.h"

#include <algorithm>

#include "brave/browser/ui/tabs/brave_tab_layout_constants.h"
#include "brave/browser/ui/tabs/brave_tab_prefs.h"
#include "brave/browser/ui/tabs/features.h"
#include "brave/browser/ui/views/frame/brave_browser_view.h"
#include "brave/browser/ui/views/frame/vertical_tab_strip_region_view.h"
#include "brave/browser/ui/views/frame/vertical_tab_strip_widget_delegate_view.h"
#include "brave/browser/ui/views/tabs/vertical_tab_utils.h"
#include "brave/browser/ui/views/view_shadow.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/views/tabs/alert_indicator_button.h"
#include "chrome/browser/ui/views/tabs/tab_close_button.h"
#include "chrome/browser/ui/views/tabs/tab_slot_controller.h"
#include "ui/compositor/layer.h"
#include "ui/compositor/paint_recorder.h"
#include "ui/gfx/favicon_size.h"
#include "ui/gfx/skia_paint_util.h"
#include "ui/views/animation/ink_drop.h"
#include "ui/views/controls/label.h"
#include "ui/views/view_class_properties.h"

namespace {

constexpr ViewShadow::ShadowParameters kShadow{
    .offset_x = 0,
    .offset_y = 1,
    .blur_radius = 4,
    .shadow_color = SkColorSetA(SK_ColorBLACK, 0.07 * 255)};

}  // namespace

BraveTab::BraveTab(TabSlotController* controller)
    : Tab(controller),
      normal_font_(views::Label::GetDefaultFontList()),
      active_tab_font_(
          normal_font_.DeriveWithWeight(gfx::Font::Weight::MEDIUM)) {
  title_->SetFontList(IsActive() ? active_tab_font_ : normal_font_);
}

BraveTab::~BraveTab() = default;

std::u16string BraveTab::GetTooltipText(const gfx::Point& p) const {
  auto* browser = controller_->GetBrowser();
  if (browser &&
      brave_tabs::AreTooltipsEnabled(browser->profile()->GetPrefs())) {
    return Tab::GetTooltipText(data_.title,
                               GetAlertStateToShow(data_.alert_state));
  }
  return Tab::GetTooltipText(p);
}

int BraveTab::GetWidthOfLargestSelectableRegion() const {
  // Assume the entire region except the area that alert indicator/close buttons
  // occupied is available for click-to-select.
  // If neither are visible, the entire tab region is available.
  int selectable_width = width();
  if (alert_indicator_button_->GetVisible()) {
    selectable_width -= alert_indicator_button_->width();
  }

  if (close_button_->GetVisible()) {
    selectable_width -= close_button_->width();
  }

  return std::max(0, selectable_width);
}

void BraveTab::ActiveStateChanged() {
  Tab::ActiveStateChanged();

  title_->SetFontList(IsActive() ? active_tab_font_ : normal_font_);

  // This should be called whenever the active state changes
  // see comment on UpdateEnabledForMuteToggle();
  // https://github.com/brave/brave-browser/issues/23476/
  alert_indicator_button_->UpdateEnabledForMuteToggle();

  UpdateShadowForActiveTab();
}

absl::optional<SkColor> BraveTab::GetGroupColor() const {
  // Hide tab border with group color as it doesn't go well with vertical tabs.
  if (tabs::utils::ShouldShowVerticalTabs(controller()->GetBrowser())) {
    return {};
  }

  if (!tabs::features::HorizontalTabsUpdateEnabled()) {
    return Tab::GetGroupColor();
  }

  // Unlike upstream, tabs that are within a group are not given a border color.
  return {};
}

void BraveTab::UpdateIconVisibility() {
  Tab::UpdateIconVisibility();
  if (IsAtMinWidthForVerticalTabStrip()) {
    if (data().pinned) {
      center_icon_ = true;
      showing_icon_ = !showing_alert_indicator_;
      showing_close_button_ = false;
    } else {
      center_icon_ = true;

      const bool is_active = IsActive();
      const bool can_enter_floating_mode =
          tabs::utils::IsFloatingVerticalTabsEnabled(
              controller()->GetBrowser());
      // When floating mode enabled, we don't show close button as the tab strip
      // will be expanded as soon as mouse hovers onto the tab.
      showing_close_button_ =
          !showing_alert_indicator_ && !can_enter_floating_mode && is_active;
      showing_icon_ = !showing_alert_indicator_ && !showing_close_button_;
    }
  }
}

void BraveTab::Layout() {
  Tab::Layout();
  if (IsAtMinWidthForVerticalTabStrip()) {
    if (showing_close_button_) {
      close_button_->SetX(GetLocalBounds().CenterPoint().x() -
                          (close_button_->width() / 2));

      // In order to reset ink drop bounds based on new padding.
      auto* ink_drop = views::InkDrop::Get(close_button_)->GetInkDrop();
      DCHECK(ink_drop);
      ink_drop->HostSizeChanged(close_button_->size());
    }
  }
}

gfx::Insets BraveTab::GetInsets() const {
  // Supplement extra left side padding.
  // Upstream gives extra padding to balance with right side padding space but
  // it's gone when tab doesn't have sufficient available width. In our case,
  // As we have more narrow left & right padding than upstream, icon seems stick
  // to left side when extra padding is not used.
  // We only need to do that when |extra_padding_before_content_| is false.
  int extra_left_padding = 0;

  // Add extra padding if upstream tab doesn't have it.
  if (!extra_padding_before_content_) {
    extra_left_padding = kExtraLeftPadding;
  }

  auto insets = Tab::GetInsets();
  insets.set_left(insets.left() + extra_left_padding);
  return insets;
}

void BraveTab::MaybeAdjustLeftForPinnedTab(gfx::Rect* bounds,
                                           int visual_width) const {
  if (!tabs::utils::ShouldShowVerticalTabs(controller()->GetBrowser())) {
    Tab::MaybeAdjustLeftForPinnedTab(bounds, visual_width);
    return;
  }

  // We keep favicon on fixed position so that it won't move left and right
  // during animation.
  bounds->set_x((tabs::kVerticalTabMinWidth - gfx::kFaviconSize) / 2);
}

bool BraveTab::ShouldRenderAsNormalTab() const {
  if (IsAtMinWidthForVerticalTabStrip()) {
    // Returns false to hide title
    return false;
  }

  return Tab::ShouldRenderAsNormalTab();
}

bool BraveTab::IsAtMinWidthForVerticalTabStrip() const {
  return tabs::utils::ShouldShowVerticalTabs(controller()->GetBrowser()) &&
         width() <= tabs::kVerticalTabMinWidth;
}

void BraveTab::UpdateShadowForActiveTab() {
  bool can_render_shadows =
      tabs::features::HorizontalTabsUpdateEnabled() ||
      tabs::utils::ShouldShowVerticalTabs(controller()->GetBrowser());

  if (IsActive() && can_render_shadows) {
    if (!view_shadow_) {
      view_shadow_ = std::make_unique<ViewShadow>(
          this, tabs::GetTabCornerRadius(*this), kShadow);
      layer()->SetFillsBoundsOpaquely(false);
    }

    gfx::Insets shadow_insets;
    if (!tabs::utils::ShouldShowVerticalTabs(controller()->GetBrowser())) {
      // For horizontal tabs, inset the shadow layer to match the visual rounded
      // rectangle of the tab, which is inset from the tab view.
      shadow_insets = gfx::Insets::VH(0, brave_tabs::kHorizontalTabInset);
    }
    view_shadow_->SetInsets(shadow_insets);
  } else if (view_shadow_) {
    view_shadow_.reset();
    DestroyLayer();
  }
}
