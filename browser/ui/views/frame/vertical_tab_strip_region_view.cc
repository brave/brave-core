/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/frame/vertical_tab_strip_region_view.h"

#include <algorithm>
#include <memory>
#include <utility>
#include <vector>

#include "brave/app/vector_icons/vector_icons.h"
#include "brave/browser/ui/brave_browser.h"
#include "brave/browser/ui/color/brave_color_id.h"
#include "brave/browser/ui/tabs/brave_tab_prefs.h"
#include "brave/browser/ui/views/tabs/brave_new_tab_button.h"
#include "brave/browser/ui/views/tabs/brave_tab_search_button.h"
#include "brave/browser/ui/views/tabs/brave_tab_strip_layout_helper.h"
#include "brave/browser/ui/views/tabs/vertical_tab_utils.h"
#include "brave/components/constants/pref_names.h"
#include "brave/components/vector_icons/vector_icons.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/themes/theme_properties.h"
#include "chrome/browser/ui/browser_list.h"
#include "chrome/browser/ui/exclusive_access/exclusive_access_manager.h"
#include "chrome/browser/ui/exclusive_access/fullscreen_controller.h"
#include "chrome/browser/ui/frame/window_frame_util.h"
#include "chrome/browser/ui/tabs/tab_style.h"
#include "chrome/browser/ui/views/chrome_layout_provider.h"
#include "chrome/browser/ui/views/frame/browser_non_client_frame_view.h"
#include "chrome/browser/ui/views/tabs/tab_strip_scroll_container.h"
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

// Inherits NewTabButton in order to synchronize ink drop effect with
// the search button. Unfortunately, we can't inherit BraveTabSearchButton
// as NotifyClick() is marked as 'final'.
class ToggleButton : public BraveNewTabButton {
 public:
  METADATA_HEADER(ToggleButton);

  ToggleButton(Button::PressedCallback callback,
               VerticalTabStripRegionView* region_view)
      : BraveNewTabButton(region_view->tab_strip(),
                          std::move(std::move(callback))),
        region_view_(region_view),
        tab_strip_(region_view_->tab_strip()) {
    SetPreferredSize(gfx::Size{GetIconWidth(), GetIconWidth()});
  }
  ~ToggleButton() override = default;

  constexpr static int GetIconWidth() { return tabs::kVerticalTabHeight; }

  // views::BraveNewTabButton:
  void OnThemeChanged() override {
    BraveNewTabButton::OnThemeChanged();

    // Resets the ink drop highlight color
    views::InkDrop::Get(this)->GetInkDrop()->HostViewThemeChanged();
    if (views::InkDrop::Get(this)->GetHighlighted()) {
      // Note that we're calling this to make InkDropHighlight visible even if
      // the state could already be transitioned to activated.
      views::InkDrop::Get(this)->GetInkDrop()->SnapToActivated();
    }

    // Resets icon.
    auto* cp = GetColorProvider();
    CHECK(cp);

    auto color = cp->GetColor(kColorBraveVerticalTabHeaderButtonColor);
    icon_ = gfx::CreateVectorIcon(kVerticalTabStripToggleButtonIcon, color);
  }

  void PaintButtonContents(gfx::Canvas* canvas) override {
    const gfx::Point origin =
        GetContentsBounds().CenterPoint() -
        gfx::Vector2d(icon_.width() / 2, icon_.height() / 2);
    canvas->DrawImageInt(icon_, origin.x(), origin.y());
  }

  gfx::Insets GetInsets() const override {
    // By pass BraveNewTabButton::GetInsets().
    return NewTabButton::GetInsets();
  }

  int GetCornerRadius() const override {
    return ChromeLayoutProvider::Get()->GetCornerRadiusMetric(
        views::Emphasis::kMaximum, GetPreferredSize());
  }

