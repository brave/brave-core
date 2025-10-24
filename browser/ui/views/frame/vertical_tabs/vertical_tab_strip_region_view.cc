/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/frame/vertical_tabs/vertical_tab_strip_region_view.h"

#include <algorithm>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

#include "base/check.h"
#include "base/check_op.h"
#include "base/command_line.h"
#include "base/dcheck_is_on.h"
#include "base/feature_list.h"
#include "base/functional/bind.h"
#include "base/strings/string_split.h"
#include "brave/app/vector_icons/vector_icons.h"
#include "brave/browser/ui/brave_browser.h"
#include "brave/browser/ui/color/brave_color_id.h"
#include "brave/browser/ui/tabs/brave_tab_prefs.h"
#include "brave/browser/ui/tabs/features.h"
#include "brave/browser/ui/views/brave_tab_search_bubble_host.h"
#include "brave/browser/ui/views/frame/brave_contents_view_util.h"
#include "brave/browser/ui/views/tabs/brave_new_tab_button.h"
#include "brave/browser/ui/views/tabs/brave_tab_search_button.h"
#include "brave/browser/ui/views/tabs/brave_tab_strip_layout_helper.h"
#include "brave/browser/ui/views/tabs/switches.h"
#include "brave/browser/ui/views/tabs/vertical_tab_utils.h"
#include "brave/components/constants/pref_names.h"
#include "brave/components/vector_icons/vector_icons.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/themes/theme_properties.h"
#include "chrome/browser/ui/color/chrome_color_id.h"
#include "chrome/browser/ui/exclusive_access/exclusive_access_manager.h"
#include "chrome/browser/ui/exclusive_access/fullscreen_controller.h"
#include "chrome/browser/ui/frame/window_frame_util.h"
#include "chrome/browser/ui/tabs/features.h"
#include "chrome/browser/ui/tabs/tab_style.h"
#include "chrome/browser/ui/ui_features.h"
#include "chrome/browser/ui/views/chrome_layout_provider.h"
#include "chrome/browser/ui/views/frame/browser_non_client_frame_view.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/tabs/tab_strip_scroll_container.h"
#include "chrome/browser/ui/views/toolbar/toolbar_button.h"
#include "chrome/browser/ui/views/toolbar/toolbar_ink_drop_util.h"
#include "chrome/browser/ui/views/toolbar/toolbar_view.h"
#include "chrome/common/pref_names.h"
#include "chrome/grit/generated_resources.h"
#include "components/prefs/pref_service.h"
#include "ui/base/cursor/cursor.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/compositor/compositor.h"
#include "ui/compositor/layer.h"
#include "ui/display/screen.h"
#include "ui/events/event_observer.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/paint_vector_icon.h"
#include "ui/gfx/scoped_canvas.h"
#include "ui/views/accessibility/accessibility_paint_checks.h"
#include "ui/views/animation/ink_drop.h"
#include "ui/views/controls/highlight_path_generator.h"
#include "ui/views/controls/image_view.h"
#include "ui/views/controls/resize_area.h"
#include "ui/views/controls/scroll_view.h"
#include "ui/views/event_monitor.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/layout/fill_layout.h"
#include "ui/views/layout/flex_layout.h"
#include "ui/views/layout/layout_types.h"
#include "ui/views/view_utils.h"

#if BUILDFLAG(IS_WIN)
#include "ui/views/win/hwnd_util.h"
#endif

#if !BUILDFLAG(IS_MAC)
#include "chrome/app/chrome_command_ids.h"
#endif

namespace {

constexpr int kHeaderInset = tabs::kMarginForVerticalTabContainers;
constexpr int kSeparatorHeight = 1;

// Use toolbar button's ink drop effect.
class ToggleButton : public ToolbarButton {
  METADATA_HEADER(ToggleButton, ToolbarButton)
 public:
  ToggleButton(PressedCallback callback,
               BraveVerticalTabStripRegionView* region_view)
      : ToolbarButton(std::move(callback)), region_view_(*region_view) {
    SetVectorIcon(kVerticalTabStripToggleButtonIcon);
    SetPreferredSize(gfx::Size{GetIconWidth(), GetIconWidth()});
    SetHorizontalAlignment(gfx::ALIGN_CENTER);
  }
  ~ToggleButton() override = default;

  // ToolbarButton:
  void OnThemeChanged() override {
    ToolbarButton::OnThemeChanged();
    SetHighlighted(region_view_->state() ==
                   BraveVerticalTabStripRegionView::State::kExpanded);
  }

  void StateChanged(ButtonState old_state) override {
    ToolbarButton::StateChanged(old_state);

    if (GetState() == views::Button::STATE_NORMAL) {
      // Double check highlight state after changing state to normal. Dragging
      // the button can make the highlight effect hidden.
      // https://github.com/brave/brave-browser/issues/31421
      SetHighlighted(region_view_->state() ==
                     BraveVerticalTabStripRegionView::State::kExpanded);
    }
  }

  std::u16string GetRenderedTooltipText(const gfx::Point& p) const override {
    if (region_view_->state() ==
        BraveVerticalTabStripRegionView::State::kExpanded) {
      return l10n_util::GetStringUTF16(IDS_VERTICAL_TABS_MINIMIZE);
    }

    // When it's minimized or floating.
    return l10n_util::GetStringUTF16(IDS_VERTICAL_TABS_EXPAND);
  }

  constexpr static int GetIconWidth() { return tabs::kVerticalTabHeight; }

 private:
  raw_ref<BraveVerticalTabStripRegionView> region_view_;
};

BEGIN_METADATA(ToggleButton)
END_METADATA

class ShortcutBox : public views::View {
  METADATA_HEADER(ShortcutBox, views::View)
 public:
  explicit ShortcutBox(const std::u16string& shortcut_text) {
    constexpr int kChildSpacing = 4;
    SetLayoutManager(std::make_unique<views::BoxLayout>(
        views::BoxLayout::Orientation::kHorizontal, gfx::Insets(),
        kChildSpacing));

    std::vector<std::u16string> tokens = base::SplitString(
        shortcut_text, u"+", base::KEEP_WHITESPACE, base::SPLIT_WANT_ALL);
    for (const auto& token : tokens) {
      AddShortcutPart(token);
    }
  }

