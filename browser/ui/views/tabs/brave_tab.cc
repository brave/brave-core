// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/views/tabs/brave_tab.h"

#include <algorithm>
#include <memory>
#include <optional>
#include <string>
#include <utility>

#include "base/check.h"
#include "base/check_is_test.h"
#include "base/feature_list.h"
#include "base/functional/callback_forward.h"
#include "base/notimplemented.h"
#include "chrome/browser/ui/layout_constants.h"
#include "brave/browser/ui/tabs/brave_tab_prefs.h"
#include "brave/browser/ui/views/frame/brave_browser_view.h"
#include "brave/browser/ui/views/frame/vertical_tabs/vertical_tab_strip_region_view.h"
#include "brave/browser/ui/views/frame/vertical_tabs/vertical_tab_strip_widget_delegate_view.h"
#include "brave/browser/ui/views/tabs/vertical_tab_utils.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/features.h"
#include "chrome/browser/ui/views/tabs/alert_indicator_button.h"
#include "chrome/browser/ui/views/tabs/tab_close_button.h"
#include "chrome/browser/ui/views/tabs/tab_slot_controller.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/gfx/favicon_size.h"
#include "ui/views/animation/ink_drop.h"
#include "ui/views/controls/label.h"
#include "ui/views/controls/textfield/textfield.h"
#include "ui/views/view_class_properties.h"

namespace {

// MouseWatcherHost implementation that tells whether mouse clicks outside
// of tracked area.
class ClickWatcherHost : public views::MouseWatcherHost {
 public:
  explicit ClickWatcherHost(views::Textfield& textfield)
      : textfield_(textfield) {}
  ~ClickWatcherHost() override = default;

  bool Contains(const gfx::Point& screen_point, EventType type) override {
    if (type != EventType::kPress) {
      // We only tracks mouse press events
      return true;
    }

    auto bounds = textfield_->GetLocalBounds();
    views::View::ConvertRectToScreen(base::to_address(textfield_), &bounds);
    return bounds.Contains(screen_point);
  }

 private:
  raw_ref<views::Textfield> textfield_;
};

}  // namespace

BraveTab::RenameTextfield::RenameTextfield(
    base::RepeatingClosure on_click_outside_callback)
    : on_click_outside_callback_(std::move(on_click_outside_callback)),
      mouse_watcher_(std::make_unique<ClickWatcherHost>(*this), this) {
  SetBorder(nullptr);
  SetBackgroundEnabled(false);
}

BraveTab::RenameTextfield::~RenameTextfield() = default;

// views::Textfield:
void BraveTab::RenameTextfield::VisibilityChanged(views::View* starting_from,
                                                  bool is_visible) {
  if (starting_from != this) {
    return;
  }

  // We start or stop mouse watcher based on visibility of the textfield.
  if (is_visible) {
    auto* widget = GetWidget();
    if (!widget) {
      CHECK_IS_TEST();
      return;
    }

    mouse_watcher_.Start(widget->GetNativeWindow());
  } else {
    mouse_watcher_.Stop();
  }
}

// views::MouseWatcherListener:
void BraveTab::RenameTextfield::MouseMovedOutOfHost() {
  CHECK(on_click_outside_callback_);
  on_click_outside_callback_.Run();
}

BEGIN_METADATA(BraveTab, RenameTextfield)
END_METADATA

BraveTab::BraveTab(TabSlotController* controller) : Tab(controller) {
  if (!base::FeatureList::IsEnabled(tabs::kBraveRenamingTabs)) {
    return;
  }
  rename_textfield_ =
      AddChildView(std::make_unique<RenameTextfield>(base::BindRepeating(
          // This is safe to pass base::Unretained(this), as BraveTab is the
          // owner of RenameTextfield and outlives it.
          &BraveTab::CommitRename, base::Unretained(this))));
  rename_textfield_->SetVisible(false);
  rename_textfield_->set_controller(this);
  rename_textfield_->SetBorder(nullptr);
  rename_textfield_->SetBackgroundEnabled(false);
}

BraveTab::~BraveTab() = default;