  std::u16string GetTooltipText(const gfx::Point& p) const override {
    if (region_view_->state() == VerticalTabStripRegionView::State::kExpanded) {
      return l10n_util::GetStringUTF16(IDS_VERTICAL_TABS_MINIMIZE);
    }

    // When it's minimized or floating.
    return l10n_util::GetStringUTF16(IDS_VERTICAL_TABS_EXPAND);
  }

#if DCHECK_IS_ON()
  void OnBoundsChanged(const gfx::Rect& previous_bounds) override {
    CHECK_EQ(width(), GetIconWidth());
    CHECK_EQ(height(), GetIconWidth());
  }
#endif

  void NotifyClick(const ui::Event& event) override {
    // Bypass NewTab::NotifyClick implementation in order keep ink drop state
    // ACTIVATED. As NewTabButton::NotifyClick animate ink drop state to
    // ActionTriggered after notifying this event, we shouldn't use it.
    // otherwise, ink drop state will be hidden.
    views::InkDrop::Get(this)->GetInkDrop()->AnimateToState(
        views::InkDropState::ACTION_TRIGGERED);
    ImageButton::NotifyClick(event);
  }

  void StateChanged(ButtonState old_state) override {
    BraveNewTabButton::StateChanged(old_state);

    if (GetState() == views::Button::STATE_NORMAL) {
      // Double check highlight state after changing state to normal. Dragging
      // the button can make the highlight effect hidden.
      // https://github.com/brave/brave-browser/issues/31421
      SetHighlighted(region_view_->state() ==
                     VerticalTabStripRegionView::State::kExpanded);
    }
  }

 private:
  raw_ptr<VerticalTabStripRegionView> region_view_ = nullptr;
  raw_ptr<const TabStrip> tab_strip_ = nullptr;

  gfx::ImageSkia icon_;
};

BEGIN_METADATA(ToggleButton, views::Button)
END_METADATA

// A custom scroll view to avoid crash on Mac
// TODO(sko) Remove this once the "sticky pinned tabs" is enabled by default.
// https://github.com/brave/brave-browser/issues/29935
class CustomScrollView : public views::ScrollView {
 public:
  METADATA_HEADER(CustomScrollView);

#if BUILDFLAG(IS_MAC)
  CustomScrollView()
      : views::ScrollView(views::ScrollView::ScrollWithLayers::kDisabled) {}
#else
  CustomScrollView() : views::ScrollView() {}
#endif
  ~CustomScrollView() override = default;

  // views::ScrollView:
  void OnScrollEvent(ui::ScrollEvent* event) override {
#if !BUILDFLAG(IS_MAC)
    views::ScrollView::OnScrollEvent(event);
#endif
  }
};

BEGIN_METADATA(CustomScrollView, views::ScrollView)
END_METADATA

class VerticalTabSearchButton : public BraveTabSearchButton {
 public:
  METADATA_HEADER(VerticalTabSearchButton);

  explicit VerticalTabSearchButton(TabStrip* tab_strip)
      : BraveTabSearchButton(tab_strip) {
    SetPreferredSize(
        gfx::Size{ToggleButton::GetIconWidth(), ToggleButton::GetIconWidth()});
    SetTooltipText(l10n_util::GetStringUTF16(IDS_TOOLTIP_TAB_SEARCH));
    SetAccessibleName(l10n_util::GetStringUTF16(IDS_ACCNAME_TAB_SEARCH));
    SetBubbleArrow(views::BubbleBorder::LEFT_TOP);
    SetBorder(nullptr);
  }

  ~VerticalTabSearchButton() override = default;

  // BraveTabSearchButton:
  void UpdateColors() override {
    BraveTabSearchButton::UpdateColors();

    // Override images set from UpdateIcon().
    SetImageModel(views::Button::STATE_NORMAL,
                  ui::ImageModel::FromVectorIcon(
                      kLeoSearchIcon, kColorBraveVerticalTabHeaderButtonColor,
                      /* icon_size= */ 16));
    SetImageModel(views::Button::STATE_HOVERED, ui::ImageModel());
    SetImageModel(views::Button::STATE_PRESSED, ui::ImageModel());
    SetBackground(nullptr);
  }
};