 private:
  void AddShortcutPart(const std::u16string& text) {
    constexpr int kFontSize = 12;
    auto* shortcut_part = AddChildView(std::make_unique<views::Label>(text));
    shortcut_part->SetHorizontalAlignment(
        gfx::HorizontalAlignment::ALIGN_CENTER);
    shortcut_part->SetVerticalAlignment(gfx::VerticalAlignment::ALIGN_MIDDLE);
    const auto shortcut_font = shortcut_part->font_list();
    shortcut_part->SetFontList(shortcut_font.DeriveWithSizeDelta(
        kFontSize - shortcut_font.GetFontSize()));
    shortcut_part->SetEnabledColor(kColorBraveVerticalTabNTBShortcutTextColor);
    shortcut_part->SetBorder(views::CreateRoundedRectBorder(
        /*thickness*/ 1, /*radius*/ 4, kColorBraveVerticalTabSeparator));

    // Give padding and set minimum to width.
    auto preferred_size = shortcut_part->GetPreferredSize();
    preferred_size.Enlarge(4, 0);
    constexpr int kMinWidth = 18;
    preferred_size.set_width(std::max(kMinWidth, preferred_size.width()));
    shortcut_part->SetPreferredSize(preferred_size);
  }
};

BEGIN_METADATA(ShortcutBox)
END_METADATA

class VerticalTabNewTabButton : public BraveNewTabButton {
  METADATA_HEADER(VerticalTabNewTabButton, BraveNewTabButton)
 public:
  VerticalTabNewTabButton(TabStripController* tab_strip_controller,
                          PressedCallback callback,
                          const std::u16string& shortcut_text)
      : BraveNewTabButton(tab_strip_controller,
                          std::move(callback),
                          kLeoPlusAddIcon) {
    // Turn off inkdrop to have same bg color with tab's.
    views::InkDrop::Get(this)->SetMode(views::InkDropHost::InkDropMode::OFF);

    // We're going to use flex layout for children of this class. Other children
    // from base classes should be handled out of flex layout.
    for (views::View* child : children()) {
      child->SetProperty(views::kViewIgnoredByLayoutKey, true);
    }

    SetNotifyEnterExitOnChild(true);

    constexpr int kNewTabVerticalPadding = 8;
    constexpr int kNewTabHorizontalPadding = 7;
    SetLayoutManager(std::make_unique<views::FlexLayout>())
        ->SetOrientation(views::LayoutOrientation::kHorizontal)
        .SetCrossAxisAlignment(views::LayoutAlignment::kStretch)
        .SetInteriorMargin(
            gfx::Insets::VH(kNewTabHorizontalPadding, kNewTabVerticalPadding));

    plus_icon_ = AddChildView(std::make_unique<views::ImageView>());
    plus_icon_->SetHorizontalAlignment(views::ImageView::Alignment::kCenter);
    plus_icon_->SetVerticalAlignment(views::ImageView::Alignment::kCenter);
    plus_icon_->SetImage(ui::ImageModel::FromVectorIcon(
        kLeoPlusAddIcon, kColorBraveVerticalTabNTBIconColor,
        /* icon_size= */ 16));
    plus_icon_->SetProperty(
        views::kFlexBehaviorKey,
        views::FlexSpecification(views::MinimumFlexSizeRule::kPreferred,
                                 views::MaximumFlexSizeRule::kPreferred)
            .WithOrder(1));

    text_ = AddChildView(std::make_unique<views::Label>(
        l10n_util::GetStringUTF16(IDS_ACCNAME_NEWTAB)));
    text_->SetHorizontalAlignment(gfx::HorizontalAlignment::ALIGN_LEFT);
    text_->SetVerticalAlignment(gfx::VerticalAlignment::ALIGN_MIDDLE);
    constexpr int kGapBetweenIconAndText = 16;
    text_->SetProperty(views::kMarginsKey,
                       gfx::Insets::TLBR(0, kGapBetweenIconAndText, 0, 0));
    text_->SetProperty(views::kFlexBehaviorKey,
                       views::FlexSpecification(
                           views::MinimumFlexSizeRule::kPreferredSnapToZero,
                           views::MaximumFlexSizeRule::kPreferred)
                           .WithOrder(3));

    constexpr int kFontSize = 12;
    const auto text_font = text_->font_list();
    text_->SetFontList(
        text_font.DeriveWithSizeDelta(kFontSize - text_font.GetFontSize()));
    text_->SetEnabledColor(kColorBraveVerticalTabNTBTextColor);

    auto* spacer = AddChildView(std::make_unique<views::View>());
    spacer->SetProperty(
        views::kFlexBehaviorKey,
        views::FlexSpecification(views::MinimumFlexSizeRule::kScaleToZero,
                                 views::MaximumFlexSizeRule::kUnbounded)
            .WithOrder(4));

    auto* shortcut_box =
        AddChildView(std::make_unique<ShortcutBox>(shortcut_text));
    shortcut_box->SetProperty(
        views::kMarginsKey, gfx::Insets::TLBR(0, kGapBetweenIconAndText, 0, 0));
    shortcut_box->SetProperty(
        views::kFlexBehaviorKey,
        views::FlexSpecification(views::MinimumFlexSizeRule::kScaleToZero,
                                 views::MaximumFlexSizeRule::kPreferred)
            .WithOrder(2));

    SetTooltipText(l10n_util::GetStringUTF16(IDS_TOOLTIP_NEW_TAB));
    SetAccessibleName(l10n_util::GetStringUTF16(IDS_ACCNAME_NEWTAB));
  }

  ~VerticalTabNewTabButton() override = default;

  gfx::Insets GetInsets() const override {
    // This button doesn't need any insets. Invalidate parent's one.
    return gfx::Insets();
  }

  void StateChanged(ButtonState old_state) override {
    BraveNewTabButton::StateChanged(old_state);
    UpdateColors();
  }

 private:
  void UpdateColors() override {
    auto* widget = GetWidget();
    if (!widget || widget->IsClosed()) {
      // Don't update colors if the widget is closed. Otherwise, it may cause
      // crash.
      return;
    }

    BraveNewTabButton::UpdateColors();

    int bg_color_id = kColorToolbar;
    if (GetState() == views::Button::STATE_PRESSED) {
      bg_color_id = kColorBraveVerticalTabActiveBackground;
    } else if (GetState() == views::Button::STATE_HOVERED) {
      bg_color_id = kColorBraveVerticalTabHoveredBackground;
    }

    SetBackground(
        views::CreateRoundedRectBackground(bg_color_id, GetCornerRadius()));
  }

  raw_ptr<views::ImageView> plus_icon_ = nullptr;
  raw_ptr<views::Label> text_ = nullptr;
};

BEGIN_METADATA(VerticalTabNewTabButton)
END_METADATA

class ResettableResizeArea : public views::ResizeArea {
  METADATA_HEADER(ResettableResizeArea, views::ResizeArea)
 public:
  explicit ResettableResizeArea(BraveVerticalTabStripRegionView* region_view)
      : ResizeArea(region_view), region_view_(region_view) {}
  ~ResettableResizeArea() override = default;

  // views::ResizeArea
  void OnMouseReleased(const ui::MouseEvent& event) override {
    ResizeArea::OnMouseReleased(event);

    if (event.IsOnlyLeftMouseButton() && event.GetClickCount() > 1) {
      region_view_->ResetExpandedWidth();
    }
  }

