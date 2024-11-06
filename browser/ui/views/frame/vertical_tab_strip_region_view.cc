/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/frame/vertical_tab_strip_region_view.h"

#include <algorithm>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

#include "base/functional/bind.h"
#include "brave/app/vector_icons/vector_icons.h"
#include "brave/browser/ui/brave_browser.h"
#include "brave/browser/ui/color/brave_color_id.h"
#include "brave/browser/ui/tabs/brave_tab_prefs.h"
#include "brave/browser/ui/views/frame/brave_contents_view_util.h"
#include "brave/browser/ui/views/tabs/brave_new_tab_button.h"
#include "brave/browser/ui/views/tabs/brave_tab_search_button.h"
#include "brave/browser/ui/views/tabs/brave_tab_strip_layout_helper.h"
#include "brave/browser/ui/views/tabs/vertical_tab_utils.h"
#include "brave/components/constants/pref_names.h"
#include "brave/components/vector_icons/vector_icons.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/themes/theme_properties.h"
#include "chrome/browser/ui/browser_list.h"
#include "chrome/browser/ui/color/chrome_color_id.h"
#include "chrome/browser/ui/exclusive_access/exclusive_access_manager.h"
#include "chrome/browser/ui/exclusive_access/fullscreen_controller.h"
#include "chrome/browser/ui/frame/window_frame_util.h"
#include "chrome/browser/ui/tabs/features.h"
#include "chrome/browser/ui/tabs/tab_style.h"
#include "chrome/browser/ui/views/chrome_layout_provider.h"
#include "chrome/browser/ui/views/frame/browser_non_client_frame_view.h"
#include "chrome/browser/ui/views/tabs/tab_strip_scroll_container.h"
#include "chrome/browser/ui/views/toolbar/toolbar_button.h"
#include "chrome/browser/ui/views/toolbar/toolbar_ink_drop_util.h"
#include "chrome/common/pref_names.h"
#include "chrome/grit/generated_resources.h"
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

#if !BUILDFLAG(IS_MAC)
#include "chrome/app/chrome_command_ids.h"
#endif

namespace {

constexpr int kHeaderInset = tabs::kMarginForVerticalTabContainers;

// Use toolbar button's ink drop effect.
class ToggleButton : public ToolbarButton {
  METADATA_HEADER(ToggleButton, ToolbarButton)
 public:
  ToggleButton(PressedCallback callback,
               VerticalTabStripRegionView* region_view)
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
                   VerticalTabStripRegionView::State::kExpanded);
  }

  void StateChanged(ButtonState old_state) override {
    ToolbarButton::StateChanged(old_state);

    if (GetState() == views::Button::STATE_NORMAL) {
      // Double check highlight state after changing state to normal. Dragging
      // the button can make the highlight effect hidden.
      // https://github.com/brave/brave-browser/issues/31421
      SetHighlighted(region_view_->state() ==
                     VerticalTabStripRegionView::State::kExpanded);
    }
  }

  std::u16string GetTooltipText(const gfx::Point& p) const override {
    if (region_view_->state() == VerticalTabStripRegionView::State::kExpanded) {
      return l10n_util::GetStringUTF16(IDS_VERTICAL_TABS_MINIMIZE);
    }

    // When it's minimized or floating.
    return l10n_util::GetStringUTF16(IDS_VERTICAL_TABS_EXPAND);
  }

  constexpr static int GetIconWidth() { return tabs::kVerticalTabHeight; }

 private:
  raw_ref<VerticalTabStripRegionView> region_view_;
};

BEGIN_METADATA(ToggleButton)
END_METADATA

class VerticalTabSearchButton : public BraveTabSearchButton {
  METADATA_HEADER(VerticalTabSearchButton, BraveTabSearchButton)
 public:
  VerticalTabSearchButton(VerticalTabStripRegionView* region_view,
                          TabStripController* tab_strip_controller,
                          BrowserWindowInterface* browser_window_interface,
                          Edge fixed_flat_edge,
                          Edge animated_flat_edge)
      : BraveTabSearchButton(tab_strip_controller,
                             browser_window_interface,
                             fixed_flat_edge,
                             animated_flat_edge) {
    SetPreferredSize(
        gfx::Size{ToggleButton::GetIconWidth(), ToggleButton::GetIconWidth()});
    SetTooltipText(l10n_util::GetStringUTF16(IDS_TOOLTIP_TAB_SEARCH));
    SetAccessibleName(l10n_util::GetStringUTF16(IDS_ACCNAME_TAB_SEARCH));
    SetBorder(nullptr);

    vertical_tab_on_right_.Init(
        brave_tabs::kVerticalTabsOnRight,
        region_view->browser()->profile()->GetPrefs(),
        base::BindRepeating(&VerticalTabSearchButton::UpdateBubbleArrow,
                            base::Unretained(this)));
    UpdateBubbleArrow();
  }

  ~VerticalTabSearchButton() override = default;