BEGIN_METADATA(VerticalTabSearchButton, BraveTabSearchButton)
END_METADATA

class VerticalTabNewTabButton : public BraveNewTabButton {
 public:
  METADATA_HEADER(VerticalTabNewTabButton);

  static constexpr int kHeight = 50;

  VerticalTabNewTabButton(TabStrip* tab_strip,
                          PressedCallback callback,
                          const std::u16string& shortcut_text,
                          VerticalTabStripRegionView* region_view)
      : BraveNewTabButton(tab_strip, std::move(callback)),
        region_view_(region_view) {
    // We're going to use flex layout for children of this class. Other children
    // from base classes should be handled out of flex layout.
    for (auto* child : children()) {
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
                       float scale,
                       bool extend_to_top) const override {
    auto contents_bounds = GetContentsBounds();
    const float radius = GetCornerRadius() * scale;
    SkPath path;
    const gfx::Rect path_rect(origin.x(), origin.y(),
                              contents_bounds.width() * scale,
                              contents_bounds.height() * scale);
    path.addRoundRect(RectToSkRect(path_rect), radius, radius);
    path.close();
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
      canvas->DrawPath(
          GetBorderPath(gfx::Point(), canvas->image_scale(), false), flags);
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
    flags.setColor(cp->GetColor(kColorBraveVerticalTabSeparator));
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

  void Layout() override {
    BraveNewTabButton::Layout();

    // FlexLayout could set the ink drop container invisible.
    if (!ink_drop_container()->GetVisible()) {
      ink_drop_container()->SetVisible(true);
    }
  }

  gfx::Size CalculatePreferredSize() const override {
    auto size = BraveNewTabButton::CalculatePreferredSize();
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

BEGIN_METADATA(VerticalTabNewTabButton, BraveNewTabButton)
END_METADATA

class ResettableResizeArea : public views::ResizeArea {
 public:
  METADATA_HEADER(ResettableResizeArea);

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

BEGIN_METADATA(ResettableResizeArea, ResizeArea)
END_METADATA

}  // namespace

class VerticalTabStripScrollContentsView : public views::View {
 public:
  METADATA_HEADER(VerticalTabStripScrollContentsView);

  VerticalTabStripScrollContentsView(VerticalTabStripRegionView* container,
                                     TabStrip* tab_strip)
      : container_(container), tab_strip_(tab_strip) {
    SetLayoutManager(std::make_unique<views::FillLayout>());
  }
  ~VerticalTabStripScrollContentsView() override = default;

  // views::View:
  void ChildPreferredSizeChanged(views::View* child) override {
    if (base::FeatureList::IsEnabled(features::kScrollableTabStrip)) {
      return;
    }

    if (in_preferred_size_changed_) {
      return;
    }

    // Prevent reentrance caused by container_->Layout()
    base::AutoReset<bool> in_preferred_size_change(&in_preferred_size_changed_,
                                                   true);
    container_->set_layout_dirty({});
    container_->Layout();
  }

  void OnPaintBackground(gfx::Canvas* canvas) override {
    canvas->DrawColor(GetColorProvider()->GetColor(kColorToolbar));
  }

 private:
  raw_ptr<VerticalTabStripRegionView> container_ = nullptr;
  raw_ptr<TabStrip> tab_strip_ = nullptr;

  bool in_preferred_size_changed_ = false;
};

BEGIN_METADATA(VerticalTabStripScrollContentsView, views::View)
END_METADATA

class VerticalTabStripRegionView::HeaderView : public views::View {
 public:
  METADATA_HEADER(HeaderView);

  HeaderView(views::Button::PressedCallback toggle_callback,
             VerticalTabStripRegionView* region_view)
      : region_view_(region_view), tab_strip_(region_view->tab_strip()) {
    SetBorder(views::CreateEmptyBorder(gfx::Insets(kHeaderInset)));

    layout_ = SetLayoutManager(std::make_unique<views::BoxLayout>(
        views::BoxLayout::Orientation::kHorizontal));
    layout_->set_cross_axis_alignment(
        views::BoxLayout::CrossAxisAlignment::kStretch);

    toggle_button_ = AddChildView(std::make_unique<ToggleButton>(
        std::move(toggle_callback), region_view));

    auto* spacer = AddChildView(std::make_unique<views::View>());
    layout_->SetFlexForView(spacer,
                            1 /* resize |spacer| to fill the rest of space */);

    // We layout the search button at the end, because there's no
    // way to change its bubble arrow from TOP_RIGHT at the moment.
    tab_search_button_ = AddChildView(
        std::make_unique<VerticalTabSearchButton>(region_view->tab_strip()));
    UpdateTabSearchButtonVisibility();
  }
  ~HeaderView() override = default;

  BraveTabSearchButton* tab_search_button() { return tab_search_button_; }

  void UpdateTabSearchButtonVisibility() {
    tab_search_button_->SetVisible(
        !WindowFrameUtil::IsWindowsTabSearchCaptionButtonEnabled(
            region_view_->browser()) &&
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
  raw_ptr<views::BoxLayout> layout_ = nullptr;
  raw_ptr<VerticalTabStripRegionView> region_view_ = nullptr;
  raw_ptr<const TabStrip> tab_strip_ = nullptr;
  raw_ptr<ToggleButton> toggle_button_ = nullptr;
  raw_ptr<BraveTabSearchButton> tab_search_button_ = nullptr;
};

BEGIN_METADATA(VerticalTabStripRegionView, HeaderView, views::View)
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
            {ui::ET_MOUSE_PRESSED, ui::ET_MOUSE_ENTERED,
             ui::ET_MOUSE_EXITED})) {}

  // ui::EventObserver:
  void OnEvent(const ui::Event& event) override {
    switch (event.type()) {
      case ui::ET_MOUSE_ENTERED:
        region_view_->OnMouseEntered();
        break;
      case ui::ET_MOUSE_PRESSED:
        region_view_->OnMousePressedInTree();
        break;
      case ui::ET_MOUSE_EXITED:
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
      this));
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

  expanded_width_.Init(
      brave_tabs::kVerticalTabsExpandedWidth, prefs,
      base::BindRepeating(&VerticalTabStripRegionView::PreferredSizeChanged,
                          base::Unretained(this)));

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

  floating_mode_pref_.Init(
      brave_tabs::kVerticalTabsFloatingEnabled, prefs,
      base::BindRepeating(
          &VerticalTabStripRegionView::OnFloatingModePrefChanged,
          base::Unretained(this)));
  OnFloatingModePrefChanged();

#if BUILDFLAG(IS_MAC)
  show_toolbar_on_fullscreen_pref_.Init(
      prefs::kShowFullscreenToolbar, prefs,
      base::BindRepeating(&VerticalTabStripRegionView::OnFullscreenStateChanged,
                          base::Unretained(this)));
#endif

  widget_observation_.Observe(browser_view->GetWidget());

  // At this point, Browser hasn't finished its initialization. In order to
  // access some of its member, we should observe BrowserList.
  DCHECK(base::ranges::find(*BrowserList::GetInstance(),
                            browser_view->browser()) ==
         BrowserList::GetInstance()->end())
      << "Browser shouldn't be added at this point.";
  BrowserList::AddObserver(this);
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

gfx::Size VerticalTabStripRegionView::CalculatePreferredSize() const {
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

void VerticalTabStripRegionView::Layout() {
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

  // Put resize area on the right side, overlapped with contents.
  constexpr int kResizeAreaWidth = 4;
  resize_area_->SetBounds(width() - kResizeAreaWidth, contents_bounds.y(),
                          kResizeAreaWidth, contents_bounds.height());
}

void VerticalTabStripRegionView::OnShowVerticalTabsPrefChanged() {
  UpdateLayout(/* in_destruction= */ false);

  if (!tabs::utils::ShouldShowVerticalTabs(browser_) &&
      state_ == State::kFloating) {
    mouse_enter_timer_.Stop();
    SetState(State::kCollapsed);
  }
}

void VerticalTabStripRegionView::UpdateLayout(bool in_destruction) {
  layout_dirty_ = true;
  if (tabs::utils::ShouldShowVerticalTabs(browser_) && !in_destruction) {
    if (!Contains(original_region_view_)) {
      original_parent_of_region_view_ = original_region_view_->parent();
      original_parent_of_region_view_->RemoveChildView(original_region_view_);
      contents_view_->AddChildView(original_region_view_.get());
    }

    original_region_view_->layout_manager_->SetOrientation(
        views::LayoutOrientation::kVertical);
    if (base::FeatureList::IsEnabled(features::kScrollableTabStrip)) {
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

    original_region_view_->layout_manager_->SetOrientation(
        views::LayoutOrientation::kHorizontal);
    if (base::FeatureList::IsEnabled(features::kScrollableTabStrip)) {
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
  Layout();
}

void VerticalTabStripRegionView::OnThemeChanged() {
  View::OnThemeChanged();

  auto* cp = GetColorProvider();
  CHECK(cp);

  const auto background_color = cp->GetColor(kColorToolbar);
  SetBackground(views::CreateSolidBackground(background_color));

  new_tab_button_->FrameColorsChanged();

  SetBorder(views::CreateSolidSidedBorder(
      gfx::Insets().set_right(1),
      cp->GetColor(kColorBraveVerticalTabSeparator)));
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
  if (auto width = GetContentsBounds().width();
      width && !IsBrowserFullscren()) {
    CHECK_GE(width, tabs::kVerticalTabMinWidth +
                        tabs::kMarginForVerticalTabContainers * 2);
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
    resize_offset_ = cursor_position - bounds_in_screen.right();
  }
  // Note that we're not using |resize_amount|. The variable is offset from
  // the initial point, it grows bigger and bigger.
  auto dest_width = cursor_position - bounds_in_screen.x() - *resize_offset_ -
                    GetInsets().width();
  dest_width = std::clamp(dest_width, tab_style_->GetPinnedWidth() * 3,
                          tab_style_->GetStandardWidth() * 2);
  if (done_resizing) {
    resize_offset_ = absl::nullopt;
  }

  if (*expanded_width_ == dest_width) {
    return;
  }

  // When mouse goes toward web contents area, the cursor could have been
  // changed to the normal cursor. Reset it resize cursor.
  GetWidget()->SetCursor(ui::Cursor(ui::mojom::CursorType::kEastWestResize));

  if (width_animation_.is_animating()) {
    width_animation_.Stop();
    width_animation_.Reset(state_ == State::kCollapsed ? 0 : 1);
  }

  expanded_width_.SetValue(dest_width);
  PreferredSizeChanged();
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

void VerticalTabStripRegionView::OnCollapsedPrefChanged() {
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
    return {2, View::CalculatePreferredSize().height()};
  }

  return {GetPreferredWidthForState(state, include_border, ignore_animation),
          View::CalculatePreferredSize().height()};
}

int VerticalTabStripRegionView::GetPreferredWidthForState(
    State state,
    bool include_border,
    bool ignore_animation) const {
  auto calculate_expanded_width = [&]() {
    return *expanded_width_ + (include_border ? GetInsets().width() : 0);
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
  CHECK(base::FeatureList::IsEnabled(features::kScrollableTabStrip));
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

  NOTREACHED() << "Couldn't find the accelerator for new tab.";
  return {};
}
#endif

BEGIN_METADATA(VerticalTabStripRegionView, views::View)
END_METADATA