 private:
  raw_ptr<BraveVerticalTabStripRegionView> region_view_;
};

BEGIN_METADATA(ResettableResizeArea)
END_METADATA

}  // namespace

class VerticalTabStripScrollContentsView : public views::View {
  METADATA_HEADER(VerticalTabStripScrollContentsView, views::View)
 public:
  VerticalTabStripScrollContentsView() {
    SetLayoutManager(std::make_unique<views::FillLayout>());
  }
  ~VerticalTabStripScrollContentsView() override = default;

  // views::View:
  void ChildPreferredSizeChanged(views::View* child) override {
    if (base::FeatureList::IsEnabled(tabs::kScrollableTabStrip)) {
      return;
    }

    PreferredSizeChanged();
  }

  void OnPaintBackground(gfx::Canvas* canvas) override {
    canvas->DrawColor(GetColorProvider()->GetColor(kColorToolbar));
  }
};

BEGIN_METADATA(VerticalTabStripScrollContentsView)
END_METADATA

class BraveVerticalTabStripRegionView::HeaderView : public views::View {
  METADATA_HEADER(HeaderView, views::View)
 public:
  HeaderView(views::Button::PressedCallback toggle_callback,
             BraveVerticalTabStripRegionView* region_view,
             BrowserWindowInterface* browser_window_interface)
      : region_view_(region_view), tab_strip_(region_view->tab_strip()) {
    SetBorder(views::CreateEmptyBorder(gfx::Insets(kHeaderInset)));

    layout_ = SetLayoutManager(std::make_unique<views::BoxLayout>(
        views::BoxLayout::Orientation::kHorizontal));
    layout_->set_cross_axis_alignment(
        views::BoxLayout::CrossAxisAlignment::kStretch);

    toggle_button_ = AddChildView(std::make_unique<ToggleButton>(
        std::move(toggle_callback), region_view));

    spacer_ = AddChildView(std::make_unique<views::View>());

    vertical_tab_on_right_.Init(
        brave_tabs::kVerticalTabsOnRight,
        region_view_->browser()->profile()->GetPrefs(),
        base::BindRepeating(&HeaderView::OnVerticalTabPositionChanged,
                            base::Unretained(this)));
    OnVerticalTabPositionChanged();
  }
  ~HeaderView() override = default;

  ToggleButton* toggle_button() { return toggle_button_; }

  // views::View:
  void OnThemeChanged() override {
    View::OnThemeChanged();

    SetBackground(views::CreateSolidBackground(
        GetColorProvider()->GetColor(kColorToolbar)));
  }

 private:
  void OnVerticalTabPositionChanged() {
    std::vector<views::View*> new_children = {toggle_button_.get(),
                                              spacer_.get()};
    if (tabs::utils::IsVerticalTabOnRight(region_view_->browser())) {
      std::reverse(new_children.begin(), new_children.end());
    }

    CHECK_EQ(children().size(), new_children.size());
    if (children().front() == new_children.front()) {
      // In order to make sure that |spacer_| has flex behavior on start up.
      layout_->SetFlexForView(
          spacer_, 1 /* resize |spacer| to fill the rest of space */);
      return;
    }

    // View::ReorderChildView() didn't work for us. So remove child views and
    // add them again.
    while (!children().empty()) {
      RemoveChildView(children().front());
    }

    std::ranges::for_each(new_children, [&](auto* v) { AddChildView(v); });
    layout_->SetFlexForView(spacer_,
                            1 /* resize |spacer| to fill the rest of space */);
  }

  raw_ptr<views::BoxLayout> layout_ = nullptr;
  raw_ptr<BraveVerticalTabStripRegionView> region_view_ = nullptr;
  raw_ptr<const TabStrip> tab_strip_ = nullptr;
  raw_ptr<ToggleButton> toggle_button_ = nullptr;
  raw_ptr<views::View> spacer_ = nullptr;
  BooleanPrefMember vertical_tab_on_right_;
};

using HeaderView = BraveVerticalTabStripRegionView::HeaderView;
BEGIN_METADATA(HeaderView)
END_METADATA

// Double checks mouse hovered state. When there's border around the region view
// or window resizable area the mouse enter/exit event might not be correct.
// Thus, observes mouse events that passes the window.
class BraveVerticalTabStripRegionView::MouseWatcher : public ui::EventObserver {
 public:
  explicit MouseWatcher(BraveVerticalTabStripRegionView* region_view)
      : region_view_(region_view),
        event_monitor_(views::EventMonitor::CreateWindowMonitor(
            this,
            region_view_->GetWidget()->GetNativeWindow(),
            {ui::EventType::kMousePressed, ui::EventType::kMouseEntered,
             ui::EventType::kMouseExited})) {}

  // ui::EventObserver:
  void OnEvent(const ui::Event& event) override {
    switch (event.type()) {
      case ui::EventType::kMouseEntered:
        region_view_->OnMouseEntered();
        break;
      case ui::EventType::kMousePressed:
        region_view_->OnMousePressedInTree();
        break;
      case ui::EventType::kMouseExited:
        region_view_->OnMouseExited();
        break;
      default:
        break;
    }
  }

 private:
  raw_ptr<BraveVerticalTabStripRegionView> region_view_;
  std::unique_ptr<views::EventMonitor> event_monitor_;
};