  // BraveTabSearchButton:
  void UpdateColors() override {
    BraveTabSearchButton::UpdateColors();

    // Override images set from UpdateIcon().
    constexpr int kIconSize = 16;
    const ui::ImageModel icon_image_model = ui::ImageModel::FromVectorIcon(
        kLeoSearchIcon, GetForegroundColor(), kIconSize);
    SetImageModel(views::Button::STATE_NORMAL, icon_image_model);
    SetImageModel(views::Button::STATE_HOVERED, icon_image_model);
    SetImageModel(views::Button::STATE_PRESSED, icon_image_model);
    SetBackground(nullptr);
  }

  void OnThemeChanged() override {
    BraveTabSearchButton::OnThemeChanged();
    ConfigureInkDropForToolbar(this);
  }

  ui::ColorId GetForegroundColor() override {
    if (views::InkDrop::Get(this)->GetInkDrop()->GetTargetInkDropState() ==
        views::InkDropState::ACTIVATED) {
      return kColorToolbarButtonActivated;
    }
    return kColorToolbarButtonIcon;
  }

  void UpdateInkDrop() override {
    // Do nothing as we don't need to change ink drop configs at this time.
  }

  int GetCornerRadius() const override {
    // As this button uses toolbar button's style, use toolbar's radius also.
    return ChromeLayoutProvider::Get()->GetCornerRadiusMetric(
        views::Emphasis::kMaximum, GetContentsBounds().size());
  }

  void StateChanged(ButtonState old_state) override {
    BraveTabSearchButton::StateChanged(old_state);
    UpdateColors();
  }

 private:
  void UpdateBubbleArrow() {
    if (*vertical_tab_on_right_) {
      SetBubbleArrow(views::BubbleBorder::RIGHT_TOP);
    } else {
      SetBubbleArrow(views::BubbleBorder::LEFT_TOP);
    }
  }

  BooleanPrefMember vertical_tab_on_right_;
};

BEGIN_METADATA(VerticalTabSearchButton)
END_METADATA

class VerticalTabNewTabButton : public BraveNewTabButton {
  METADATA_HEADER(VerticalTabNewTabButton, BraveNewTabButton)
 public:
  static constexpr int kHeight = 50;

  VerticalTabNewTabButton(TabStrip* tab_strip,
                          PressedCallback callback,
                          const std::u16string& shortcut_text,
                          VerticalTabStripRegionView* region_view)
      : BraveNewTabButton(tab_strip, std::move(callback)),
        region_view_(region_view) {
    // We're going to use flex layout for children of this class. Other children
    // from base classes should be handled out of flex layout.
    for (views::View* child : children()) {
      child->SetProperty(views::kViewIgnoredByLayoutKey, true);
    }

    SetNotifyEnterExitOnChild(true);

    SetLayoutManager(std::make_unique<views::FlexLayout>())
        ->SetOrientation(views::LayoutOrientation::kHorizontal)
        .SetCrossAxisAlignment(views::LayoutAlignment::kStretch);

    plus_icon_ = AddChildView(std::make_unique<views::ImageView>());
    plus_icon_->SetPreferredSize(
        gfx::Size(tabs::kVerticalTabMinWidth, kHeight));
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
    text_->SetProperty(views::kFlexBehaviorKey,
                       views::FlexSpecification(
                           views::MinimumFlexSizeRule::kPreferredSnapToZero,
                           views::MaximumFlexSizeRule::kUnbounded)
                           .WithOrder(3)
                           .WithWeight(0));

    constexpr int kFontSize = 12;
    const auto text_font = text_->font_list();
    text_->SetFontList(
        text_font.DeriveWithSizeDelta(kFontSize - text_font.GetFontSize()));

    shortcut_text_ = AddChildView(std::make_unique<views::Label>());
    shortcut_text_->SetHorizontalAlignment(
        gfx::HorizontalAlignment::ALIGN_RIGHT);
    shortcut_text_->SetVerticalAlignment(gfx::VerticalAlignment::ALIGN_MIDDLE);
    const auto shortcut_font = shortcut_text_->font_list();
    shortcut_text_->SetFontList(shortcut_font.DeriveWithSizeDelta(
        kFontSize - shortcut_font.GetFontSize()));
    shortcut_text_->SetProperty(
        views::kMarginsKey,
        gfx::Insets::VH(0, tabs::kMarginForVerticalTabContainers));
    shortcut_text_->SetProperty(
        views::kFlexBehaviorKey,
        views::FlexSpecification(
            views::MinimumFlexSizeRule::kPreferredSnapToZero,
            views::MaximumFlexSizeRule::kPreferred)
            .WithOrder(2));

    SetTooltipText(l10n_util::GetStringUTF16(IDS_TOOLTIP_NEW_TAB));
    SetAccessibleName(l10n_util::GetStringUTF16(IDS_ACCNAME_NEWTAB));
    SetShortcutText(shortcut_text);
  }

  ~VerticalTabNewTabButton() override = default;