void BraveTab::EnterRenameMode() {
  if (!rename_textfield_) {
    return;
  }

  if (in_renaming_mode()) {
    return;  // Already in rename mode.
  }

  // Fill the textfield with the current title of the tab and select all text.
  if (rename_textfield_->GetText().empty()) {
    rename_textfield_->SetText(title_->GetText());
  }
  rename_textfield_->SelectAll(/*reversed=*/false);
  UpdateRenameTextfieldBounds();
  rename_textfield_->SetVisible(true);
  title_->SetVisible(false);
  rename_textfield_->RequestFocus();
}

std::u16string BraveTab::GetRenderedTooltipText(const gfx::Point& p) const {
  auto* browser = controller_->GetBrowser();
  if (browser &&
      brave_tabs::AreTooltipsEnabled(browser->profile()->GetPrefs())) {
    return Tab::GetTooltipText(data_.title,
                               GetAlertStateToShow(data_.alert_state));
  }
  return TabSlotView::GetTooltipText();
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

  // This should be called whenever the active state changes
  // see comment on UpdateEnabledForMuteToggle();
  // https://github.com/brave/brave-browser/issues/23476/
  alert_indicator_button_->UpdateEnabledForMuteToggle();
}

std::optional<SkColor> BraveTab::GetGroupColor() const {
  // Hide tab border with group color as it doesn't go well with vertical tabs.
  if (tabs::utils::ShouldShowVerticalTabs(controller()->GetBrowser())) {
    return {};
  }

  if (!tabs::HorizontalTabsUpdateEnabled()) {
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

void BraveTab::Layout(PassKey) {
  LayoutSuperclass<Tab>(this);
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

  if (in_renaming_mode()) {
    UpdateRenameTextfieldBounds();
    title_->SetVisible(false);
  }
}

gfx::Insets BraveTab::GetInsets() const {
  // As close button has more padding, it seems favicon is too close to the left
  // edge of the tab left border comppared with close button. Give additional
  // left padding to make both visible with same space from tab border.
  // See https://www.github.com/brave/brave-browser/issues/30469.
  auto insets = Tab::GetInsets();
  insets.set_left(insets.left() + kExtraLeftPadding);
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

void BraveTab::SetData(TabRendererData data) {
  const bool data_changed = data != data_;
  Tab::SetData(std::move(data));

  // Our vertical tab uses CompoundTabContainer.
  // When tab is moved from the group by pinning, it's moved to
  // pinned TabContainerImpl before its tab group id is cleared.
  // And it causes runtime crash as using this tab from pinned TabContainerImpl
  // has assumption that it's not included in any group.
  // So, clear in-advance when tab enters to pinned TabContainerImpl.
  if (data_changed &&
      tabs::utils::ShouldShowVerticalTabs(controller()->GetBrowser()) &&
      data_.pinned) {
    SetGroup(std::nullopt);
  }
}

bool BraveTab::IsActive() const {
  // When SideBySide is enabled, chromium gives true if tab is in split tab even
  // it's not active. We want to give true only for current active tab.
  return controller_->IsActiveTab(this);
}

bool BraveTab::HandleKeyEvent(views::Textfield* sender,
                              const ui::KeyEvent& key_event) {
  if (key_event.type() != ui::EventType::kKeyPressed) {
    return false;
  }

  switch (key_event.key_code()) {
    case ui::VKEY_ESCAPE:
      // Cancel the rename on Escape key press.
      ExitRenameMode();
      return true;
    case ui::VKEY_RETURN:
      // Commit the rename on Enter key press.
      CommitRename();
      return true;
    default:
      break;
  }

  return false;
}

void BraveTab::CommitRename() {
  auto text = rename_textfield_->GetText();
  controller_->SetCustomTitleForTab(
      this,
      text.empty() ? std::nullopt : std::make_optional(std::u16string(text)));
  ExitRenameMode();
}

void BraveTab::ExitRenameMode() {
  CHECK(in_renaming_mode());

  rename_textfield_->SetVisible(false);
  title_->SetVisible(true);

  rename_textfield_->SetText(std::u16string());
}

void BraveTab::UpdateRenameTextfieldBounds() {
  int constexpr kHeight = 18;

  // Update the bounds of the rename textfield to match the title bounds.
  auto bounds = title_->bounds();
  bounds.set_y(bounds.CenterPoint().y() - kHeight / 2);
  bounds.set_height(kHeight);
  rename_textfield_->SetBoundsRect(bounds);
}