BraveVerticalTabStripRegionView::BraveVerticalTabStripRegionView(
    BrowserView* browser_view,
    TabStripRegionView* region_view)
    : views::AnimationDelegateViews(this),
      browser_view_(browser_view),
      browser_(browser_view->browser()),
      original_region_view_(region_view),
      tab_style_(TabStyle::Get()) {
  SetNotifyEnterExitOnChild(true);

  // The default state is kExpanded, so reset animation state to 1.0.
  width_animation_.Reset(1.0);

  header_view_ = AddChildView(std::make_unique<HeaderView>(
      base::BindRepeating(&BraveVerticalTabStripRegionView::ToggleState,
                          base::Unretained(this)),
      this, browser_));
  contents_view_ =
      AddChildView(std::make_unique<VerticalTabStripScrollContentsView>());
  header_view_->toggle_button()->SetHighlighted(state_ == State::kExpanded);
  separator_ = AddChildView(std::make_unique<views::View>());
  separator_->SetBackground(
      views::CreateSolidBackground(kColorBraveVerticalTabSeparator));
  new_tab_button_ = AddChildView(std::make_unique<VerticalTabNewTabButton>(
      original_region_view_->tab_strip_->controller(),
      base::BindRepeating(&TabStrip::NewTabButtonPressed,
                          base::Unretained(original_region_view_->tab_strip_)),
      GetShortcutTextForNewTabButton(browser_view)));

  resize_area_ = AddChildView(std::make_unique<ResettableResizeArea>(this));
  SetBackground(views::CreateSolidBackground(kColorToolbar));

  auto* prefs = browser_->profile()->GetPrefs();

  sidebar_side_.Init(prefs::kSidePanelHorizontalAlignment, prefs,
                     base::BindRepeating(
                         &BraveVerticalTabStripRegionView::OnBrowserPanelsMoved,
                         base::Unretained(this)));

  expanded_width_pref_.Init(
      brave_tabs::kVerticalTabsExpandedWidth, prefs,
      base::BindRepeating(
          &BraveVerticalTabStripRegionView::OnExpandedWidthPrefChanged,
          base::Unretained(this)));
  OnExpandedWidthPrefChanged();

  show_vertical_tabs_.Init(
      brave_tabs::kVerticalTabsEnabled, prefs,
      base::BindRepeating(
          &BraveVerticalTabStripRegionView::OnShowVerticalTabsPrefChanged,
          base::Unretained(this)));
  UpdateLayout();

  collapsed_pref_.Init(
      brave_tabs::kVerticalTabsCollapsed, prefs,
      base::BindRepeating(
          &BraveVerticalTabStripRegionView::OnCollapsedPrefChanged,
          base::Unretained(this)));
  OnCollapsedPrefChanged();

  expanded_state_per_window_pref_.Init(
      brave_tabs::kVerticalTabsExpandedStatePerWindow, prefs,
      base::BindRepeating(
          &BraveVerticalTabStripRegionView::OnExpandedStatePerWindowPrefChanged,
          base::Unretained(this)));

  floating_mode_pref_.Init(
      brave_tabs::kVerticalTabsFloatingEnabled, prefs,
      base::BindRepeating(
          &BraveVerticalTabStripRegionView::OnFloatingModePrefChanged,
          base::Unretained(this)));

#if BUILDFLAG(IS_MAC)
  show_toolbar_on_fullscreen_pref_.Init(
      prefs::kShowFullscreenToolbar, prefs,
      base::BindRepeating(
          &BraveVerticalTabStripRegionView::OnFullscreenStateChanged,
          base::Unretained(this)));
#endif

  vertical_tab_on_right_.Init(
      brave_tabs::kVerticalTabsOnRight, browser()->profile()->GetPrefs(),
      base::BindRepeating(
          &BraveVerticalTabStripRegionView::OnBrowserPanelsMoved,
          base::Unretained(this)));

  if (base::FeatureList::IsEnabled(
          tabs::features::kBraveVerticalTabHideCompletely)) {
    hide_completely_when_collapsed_pref_.Init(
        brave_tabs::kVerticalTabsHideCompletelyWhenCollapsed, prefs,
        base::BindRepeating(&BraveVerticalTabStripRegionView::
                                OnHideComopletelyWhenCollapsedPrefChanged,
                            base::Unretained(this)));
  }

  widget_observation_.Observe(browser_view->GetWidget());

  // Note: This should happen after all the PrefMembers have been initialized.
  OnFloatingModePrefChanged();

  set_context_menu_controller(this);
}

BraveVerticalTabStripRegionView::~BraveVerticalTabStripRegionView() {
  // We need to move tab strip region to its original parent to avoid crash
  // during drag and drop session.
  UpdateLayout(true);
}

void BraveVerticalTabStripRegionView::ToggleState() {
  if (state_ == State::kExpanded) {
    collapsed_pref_.SetValue(true);
    SetState(State::kCollapsed);
  } else {
    collapsed_pref_.SetValue(false);
    SetState(State::kExpanded);
  }
}

void BraveVerticalTabStripRegionView::OnWidgetActivationChanged(
    views::Widget* widget,
    bool active) {
  if (active) {
    if (*floating_mode_pref_ && IsMouseHovered()) {
      ScheduleFloatingModeTimer();
    }
    return;
  }

  // When parent widget is deactivated, we should collapse vertical tab
  mouse_enter_timer_.Stop();
  if (state_ == State::kFloating) {
    SetState(State::kCollapsed);
  }
}

void BraveVerticalTabStripRegionView::OnWidgetDestroying(
    views::Widget* widget) {
  widget_observation_.Reset();
}

void BraveVerticalTabStripRegionView::OnFullscreenStateChanged() {
  if (!tabs::utils::ShouldShowVerticalTabs(browser_)) {
    return;
  }

  if (IsFloatingEnabledForBrowserFullscreen()) {
    width_animation_.Stop();
    SetVisible(false);
    SetState(State::kCollapsed);
  } else {
    SetVisible(true);
  }

  PreferredSizeChanged();
}

void BraveVerticalTabStripRegionView::ListenFullscreenChanges() {
  auto* fullscreen_controller = GetFullscreenController();
  DCHECK(fullscreen_controller);
  fullscreen_observation_.Observe(fullscreen_controller);
}

void BraveVerticalTabStripRegionView::StopListeningFullscreenChanges() {
  fullscreen_observation_.Reset();
}

FullscreenController* BraveVerticalTabStripRegionView::GetFullscreenController()
    const {
  auto* exclusive_access_manager =
      browser_->GetFeatures().exclusive_access_manager();
  if (!exclusive_access_manager) {
    return nullptr;
  }

  return exclusive_access_manager->fullscreen_controller();
}

bool BraveVerticalTabStripRegionView::IsTabFullscreen() const {
  const auto* fullscreen_controller = GetFullscreenController();
  return fullscreen_controller &&
         fullscreen_controller->IsWindowFullscreenForTabOrPending();
}

bool BraveVerticalTabStripRegionView::IsBrowserFullscren() const {
  const auto* fullscreen_controller = GetFullscreenController();
  return fullscreen_controller &&
         fullscreen_controller->IsFullscreenForBrowser();
}

bool BraveVerticalTabStripRegionView::
    ShouldShowVerticalTabsInBrowserFullscreen() const {
#if BUILDFLAG(IS_MAC)
  // Refer to "Always show toolbar in Fullscreen" pref in the app menu
  return show_toolbar_on_fullscreen_pref_.GetValue();
#else
  return false;
#endif
}