  // BraveNewTabButton:
  SkPath GetBorderPath(const gfx::Point& origin,
                       bool extend_to_top) const override {
    auto contents_bounds = GetContentsBounds();
    SkPath path;
    const auto* widget = GetWidget();
    if (widget) {
      const float radius = GetCornerRadius();
      const gfx::Rect path_rect(origin.x(), origin.y(), contents_bounds.width(),
                                contents_bounds.height());
      path.addRoundRect(RectToSkRect(path_rect), radius, radius);
      path.close();
    }
    return path;
  }

  void PaintIcon(gfx::Canvas* canvas) override {
    // Revert back the offset set by NewTabButton::PaintButtonContents(), which
    // is the caller of this method.
    gfx::ScopedCanvas scoped_canvas(canvas);
    canvas->Translate(-GetContentsBounds().OffsetFromOrigin());

    // Bypass '+' painting as we have a |plus_icon_| for that.
    ImageButton::PaintButtonContents(canvas);
  }

  gfx::Insets GetInsets() const override {
    return gfx::Insets(tabs::kMarginForVerticalTabContainers);
  }

  void OnPaintFill(gfx::Canvas* canvas) const override {
    auto* cp = GetColorProvider();
    CHECK(cp);

    // Override fill color
    {
      gfx::ScopedCanvas scoped_canvas_for_scaling(canvas);
      canvas->UndoDeviceScaleFactor();
      cc::PaintFlags flags;
      flags.setAntiAlias(true);
      flags.setColor(cp->GetColor(kColorToolbar));
      canvas->DrawPath(GetBorderPath(gfx::Point(), false), flags);
    }

    // Draw split line on the top.
    // Revert back the offset set by NewTabButton::PaintButtonContents(), which
    // is the caller of this method.
    gfx::ScopedCanvas scoped_canvas_for_translating(canvas);
    canvas->Translate(-GetContentsBounds().OffsetFromOrigin());

    gfx::Rect separator_bounds = GetLocalBounds();
    separator_bounds.set_height(1);
    cc::PaintFlags flags;
    flags.setStyle(cc::PaintFlags::kFill_Style);
    flags.setColor(cp->GetColor(kColorToolbar));
    canvas->DrawRect(gfx::RectF(separator_bounds), flags);
  }

  void OnThemeChanged() override {
    BraveNewTabButton::OnThemeChanged();

    CHECK(text_ && shortcut_text_);

    auto* cp = GetColorProvider();
    CHECK(cp);

    plus_icon_->SchedulePaint();
    text_->SetEnabledColor(cp->GetColor(kColorBraveVerticalTabNTBTextColor));
    shortcut_text_->SetEnabledColor(
        cp->GetColor(kColorBraveVerticalTabNTBShortcutTextColor));
  }

  void Layout(PassKey) override {
    LayoutSuperclass<BraveNewTabButton>(this);

    // FlexLayout could set the ink drop container invisible.
    if (!ink_drop_container()->GetVisible()) {
      ink_drop_container()->SetVisible(true);
    }
  }

  gfx::Size CalculatePreferredSize(
      const views::SizeBounds& available_size) const override {
    auto size = BraveNewTabButton::CalculatePreferredSize(available_size);
    if (tabs::utils::ShouldShowVerticalTabs(tab_strip()->GetBrowser())) {
      size.set_height(kHeight);
    }
    return size;
  }

 private:
  void SetShortcutText(const std::u16string& text) {
    CHECK(shortcut_text_);
    shortcut_text_->SetText(text);
  }

  raw_ptr<VerticalTabStripRegionView> region_view_ = nullptr;
  raw_ptr<views::ImageView> plus_icon_ = nullptr;
  raw_ptr<views::Label> text_ = nullptr;
  raw_ptr<views::Label> shortcut_text_ = nullptr;
};

BEGIN_METADATA(VerticalTabNewTabButton)
END_METADATA

class ResettableResizeArea : public views::ResizeArea {
  METADATA_HEADER(ResettableResizeArea, views::ResizeArea)
 public:
  explicit ResettableResizeArea(VerticalTabStripRegionView* region_view)
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
  raw_ptr<VerticalTabStripRegionView> region_view_;
};

BEGIN_METADATA(ResettableResizeArea)
END_METADATA

}  // namespace

class VerticalTabStripScrollContentsView : public views::View {
  METADATA_HEADER(VerticalTabStripScrollContentsView, views::View)
 public:
  VerticalTabStripScrollContentsView(VerticalTabStripRegionView* container,
                                     TabStrip* tab_strip)
      : container_(container), tab_strip_(tab_strip) {
    SetLayoutManager(std::make_unique<views::FillLayout>());
  }
  ~VerticalTabStripScrollContentsView() override = default;

  // views::View:
  void ChildPreferredSizeChanged(views::View* child) override {
    if (base::FeatureList::IsEnabled(tabs::kScrollableTabStrip)) {
      return;
    }

    if (in_preferred_size_changed_) {
      return;
    }

    // Prevent reentrance caused by container_->Layout()
    base::AutoReset<bool> in_preferred_size_change(&in_preferred_size_changed_,
                                                   true);
    container_->set_layout_dirty({});
    container_->DeprecatedLayoutImmediately();
  }

  void OnPaintBackground(gfx::Canvas* canvas) override {
    canvas->DrawColor(GetColorProvider()->GetColor(kColorToolbar));
  }

 private:
  raw_ptr<VerticalTabStripRegionView> container_ = nullptr;
  raw_ptr<TabStrip> tab_strip_ = nullptr;

  bool in_preferred_size_changed_ = false;
};

BEGIN_METADATA(VerticalTabStripScrollContentsView)
END_METADATA

class VerticalTabStripRegionView::HeaderView : public views::View {
  METADATA_HEADER(HeaderView, views::View)
 public:
  HeaderView(views::Button::PressedCallback toggle_callback,
             VerticalTabStripRegionView* region_view,
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

    // We layout the search button at the end, because there's no
    // way to change its bubble arrow from TOP_RIGHT at the moment.
    tab_search_button_ = AddChildView(std::make_unique<VerticalTabSearchButton>(
        region_view, region_view->tab_strip()->controller(),
        browser_window_interface, Edge::kNone, Edge::kNone));
    UpdateTabSearchButtonVisibility();

    vertical_tab_on_right_.Init(
        brave_tabs::kVerticalTabsOnRight,
        region_view_->browser()->profile()->GetPrefs(),
        base::BindRepeating(&HeaderView::OnVerticalTabPositionChanged,
                            base::Unretained(this)));
    OnVerticalTabPositionChanged();
  }
  ~HeaderView() override = default;

  BraveTabSearchButton* tab_search_button() { return tab_search_button_; }

  void UpdateTabSearchButtonVisibility() {
    tab_search_button_->SetVisible(
        tab_search_button_->GetPreferredSize().width() +
            toggle_button_->GetPreferredSize().width() <=
        width());
    if (!tab_search_button_->GetVisible()) {
      // When it's not visible, move tab search button. Otherwise,
      // TabSearchBubble will be anchored to wrong position as LayoutManager
      // ignores invisible views.
      tab_search_button_->SetX(width() - tab_search_button_->width());
    }
  }

  ToggleButton* toggle_button() { return toggle_button_; }

  // views::View:
  void OnThemeChanged() override {
    View::OnThemeChanged();

    SetBackground(views::CreateSolidBackground(
        GetColorProvider()->GetColor(kColorToolbar)));
  }

  void OnBoundsChanged(const gfx::Rect& previous_bounds) override {
    View::OnBoundsChanged(previous_bounds);
    UpdateTabSearchButtonVisibility();
  }

 private:
  void OnVerticalTabPositionChanged() {
    std::vector<views::View*> new_children = {
        toggle_button_.get(), spacer_.get(), tab_search_button_.get()};
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

    base::ranges::for_each(new_children, [&](auto* v) { AddChildView(v); });
    layout_->SetFlexForView(spacer_,
                            1 /* resize |spacer| to fill the rest of space */);
  }

  raw_ptr<views::BoxLayout> layout_ = nullptr;
  raw_ptr<VerticalTabStripRegionView> region_view_ = nullptr;
  raw_ptr<const TabStrip> tab_strip_ = nullptr;
  raw_ptr<ToggleButton> toggle_button_ = nullptr;
  raw_ptr<views::View> spacer_ = nullptr;
  raw_ptr<BraveTabSearchButton> tab_search_button_ = nullptr;
  BooleanPrefMember vertical_tab_on_right_;
};

using HeaderView = VerticalTabStripRegionView::HeaderView;
BEGIN_METADATA(HeaderView)
END_METADATA

// Double checks mouse hovered state. When there's border around the region view
// or window resizable area the mouse enter/exit event might not be correct.
// Thus, observes mouse events that passes the window.
class VerticalTabStripRegionView::MouseWatcher : public ui::EventObserver {
 public:
  explicit MouseWatcher(VerticalTabStripRegionView* region_view)
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
  raw_ptr<VerticalTabStripRegionView> region_view_;
  std::unique_ptr<views::EventMonitor> event_monitor_;
};