void BraveVerticalTabStripRegionView::SetState(State state) {
  if (state_ == state) {
    return;
  }

  mouse_enter_timer_.Stop();
  mouse_exit_timer_.Stop();

  last_state_ = std::exchange(state_, state);
  resize_area_->SetEnabled(state == State::kExpanded);
  header_view_->toggle_button()->SetHighlighted(state == State::kExpanded);

  if (!tabs::utils::ShouldShowVerticalTabs(browser_)) {
    // This can happen when "float on mouse hover" is enabled and tab strip
    // orientation has been changed.
    return;
  }

  auto tab_strip = original_region_view_->tab_strip_;
  tab_strip->SetAvailableWidthCallback(base::BindRepeating(
      &BraveVerticalTabStripRegionView::GetAvailableWidthForTabContainer,
      base::Unretained(this)));
  tab_strip->tab_container_->InvalidateIdealBounds();
  tab_strip->tab_container_->CompleteAnimationAndLayout();

  if (gfx::Animation::ShouldRenderRichAnimation()) {
    state_ == State::kCollapsed ? width_animation_.Hide()
                                : width_animation_.Show();
  } else if (state_ == State::kCollapsed) {
    // Call the callback immediately if no animation.
    OnCollapseAnimationEnded();
  }

  if (!GetVisible() && state_ != State::kCollapsed) {
    // This can happen when
    // * vertical tab strip is expanded temporarily in browser fullscreen mode.
    // * vertical tab strip is shown from collapsed state with
    // tabs::features::kBraveVerticalTabHideCompletely on.
    SetVisible(true);
  }

  PreferredSizeChanged();
  UpdateBorder();
}

void BraveVerticalTabStripRegionView::SetExpandedWidth(int dest_width) {
  if (expanded_width_ == dest_width) {
    return;
  }

  expanded_width_ = dest_width;

  if (expanded_width_ != *expanded_width_pref_) {
    expanded_width_pref_.SetValue(expanded_width_);
  }

  PreferredSizeChanged();
}

void BraveVerticalTabStripRegionView::UpdateStateAfterDragAndDropFinished(
    State original_state) {
  DCHECK_NE(original_state, State::kExpanded)
      << "As per ExpandTabStripForDragging(), this shouldn't happen";

  if (IsFloatingVerticalTabsEnabled() && IsMouseHovered()) {
    SetState(State::kFloating);
    return;
  }

  SetState(State::kCollapsed);
}

BraveVerticalTabStripRegionView::ScopedStateResetter
BraveVerticalTabStripRegionView::ExpandTabStripForDragging() {
  if (state_ == State::kExpanded) {
    return {};
  }

  auto resetter = std::make_unique<base::ScopedClosureRunner>(base::BindOnce(
      &BraveVerticalTabStripRegionView::UpdateStateAfterDragAndDropFinished,
      weak_factory_.GetWeakPtr(), state_));

  SetState(State::kExpanded);
  // In this case, we dont' wait for the widget bounds to be changed so that
  // tab drag controller can layout tabs properly.
  SetSize(GetPreferredSize());

  return resetter;
}

gfx::Vector2d BraveVerticalTabStripRegionView::GetOffsetForDraggedTab() const {
  return {0, header_view_->GetPreferredSize().height()};
}

int BraveVerticalTabStripRegionView::GetAvailableWidthForTabContainer() {
  DCHECK(tabs::utils::ShouldShowVerticalTabs(browser_));
  return GetPreferredWidthForState(state_, /*include_border=*/false,
                                   /*ignore_animation=*/false);
}

gfx::Size BraveVerticalTabStripRegionView::CalculatePreferredSize(
    const views::SizeBounds& available_size) const {
  return GetPreferredSizeForState(state_, /*include_border=*/true,
                                  /*ignore_animation=*/false);
}

gfx::Size BraveVerticalTabStripRegionView::GetMinimumSize() const {
  if (IsFloatingEnabledForBrowserFullscreen()) {
    // Vertical tab strip always overlaps the contents area.
    return {};
  }

  if (state_ == State::kFloating) {
    return GetPreferredSizeForState(State::kCollapsed,
                                    /*include_border=*/true,
                                    /*ignore_animation=*/true);
  }

  return GetPreferredSizeForState(state_, /*include_border=*/true,
                                  /*ignore_animation=*/true);
}

void BraveVerticalTabStripRegionView::Layout(PassKey) {
  // As we have to update ScrollView's viewport size and its contents size,
  // laying out children manually will be more handy.
  const auto contents_bounds = GetContentsBounds();

  const gfx::Size header_size{contents_bounds.width(),
                              tabs::kVerticalTabHeight + kHeaderInset * 2};
  header_view_->SetBoundsRect(gfx::Rect(contents_bounds.origin(), header_size));

  constexpr int kNewTabButtonHeight = tabs::kVerticalTabHeight;
  const int contents_view_max_height =
      contents_bounds.height() - kNewTabButtonHeight - header_view_->height();
  const int contents_view_preferred_height =
      tab_strip()->GetPreferredSize().height();
  contents_view_->SetBoundsRect(gfx::Rect(
      header_view_->bounds().bottom_left(),
      gfx::Size(
          contents_bounds.width(),
          std::min(contents_view_max_height, contents_view_preferred_height))));

  gfx::Rect separator_bounds(
      contents_view_->bounds().bottom_left(),
      gfx::Size(contents_bounds.width(), kSeparatorHeight));
  separator_bounds.Inset(
      gfx::Insets::VH(0, tabs::kMarginForVerticalTabContainers));
  separator_->SetBoundsRect(separator_bounds);
  gfx::Rect new_tab_button_bounds(
      separator_->bounds().bottom_left(),
      gfx::Size(separator_bounds.width(), kNewTabButtonHeight));
  new_tab_button_bounds.Offset(0, tabs::kMarginForVerticalTabContainers);
  new_tab_button_->SetBoundsRect(new_tab_button_bounds);

  // Put resize area, overlapped with contents.
  if (vertical_tab_on_right_.GetPrefName().empty()) {
    // Not initialized yet.
    return;
  }

  constexpr int kResizeAreaWidth = 4;
  resize_area_->SetBounds(
      *vertical_tab_on_right_ ? 0 : width() - kResizeAreaWidth,
      contents_bounds.y(), kResizeAreaWidth, contents_bounds.height());
}

void BraveVerticalTabStripRegionView::OnShowVerticalTabsPrefChanged() {
  UpdateLayout(/* in_destruction= */ false);

  if (!tabs::utils::ShouldShowVerticalTabs(browser_) &&
      state_ == State::kFloating) {
    mouse_enter_timer_.Stop();
    SetState(State::kCollapsed);
  }

  UpdateBorder();
}

void BraveVerticalTabStripRegionView::OnBrowserPanelsMoved() {
  UpdateBorder();
  PreferredSizeChanged();
}

void BraveVerticalTabStripRegionView::UpdateLayout(bool in_destruction) {
  if (tabs::utils::ShouldShowVerticalTabs(browser_) && !in_destruction) {
    if (!Contains(original_region_view_)) {
      original_parent_of_region_view_ = original_region_view_->parent();
      original_parent_of_region_view_->RemoveChildView(original_region_view_);
      contents_view_->AddChildView(original_region_view_.get());
    }

    static_cast<views::FlexLayout*>(original_region_view_->GetLayoutManager())
        ->SetOrientation(views::LayoutOrientation::kVertical);
    if (base::FeatureList::IsEnabled(tabs::kScrollableTabStrip)) {
      auto* scroll_container = GetTabStripScrollContainer();
      scroll_container->SetLayoutManager(std::make_unique<views::FillLayout>());
      scroll_container->scroll_view_->SetTreatAllScrollEventsAsHorizontal(
          false);
      scroll_container->scroll_view_->SetVerticalScrollBarMode(
          views::ScrollView::ScrollBarMode::kHiddenButEnabled);
      scroll_container->overflow_view_->SetOrientation(
          views::LayoutOrientation::kVertical);
    }
  } else {
    if (Contains(original_region_view_)) {
      contents_view_->RemoveChildView(original_region_view_);
      // TabStrip should be added before other views so that we can preserve
      // the z-order. At this moment, tab strip is the first child of the
      // parent view.
      // https://github.com/chromium/chromium/blob/bdcef78b63f64119bbe950386b2495a045629f0e/chrome/browser/ui/views/frame/browser_view.cc#L904
      original_parent_of_region_view_->AddChildViewAt(
          original_region_view_.get(), 0);
    }

    static_cast<views::FlexLayout*>(original_region_view_->GetLayoutManager())
        ->SetOrientation(views::LayoutOrientation::kHorizontal);
    if (base::FeatureList::IsEnabled(tabs::kScrollableTabStrip)) {
      auto* scroll_container = GetTabStripScrollContainer();
      scroll_container->SetLayoutManager(std::make_unique<views::FillLayout>())
          ->SetMinimumSizeEnabled(true);
      scroll_container->scroll_view_->SetTreatAllScrollEventsAsHorizontal(true);
      scroll_container->scroll_view_->SetVerticalScrollBarMode(
          views::ScrollView::ScrollBarMode::kDisabled);
      scroll_container->overflow_view_->SetOrientation(
          views::LayoutOrientation::kHorizontal);
    }
  }

  UpdateNewTabButtonVisibility();

  PreferredSizeChanged();
  DeprecatedLayoutImmediately();
}

void BraveVerticalTabStripRegionView::OnThemeChanged() {
  View::OnThemeChanged();

  UpdateBorder();
}

void BraveVerticalTabStripRegionView::OnMouseExited(
    const ui::MouseEvent& event) {
  OnMouseExited();
}

void BraveVerticalTabStripRegionView::OnMouseExited() {
  DCHECK(GetWidget())
      << "As widget is the event sink, this is expected to be true.";
  if (GetWidget()->GetRootView()->IsMouseHovered() && !mouse_events_for_test_) {
    // On Windows, when mouse moves into the area which intersects with web
    // view, OnMouseExited() is invoked even mouse is on this view.
    return;
  }

  mouse_enter_timer_.Stop();
  if (state_ == State::kFloating) {
    ScheduleCollapseTimer();
  }
}

void BraveVerticalTabStripRegionView::OnMouseEntered(
    const ui::MouseEvent& event) {
  OnMouseEntered();
}

void BraveVerticalTabStripRegionView::OnMouseEntered() {
  if (!IsFloatingVerticalTabsEnabled()) {
    return;
  }

  // During tab dragging, this could be already expanded.
  if (state_ == State::kExpanded) {
    return;
  }

  mouse_exit_timer_.Stop();
  ScheduleFloatingModeTimer();
}

void BraveVerticalTabStripRegionView::OnMousePressedInTree() {
  if (IsFloatingVerticalTabsEnabled()) {
    return;
  }

  if (!mouse_enter_timer_.IsRunning()) {
    return;
  }

  // Restart timer when a user presses something. We consider the mouse press
  // event as the case where the user explicitly knows what they're going to do.
  // In this case, expanding vertical tabs could distract them. So we try
  // resetting the timer.
  mouse_enter_timer_.Stop();
  ScheduleFloatingModeTimer();
}

void BraveVerticalTabStripRegionView::OnBoundsChanged(
    const gfx::Rect& previous_bounds) {
  if (!tabs::utils::ShouldShowVerticalTabs(browser_)) {
    return;
  }

  if (previous_bounds.size() != size()) {
    if (GetAvailableWidthForTabContainer() != tab_strip()->width()) {
      // During/After the drag and drop session, tab strip container might have
      // ignored Layout() request. As the container bounds changed, we should
      // force it to layout.
      // https://github.com/brave/brave-browser/issues/29941
      tab_strip()->tab_container_->InvalidateIdealBounds();
      tab_strip()->tab_container_->CompleteAnimationAndLayout();
    }
  }

#if DCHECK_IS_ON()
  DCHECK(GetWidget());
  // In this mode,vertical tab strip takes a little width, such as 4px, and
  // when mouse is hovered, it expands to the full width.
  const bool is_hot_corner =
      IsBrowserFullscren() ||
      (tabs::utils::ShouldHideVerticalTabsCompletelyWhenCollapsed(browser_) &&
       state_ == State::kCollapsed);

  // Checks if the width is in valid range when it's visible.
  if (auto width = GetContentsBounds().width();
      width && !is_hot_corner && GetWidget()->IsVisible()) {
    CHECK_GE(
        width,
        tabs::kVerticalTabMinWidth + tabs::kMarginForVerticalTabContainers * 2 -
            BraveContentsViewUtil::GetRoundedCornersWebViewMargin(browser_));
  }
#endif
}

void BraveVerticalTabStripRegionView::AddedToWidget() {
  View::AddedToWidget();
  mouse_watcher_ = std::make_unique<MouseWatcher>(this);
}

void BraveVerticalTabStripRegionView::OnResize(int resize_amount,
                                               bool done_resizing) {
  CHECK_NE(state_, State::kCollapsed);

  gfx::Rect bounds_in_screen = GetLocalBounds();
  views::View::ConvertRectToScreen(this, &bounds_in_screen);

  auto cursor_position = display::Screen::Get()->GetCursorScreenPoint().x();
  if (!resize_offset_.has_value()) {
    resize_offset_ =
        (*vertical_tab_on_right_ ? bounds_in_screen.x() - cursor_position
                                 : cursor_position - bounds_in_screen.right());
  }
  // Note that we're not using |resize_amount|. The variable is offset from
  // the initial point, it grows bigger and bigger.
  auto dest_width =
      (*vertical_tab_on_right_ ? bounds_in_screen.right() - cursor_position
                               : cursor_position - bounds_in_screen.x()) -
      *resize_offset_ - GetInsets().width();
  // Passed |true| but it doesn't have any meaning becuase we always use same
  // width.
  dest_width =
      std::clamp(dest_width, tab_style_->GetPinnedWidth(/*is_split*/ true) * 3,
                 tab_style_->GetStandardWidth(/*is_split*/ true) * 2);
  if (done_resizing) {
    resize_offset_ = std::nullopt;
  }

  if (expanded_width_ == dest_width) {
    return;
  }

  // When mouse goes toward web contents area, the cursor could have been
  // changed to the normal cursor. Reset it resize cursor.
  GetWidget()->SetCursor(ui::Cursor(ui::mojom::CursorType::kEastWestResize));

  if (width_animation_.is_animating()) {
    width_animation_.Stop();
    width_animation_.Reset(state_ == State::kCollapsed ? 0 : 1);
  }

  SetExpandedWidth(dest_width);
}