VerticalTabStripRegionView::VerticalTabStripRegionView(
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
      base::BindRepeating(
          [](VerticalTabStripRegionView* container, const ui::Event& event) {
            // Note that Calling SetValue() doesn't trigger
            // OnCollapsedPrefChanged() for this view.
            if (container->state_ == State::kExpanded) {
              container->collapsed_pref_.SetValue(true);
              container->SetState(State::kCollapsed);
            } else {
              container->collapsed_pref_.SetValue(false);
              container->SetState(State::kExpanded);
            }
          },
          this),
      this, browser_));
  contents_view_ =
      AddChildView(std::make_unique<VerticalTabStripScrollContentsView>(
          this, original_region_view_->tab_strip_));
  header_view_->toggle_button()->SetHighlighted(state_ == State::kExpanded);

  new_tab_button_ = AddChildView(std::make_unique<VerticalTabNewTabButton>(
      original_region_view_->tab_strip_,
      base::BindRepeating(&TabStrip::NewTabButtonPressed,
                          base::Unretained(original_region_view_->tab_strip_)),
      GetShortcutTextForNewTabButton(browser_view), this));

  resize_area_ = AddChildView(std::make_unique<ResettableResizeArea>(this));

  auto* prefs = browser_->profile()->GetPrefs();

  sidebar_side_.Init(
      prefs::kSidePanelHorizontalAlignment, prefs,
      base::BindRepeating(&VerticalTabStripRegionView::OnBrowserPanelsMoved,
                          base::Unretained(this)));

  expanded_width_pref_.Init(
      brave_tabs::kVerticalTabsExpandedWidth, prefs,
      base::BindRepeating(
          &VerticalTabStripRegionView::OnExpandedWidthPrefChanged,
          base::Unretained(this)));
  OnExpandedWidthPrefChanged();

  show_vertical_tabs_.Init(
      brave_tabs::kVerticalTabsEnabled, prefs,
      base::BindRepeating(
          &VerticalTabStripRegionView::OnShowVerticalTabsPrefChanged,
          base::Unretained(this)));
  UpdateLayout();

  collapsed_pref_.Init(
      brave_tabs::kVerticalTabsCollapsed, prefs,
      base::BindRepeating(&VerticalTabStripRegionView::OnCollapsedPrefChanged,
                          base::Unretained(this)));
  OnCollapsedPrefChanged();

  expanded_state_per_window_pref_.Init(
      brave_tabs::kVerticalTabsExpandedStatePerWindow, prefs,
      base::BindRepeating(
          &VerticalTabStripRegionView::OnExpandedStatePerWindowPrefChanged,
          base::Unretained(this)));

  floating_mode_pref_.Init(
      brave_tabs::kVerticalTabsFloatingEnabled, prefs,
      base::BindRepeating(
          &VerticalTabStripRegionView::OnFloatingModePrefChanged,
          base::Unretained(this)));

#if BUILDFLAG(IS_MAC)
  show_toolbar_on_fullscreen_pref_.Init(
      prefs::kShowFullscreenToolbar, prefs,
      base::BindRepeating(&VerticalTabStripRegionView::OnFullscreenStateChanged,
                          base::Unretained(this)));
#endif

  vertical_tab_on_right_.Init(
      brave_tabs::kVerticalTabsOnRight, browser()->profile()->GetPrefs(),
      base::BindRepeating(&VerticalTabStripRegionView::OnBrowserPanelsMoved,
                          base::Unretained(this)));

  widget_observation_.Observe(browser_view->GetWidget());

  // At this point, Browser hasn't finished its initialization. In order to
  // access some of its member, we should observe BrowserList.
  DCHECK(base::ranges::find(*BrowserList::GetInstance(),
                            browser_view->browser()) ==
         BrowserList::GetInstance()->end())
      << "Browser shouldn't be added at this point.";
  BrowserList::AddObserver(this);

  // Note: This should happen after all the PrefMembers have been initialized.
  OnFloatingModePrefChanged();

  set_context_menu_controller(this);
}

VerticalTabStripRegionView::~VerticalTabStripRegionView() {
  // We need to move tab strip region to its original parent to avoid crash
  // during drag and drop session.
  UpdateLayout(true);
  DCHECK(fullscreen_observation_.IsObserving())
      << "We didn't start to observe FullscreenController from BrowserList's "
         "callback";
}

void VerticalTabStripRegionView::OnWidgetActivationChanged(
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

void VerticalTabStripRegionView::OnWidgetDestroying(views::Widget* widget) {
  widget_observation_.Reset();
}

void VerticalTabStripRegionView::OnFullscreenStateChanged() {
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

void VerticalTabStripRegionView::OnBrowserAdded(Browser* browser) {
  if (browser != browser_) {
    return;
  }

  auto* fullscreen_controller = GetFullscreenController();
  DCHECK(fullscreen_controller);
  fullscreen_observation_.Observe(fullscreen_controller);

  BrowserList::RemoveObserver(this);
}

FullscreenController* VerticalTabStripRegionView::GetFullscreenController()
    const {
  auto* exclusive_access_manager = browser_->exclusive_access_manager();
  if (!exclusive_access_manager) {
    return nullptr;
  }

  return exclusive_access_manager->fullscreen_controller();
}

bool VerticalTabStripRegionView::IsTabFullscreen() const {
  const auto* fullscreen_controller = GetFullscreenController();
  return fullscreen_controller &&
         fullscreen_controller->IsWindowFullscreenForTabOrPending();
}

bool VerticalTabStripRegionView::IsBrowserFullscren() const {
  const auto* fullscreen_controller = GetFullscreenController();
  return fullscreen_controller &&
         fullscreen_controller->IsFullscreenForBrowser();
}

bool VerticalTabStripRegionView::ShouldShowVerticalTabsInBrowserFullscreen()
    const {
#if BUILDFLAG(IS_MAC)
  // Refer to "Always show toolbar in Fullscreen" pref in the app menu
  return show_toolbar_on_fullscreen_pref_.GetValue();
#else
  return false;
#endif
}

void VerticalTabStripRegionView::SetState(State state) {
  if (state_ == state) {
    return;
  }

  mouse_enter_timer_.Stop();

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
      &VerticalTabStripRegionView::GetAvailableWidthForTabContainer,
      base::Unretained(this)));
  tab_strip->tab_container_->InvalidateIdealBounds();
  tab_strip->tab_container_->CompleteAnimationAndLayout();

  if (gfx::Animation::ShouldRenderRichAnimation()) {
    state_ == State::kCollapsed ? width_animation_.Hide()
                                : width_animation_.Show();
  }

  if (!GetVisible() && state_ != State::kCollapsed) {
    // This means vertical tab strip is expanded temporarily in browser
    // fullscreen mode.
    SetVisible(true);
  }

  PreferredSizeChanged();
  UpdateBorder();
}