void BraveVerticalTabStripRegionView::AnimationProgressed(
    const gfx::Animation* animation) {
  PreferredSizeChanged();
}

void BraveVerticalTabStripRegionView::AnimationEnded(
    const gfx::Animation* animation) {
  PreferredSizeChanged();

  if (state_ == State::kCollapsed) {
    OnCollapseAnimationEnded();
  }
}

void BraveVerticalTabStripRegionView::UpdateNewTabButtonVisibility() {
  const bool is_vertical_tabs = tabs::utils::ShouldShowVerticalTabs(browser_);
  auto* original_ntb = original_region_view_->GetNewTabButton();
  original_ntb->SetVisible(!is_vertical_tabs);
  new_tab_button_->SetVisible(is_vertical_tabs);
  separator_->SetVisible(is_vertical_tabs);
}

int BraveVerticalTabStripRegionView::GetTabStripViewportMaxHeight() const {
  // Don't depend on |contents_view_|'s current height. It could be bigger than
  // the actual viewport height.
  return GetContentsBounds().height() - header_view_->height() -
         (separator_->height() + tabs::kMarginForVerticalTabContainers) -
         new_tab_button_->height();
}

void BraveVerticalTabStripRegionView::ResetExpandedWidth() {
  auto* prefs = browser_->profile()->GetPrefs();
  prefs->ClearPref(brave_tabs::kVerticalTabsExpandedWidth);

  PreferredSizeChanged();
}

void BraveVerticalTabStripRegionView::UpdateBorder() {
  auto show_visible_border = [&]() {
    // The color provider might not be available during initialization.
    if (!GetColorProvider()) {
      return false;
    }

    if (!BraveBrowser::ShouldUseBraveWebViewRoundedCornersForContents(
            browser_)) {
      return true;
    }

    // Only show the border if the vertical tabs are enabled and in floating
    // mode, and the tabstrip is hovered.
    return tabs::utils::ShouldShowVerticalTabs(browser_) &&
           state_ == State::kFloating;
  };

  // At this point |sidebar_side_| needs to be initialized.
  CHECK(!sidebar_side_.GetPrefName().empty());

  // If the sidebar is on the same side as the vertical tab strip, we shouldn't
  // take away the margin on the vertical tabs, because the sidebar will be
  // between it and the web_contents.
  bool is_on_right =
      !vertical_tab_on_right_.GetPrefName().empty() && *vertical_tab_on_right_;
  bool sidebar_on_same_side = sidebar_side_.GetValue() == is_on_right;
  int inset =
      1 -
      (sidebar_on_same_side
           ? 0
           : BraveContentsViewUtil::GetRoundedCornersWebViewMargin(browser_));
  gfx::Insets border_insets = (is_on_right) ? gfx::Insets::TLBR(0, inset, 0, 0)
                                            : gfx::Insets::TLBR(0, 0, 0, inset);

  if (show_visible_border()) {
    SetBorder(views::CreateSolidSidedBorder(
        border_insets,
        GetColorProvider()->GetColor(kColorBraveVerticalTabSeparator)));
  } else {
    SetBorder(views::CreateEmptyBorder(border_insets));
  }

  PreferredSizeChanged();
}

void BraveVerticalTabStripRegionView::OnCollapsedPrefChanged() {
  if (!expanded_state_per_window_pref_.GetPrefName().empty() &&
      *expanded_state_per_window_pref_) {
    // On creation(when expanded_state_per_window_pref_ is empty), we set the
    // default state based on the `collapsed_pref_` even if the
    // `expanded_state_per_window_pref_` is set.
    return;
  }

  SetState(collapsed_pref_.GetValue() ? State::kCollapsed : State::kExpanded);
}

void BraveVerticalTabStripRegionView::OnFloatingModePrefChanged() {
  if (!IsFloatingVerticalTabsEnabled()) {
    if (state_ == State::kFloating) {
      SetState(State::kCollapsed);
    }
    return;
  }

  if (IsMouseHovered()) {
    ScheduleFloatingModeTimer();
  }
}

void BraveVerticalTabStripRegionView::OnExpandedStatePerWindowPrefChanged() {
  OnCollapsedPrefChanged();
  OnExpandedWidthPrefChanged();
}

void BraveVerticalTabStripRegionView::
    OnHideComopletelyWhenCollapsedPrefChanged() {
  OnFloatingModePrefChanged();
  PreferredSizeChanged();
  if (state_ == State::kCollapsed) {
    // When setting is turned on/off, we should make sure vertical tab strip is
    // getting hidden/shown.
    SetVisible(
        !tabs::utils::ShouldHideVerticalTabsCompletelyWhenCollapsed(browser_));
  }
}

void BraveVerticalTabStripRegionView::OnExpandedWidthPrefChanged() {
  if (!expanded_state_per_window_pref_.GetPrefName().empty() &&
      *expanded_state_per_window_pref_) {
    // On creation(when expanded_state_per_window_pref_ is empty), we set the
    // default state based on the `expanded_width_pref_` even if the
    // `expanded_state_per_window_pref_` is set.
    return;
  }

  SetExpandedWidth(*expanded_width_pref_);
}

gfx::Size BraveVerticalTabStripRegionView::GetPreferredSizeForState(
    State state,
    bool include_border,
    bool ignore_animation) const {
  if (!tabs::utils::ShouldShowVerticalTabs(browser_)) {
    return {};
  }

  if (IsTabFullscreen()) {
    return {};
  }

  return {GetPreferredWidthForState(state, include_border, ignore_animation),
          View::CalculatePreferredSize({}).height()};
}

int BraveVerticalTabStripRegionView::GetPreferredWidthForState(
    State state,
    bool include_border,
    bool ignore_animation) const {
  auto calculate_expanded_width = [&]() {
    return *expanded_width_pref_ + (include_border ? GetInsets().width() : 0);
  };

  auto calculate_collapsed_width = [&]() {
    if (IsFloatingEnabledForBrowserFullscreen()) {
      // In this case, vertical tab strip should be invisible but show up when
      // mouse hovers.
      // there's no border visible, so 2px is enough.
      return 2;
    }

    if (tabs::utils::ShouldHideVerticalTabsCompletelyWhenCollapsed(browser_)) {
      // Typical window frame border is 8px, so we can use 4px as vertical tab
      // space only takes inner 4px.
      return 4;
    }

    return tabs::kVerticalTabMinWidth +
           tabs::kMarginForVerticalTabContainers * 2 +
           (include_border ? GetInsets().width() : 0);
  };

  if (!ignore_animation && width_animation_.is_animating()) {
    return gfx::Tween::IntValueBetween(width_animation_.GetCurrentValue(),
                                       calculate_collapsed_width(),
                                       calculate_expanded_width());
  }

  if (state == State::kExpanded || state == State::kFloating) {
    return calculate_expanded_width();
  }

  CHECK_EQ(state, State::kCollapsed) << "If a new state was added, "
                                     << __FUNCTION__ << " should be revisited.";
  return calculate_collapsed_width();
}