void VerticalTabStripRegionView::SetExpandedWidth(int dest_width) {
  if (expanded_width_ == dest_width) {
    return;
  }

  expanded_width_ = dest_width;

  if (expanded_width_ != *expanded_width_pref_) {
    expanded_width_pref_.SetValue(expanded_width_);
  }

  PreferredSizeChanged();
}

void VerticalTabStripRegionView::UpdateStateAfterDragAndDropFinished(
    State original_state) {
  DCHECK_NE(original_state, State::kExpanded)
      << "As per ExpandTabStripForDragging(), this shouldn't happen";

  if (tabs::utils::IsFloatingVerticalTabsEnabled(browser_) &&
      IsMouseHovered()) {
    SetState(State::kFloating);
    return;
  }

  SetState(State::kCollapsed);
}

VerticalTabStripRegionView::ScopedStateResetter
VerticalTabStripRegionView::ExpandTabStripForDragging() {
  if (state_ == State::kExpanded) {
    return {};
  }

  auto resetter = std::make_unique<base::ScopedClosureRunner>(base::BindOnce(
      &VerticalTabStripRegionView::UpdateStateAfterDragAndDropFinished,
      weak_factory_.GetWeakPtr(), state_));

  SetState(State::kExpanded);
  // In this case, we dont' wait for the widget bounds to be changed so that
  // tab drag controller can layout tabs properly.
  SetSize(GetPreferredSize());

  return resetter;
}

gfx::Vector2d VerticalTabStripRegionView::GetOffsetForDraggedTab() const {
  return {0, header_view_->GetPreferredSize().height()};
}

int VerticalTabStripRegionView::GetAvailableWidthForTabContainer() {
  DCHECK(tabs::utils::ShouldShowVerticalTabs(browser_));
  return GetPreferredWidthForState(state_, /*include_border=*/false,
                                   /*ignore_animation=*/false);
}

gfx::Size VerticalTabStripRegionView::CalculatePreferredSize(
    const views::SizeBounds& available_size) const {
  return GetPreferredSizeForState(state_, /*include_border=*/true,
                                  /*ignore_animation=*/false);
}

gfx::Size VerticalTabStripRegionView::GetMinimumSize() const {
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

void VerticalTabStripRegionView::Layout(PassKey) {
  if (!layout_dirty_ && last_size_ == size()) {
    return;
  }

  layout_dirty_ = false;
  last_size_ = size();

  // As we have to update ScrollView's viewport size and its contents size,
  // laying out children manually will be more handy.

  // 1. New tab should be fixed at the bottom of container.
  const auto contents_bounds = GetContentsBounds();
  new_tab_button_->SetSize(
      {contents_bounds.width(), new_tab_button_->GetPreferredSize().height()});
  new_tab_button_->SetPosition(
      {contents_bounds.x(),
       contents_bounds.bottom() - new_tab_button_->height()});

  const gfx::Size header_size{contents_bounds.width(),
                              tabs::kVerticalTabHeight + kHeaderInset * 2};
  header_view_->SetPosition(contents_bounds.origin());
  header_view_->SetSize(header_size);

  contents_view_->SetSize(
      {contents_bounds.width(), contents_bounds.height() -
                                    new_tab_button_->height() -
                                    header_view_->height()});
  contents_view_->SetPosition({contents_bounds.origin().x(),
                               header_view_->y() + header_view_->height()});
  UpdateOriginalTabSearchButtonVisibility();

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

void VerticalTabStripRegionView::OnShowVerticalTabsPrefChanged() {
  UpdateLayout(/* in_destruction= */ false);

  if (!tabs::utils::ShouldShowVerticalTabs(browser_) &&
      state_ == State::kFloating) {
    mouse_enter_timer_.Stop();
    SetState(State::kCollapsed);
  }

  UpdateBorder();
}

void VerticalTabStripRegionView::OnBrowserPanelsMoved() {
  UpdateBorder();
  PreferredSizeChanged();
}

void VerticalTabStripRegionView::UpdateLayout(bool in_destruction) {
  layout_dirty_ = true;
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

void VerticalTabStripRegionView::OnThemeChanged() {
  View::OnThemeChanged();

  auto* cp = GetColorProvider();
  CHECK(cp);

  const auto background_color = cp->GetColor(kColorToolbar);
  SetBackground(views::CreateSolidBackground(background_color));
  UpdateBorder();

  new_tab_button_->FrameColorsChanged();
}

void VerticalTabStripRegionView::OnMouseExited(const ui::MouseEvent& event) {
  OnMouseExited();
}

void VerticalTabStripRegionView::OnMouseExited() {
  DCHECK(GetWidget())
      << "As widget is the event sink, this is expected to be true.";
  if (GetWidget()->GetRootView()->IsMouseHovered() && !mouse_events_for_test_) {
    // On Windows, when mouse moves into the area which intersects with web
    // view, OnMouseExited() is invoked even mouse is on this view.
    return;
  }

  mouse_enter_timer_.Stop();
  if (state_ == State::kFloating) {
    SetState(State::kCollapsed);
    if (IsFloatingEnabledForBrowserFullscreen()) {
      SetVisible(false);
    }
  }
}

void VerticalTabStripRegionView::OnMouseEntered(const ui::MouseEvent& event) {
  OnMouseEntered();
}

void VerticalTabStripRegionView::OnMouseEntered() {
  if (!IsFloatingVerticalTabsEnabled()) {
    return;
  }

  // During tab dragging, this could be already expanded.
  if (state_ == State::kExpanded) {
    return;
  }

  ScheduleFloatingModeTimer();
}

void VerticalTabStripRegionView::OnMousePressedInTree() {
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

void VerticalTabStripRegionView::OnBoundsChanged(
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
  if (auto width = GetContentsBounds().width();
      width && !IsBrowserFullscren() && GetWidget()->IsVisible()) {
    CHECK_GE(
        width,
        tabs::kVerticalTabMinWidth + tabs::kMarginForVerticalTabContainers * 2 -
            BraveContentsViewUtil::GetRoundedCornersWebViewMargin(browser_));
  }
#endif
}

void VerticalTabStripRegionView::PreferredSizeChanged() {
  layout_dirty_ = true;
  views::View::PreferredSizeChanged();
}

void VerticalTabStripRegionView::AddedToWidget() {
  View::AddedToWidget();
  mouse_watcher_ = std::make_unique<MouseWatcher>(this);
}

void VerticalTabStripRegionView::OnResize(int resize_amount,
                                          bool done_resizing) {
  CHECK_NE(state_, State::kCollapsed);

  gfx::Rect bounds_in_screen = GetLocalBounds();
  views::View::ConvertRectToScreen(this, &bounds_in_screen);

  auto cursor_position =
      display::Screen::GetScreen()->GetCursorScreenPoint().x();
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
  dest_width = std::clamp(dest_width, tab_style_->GetPinnedWidth() * 3,
                          tab_style_->GetStandardWidth() * 2);
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

void VerticalTabStripRegionView::AnimationProgressed(
    const gfx::Animation* animation) {
  PreferredSizeChanged();
}

void VerticalTabStripRegionView::AnimationEnded(
    const gfx::Animation* animation) {
  PreferredSizeChanged();
}

void VerticalTabStripRegionView::UpdateNewTabButtonVisibility() {
  const bool is_vertical_tabs = tabs::utils::ShouldShowVerticalTabs(browser_);
  auto* original_ntb = original_region_view_->new_tab_button();
  original_ntb->SetVisible(!is_vertical_tabs);
  new_tab_button_->SetVisible(is_vertical_tabs);
}

TabSearchBubbleHost* VerticalTabStripRegionView::GetTabSearchBubbleHost() {
  return header_view_->tab_search_button()->tab_search_bubble_host();
}

int VerticalTabStripRegionView::GetTabStripViewportHeight() const {
  // Don't depend on |contents_view_|'s current height. It could be bigger than
  // the actual viewport height.
  return GetContentsBounds().height() - header_view_->height() -
         new_tab_button_->height();
}

void VerticalTabStripRegionView::ResetExpandedWidth() {
  auto* prefs = browser_->profile()->GetPrefs();
  prefs->ClearPref(brave_tabs::kVerticalTabsExpandedWidth);

  PreferredSizeChanged();
}

void VerticalTabStripRegionView::UpdateOriginalTabSearchButtonVisibility() {
  const bool is_vertical_tabs = tabs::utils::ShouldShowVerticalTabs(browser_);
  const bool use_search_button =
      browser_->profile()->GetPrefs()->GetBoolean(kTabsSearchShow);
  if (auto* tab_search_container =
          original_region_view_->tab_search_container()) {
    if (auto* tab_search_button = tab_search_container->tab_search_button()) {
      tab_search_button->SetVisible(!is_vertical_tabs && use_search_button);
    }
  }
}

void VerticalTabStripRegionView::UpdateBorder() {
  auto show_visible_border = [&]() {
    // The color provider might not be available during initialization.
    if (!GetColorProvider()) {
      return false;
    }

    if (!BraveBrowser::ShouldUseBraveWebViewRoundedCorners(browser_)) {
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
}

void VerticalTabStripRegionView::OnCollapsedPrefChanged() {
  if (!expanded_state_per_window_pref_.GetPrefName().empty() &&
      *expanded_state_per_window_pref_) {
    // On creation(when expanded_state_per_window_pref_ is empty), we set the
    // default state based on the `collapsed_pref_` even if the
    // `expanded_state_per_window_pref_` is set.
    return;
  }

  SetState(collapsed_pref_.GetValue() ? State::kCollapsed : State::kExpanded);
}

void VerticalTabStripRegionView::OnFloatingModePrefChanged() {
  if (!tabs::utils::IsFloatingVerticalTabsEnabled(browser_)) {
    if (state_ == State::kFloating) {
      SetState(State::kCollapsed);
    }
    return;
  }

  if (IsMouseHovered()) {
    ScheduleFloatingModeTimer();
  }
}

void VerticalTabStripRegionView::OnExpandedStatePerWindowPrefChanged() {
  OnCollapsedPrefChanged();
  OnExpandedWidthPrefChanged();
}

void VerticalTabStripRegionView::OnExpandedWidthPrefChanged() {
  if (!expanded_state_per_window_pref_.GetPrefName().empty() &&
      *expanded_state_per_window_pref_) {
    // On creation(when expanded_state_per_window_pref_ is empty), we set the
    // default state based on the `expanded_width_pref_` even if the
    // `expanded_state_per_window_pref_` is set.
    return;
  }

  SetExpandedWidth(*expanded_width_pref_);
}

gfx::Size VerticalTabStripRegionView::GetPreferredSizeForState(
    State state,
    bool include_border,
    bool ignore_animation) const {
  if (!tabs::utils::ShouldShowVerticalTabs(browser_)) {
    return {};
  }

  if (IsTabFullscreen()) {
    return {};
  }

  if (IsFloatingEnabledForBrowserFullscreen() && state_ == State::kCollapsed) {
    // In this case, vertical tab strip should be invisible but show up when
    // mouse hovers.
    return {2, View::CalculatePreferredSize({}).height()};
  }

  return {GetPreferredWidthForState(state, include_border, ignore_animation),
          View::CalculatePreferredSize({}).height()};
}

int VerticalTabStripRegionView::GetPreferredWidthForState(
    State state,
    bool include_border,
    bool ignore_animation) const {
  auto calculate_expanded_width = [&]() {
    return *expanded_width_pref_ + (include_border ? GetInsets().width() : 0);
  };

  auto calculate_collapsed_width = [&]() {
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
VerticalTabStripRegionView::GetTabStripScrollContainer() {
  CHECK(base::FeatureList::IsEnabled(tabs::kScrollableTabStrip));
  auto* scroll_container = views::AsViewClass<TabStripScrollContainer>(
      original_region_view_->tab_strip_container_);
  CHECK(scroll_container)
      << "TabStripScrollContainer is used by upstream at this moment.";
  return scroll_container;
}

bool VerticalTabStripRegionView::IsFloatingVerticalTabsEnabled() const {
  return IsFloatingEnabledForBrowserFullscreen() ||
         tabs::utils::IsFloatingVerticalTabsEnabled(browser_);
}

bool VerticalTabStripRegionView::IsFloatingEnabledForBrowserFullscreen() const {
  return IsBrowserFullscren() && !ShouldShowVerticalTabsInBrowserFullscreen();
}

void VerticalTabStripRegionView::ScheduleFloatingModeTimer() {
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
    mouse_enter_timer_.Start(
        FROM_HERE, base::Milliseconds(400),
        base::BindOnce(&VerticalTabStripRegionView::SetState,
                       base::Unretained(this), State::kFloating));
  }
}

#if !BUILDFLAG(IS_MAC)
std::u16string VerticalTabStripRegionView::GetShortcutTextForNewTabButton(
    BrowserView* browser_view) {
  if (ui::Accelerator new_tab_accelerator;
      browser_view->GetAcceleratorForCommandId(IDC_NEW_TAB,
                                               &new_tab_accelerator)) {
    return new_tab_accelerator.GetShortcutText();
  }

  return {};
}
#endif

views::LabelButton& VerticalTabStripRegionView::GetToggleButtonForTesting() {
  return *header_view_->toggle_button();
}

bool VerticalTabStripRegionView::IsMenuShowing() const {
  return menu_runner_ && menu_runner_->IsRunning();
}

// Show context menu in unobscured area.
void VerticalTabStripRegionView::ShowContextMenuForViewImpl(
    views::View* source,
    const gfx::Point& p,
    ui::MenuSourceType source_type) {
  if (IsMenuShowing()) {
    return;
  }

  menu_runner_ = std::make_unique<views::MenuRunner>(
      browser_view_->frame()->GetSystemMenuModel(),
      views::MenuRunner::HAS_MNEMONICS | views::MenuRunner::CONTEXT_MENU,
      base::BindRepeating(&VerticalTabStripRegionView::OnMenuClosed,
                          base::Unretained(this)));
  menu_runner_->RunMenuAt(source->GetWidget(), nullptr,
                          gfx::Rect(p, gfx::Size(0, 0)),
                          views::MenuAnchorPosition::kTopLeft, source_type);
}

void VerticalTabStripRegionView::OnMenuClosed() {
  menu_runner_.reset();
}

BEGIN_METADATA(VerticalTabStripRegionView)
END_METADATA