TabStripScrollContainer*
BraveVerticalTabStripRegionView::GetTabStripScrollContainer() {
  CHECK(base::FeatureList::IsEnabled(tabs::kScrollableTabStrip));
  auto* scroll_container = views::AsViewClass<TabStripScrollContainer>(
      original_region_view_->tab_strip_container_);
  CHECK(scroll_container)
      << "TabStripScrollContainer is used by upstream at this moment.";
  return scroll_container;
}

bool BraveVerticalTabStripRegionView::IsFloatingVerticalTabsEnabled() const {
  return IsFloatingEnabledForBrowserFullscreen() ||
         tabs::utils::IsFloatingVerticalTabsEnabled(browser_) ||
         tabs::utils::ShouldHideVerticalTabsCompletelyWhenCollapsed(browser_);
}

bool BraveVerticalTabStripRegionView::IsFloatingEnabledForBrowserFullscreen()
    const {
  return IsBrowserFullscren() && !ShouldShowVerticalTabsInBrowserFullscreen();
}

void BraveVerticalTabStripRegionView::ScheduleFloatingModeTimer() {
  if (mouse_events_for_test_) {
    SetState(State::kFloating);
    return;
  }

  if (mouse_enter_timer_.IsRunning()) {
    return;
  }

  if (auto* widget = GetWidget();
      !widget || !widget->GetTopLevelWidget()->IsActive()) {
    // When the browser isn't active, don't schedule.
    return;
  }

  if (state_ == State::kCollapsed) {
    auto get_expand_delay = []() {
      constexpr int kDefaultDelay = 0;
      auto* cmd_line = base::CommandLine::ForCurrentProcess();
      if (!cmd_line->HasSwitch(tabs::switches::kVerticalTabExpandDelaySwitch)) {
        return kDefaultDelay;
      }

      auto delay_string = cmd_line->GetSwitchValueASCII(
          tabs::switches::kVerticalTabExpandDelaySwitch);

      int override_delay = 0;
      if (delay_string.empty() ||
          !base::StringToInt(delay_string, &override_delay)) {
        return kDefaultDelay;
      }

      return override_delay;
    };

    const auto delay = get_expand_delay();
    if (delay == 0) {
      // If the delay is 0, we should expand immediately.
      SetState(State::kFloating);
      return;
    }

    mouse_enter_timer_.Start(
        FROM_HERE, base::Milliseconds(delay),
        base::BindOnce(&BraveVerticalTabStripRegionView::SetState,
                       base::Unretained(this), State::kFloating));
  }
}

void BraveVerticalTabStripRegionView::ScheduleCollapseTimer() {
  if (state_ != State::kFloating) {
    return;
  }

  if (mouse_exit_timer_.IsRunning()) {
    return;
  }

  auto get_collapse_delay = []() {
    constexpr int kDefaultDelay = 0;
    auto* cmd_line = base::CommandLine::ForCurrentProcess();
    if (!cmd_line->HasSwitch(tabs::switches::kVerticalTabCollapseDelaySwitch)) {
      return kDefaultDelay;
    }

    auto delay_string = cmd_line->GetSwitchValueASCII(
        tabs::switches::kVerticalTabCollapseDelaySwitch);

    int override_delay = 0;
    if (delay_string.empty() ||
        !base::StringToInt(delay_string, &override_delay)) {
      return kDefaultDelay;
    }

    return override_delay;
  };

  const auto delay = get_collapse_delay();
  if (delay == 0) {
    // If the delay is 0, we should collapse immediately.
    SetState(State::kCollapsed);
    return;
  }

  mouse_exit_timer_.Start(
      FROM_HERE, base::Milliseconds(delay),
      base::BindOnce(&BraveVerticalTabStripRegionView::SetState,
                     base::Unretained(this), State::kCollapsed));
}

#if !BUILDFLAG(IS_MAC)
std::u16string BraveVerticalTabStripRegionView::GetShortcutTextForNewTabButton(
    BrowserView* browser_view) {
  if (ui::Accelerator new_tab_accelerator;
      browser_view->GetAcceleratorForCommandId(IDC_NEW_TAB,
                                               &new_tab_accelerator)) {
    return new_tab_accelerator.GetShortcutText();
  }

  return {};
}
#endif

views::LabelButton&
BraveVerticalTabStripRegionView::GetToggleButtonForTesting() {
  return *header_view_->toggle_button();
}

void BraveVerticalTabStripRegionView::OnCollapseAnimationEnded() {
  CHECK_EQ(state_, State::kCollapsed);

  if (IsFloatingEnabledForBrowserFullscreen() ||
      tabs::utils::ShouldHideVerticalTabsCompletelyWhenCollapsed(browser_)) {
    // When the animation ends, we should hide the vertical tab strip as we
    // don't want the tabstrip to be visible partially. This view only takes a
    // little width and watches mouse movement to expand itself.
    SetVisible(false);
  }
}

bool BraveVerticalTabStripRegionView::IsMenuShowing() const {
  return menu_runner_ && menu_runner_->IsRunning();
}

// Show context menu in unobscured area.
void BraveVerticalTabStripRegionView::ShowContextMenuForViewImpl(
    views::View* source,
    const gfx::Point& p,
    ui::mojom::MenuSourceType source_type) {
#if BUILDFLAG(IS_WIN)
  // Use same context menu of horizontal tab's titlebar.
  views::ShowSystemMenuAtScreenPixelLocation(views::HWNDForView(browser_view_),
                                             p);
#else
  if (IsMenuShowing()) {
    return;
  }

  menu_runner_ = std::make_unique<views::MenuRunner>(
      browser_view_->browser_widget()->GetSystemMenuModel(),
      views::MenuRunner::HAS_MNEMONICS | views::MenuRunner::CONTEXT_MENU,
      base::BindRepeating(&BraveVerticalTabStripRegionView::OnMenuClosed,
                          base::Unretained(this)));
  menu_runner_->RunMenuAt(source->GetWidget(), nullptr,
                          gfx::Rect(p, gfx::Size(0, 0)),
                          views::MenuAnchorPosition::kTopLeft, source_type);
#endif
}

void BraveVerticalTabStripRegionView::OnMenuClosed() {
  menu_runner_.reset();
}

BEGIN_METADATA(BraveVerticalTabStripRegionView)
END_METADATA
