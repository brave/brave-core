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
#include "brave/browser/ui/sidebar/sidebar_service_factory.h"
#include "brave/browser/ui/tabs/brave_tab_prefs.h"
#include "brave/browser/ui/tabs/features.h"
#include "brave/browser/ui/views/tabs/brave_new_tab_button.h"
#include "brave/browser/ui/views/tabs/brave_tab_search_button.h"
#include "brave/browser/ui/views/tabs/brave_tab_strip_layout_helper.h"
#include "brave/browser/ui/views/tabs/vertical_tab_utils.h"
#include "brave/components/sidebar/sidebar_service.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/themes/theme_properties.h"
#include "chrome/browser/ui/exclusive_access/exclusive_access_manager.h"
#include "chrome/browser/ui/exclusive_access/fullscreen_controller.h"
#include "chrome/browser/ui/frame/window_frame_util.h"
#include "chrome/browser/ui/tabs/tab_style.h"
#include "chrome/browser/ui/views/chrome_layout_provider.h"
#include "chrome/browser/ui/views/frame/browser_non_client_frame_view.h"
#include "chrome/browser/ui/views/tabs/tab_strip_scroll_container.h"
#include "chrome/grit/generated_resources.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/compositor/layer.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/paint_vector_icon.h"
#include "ui/gfx/scoped_canvas.h"
#include "ui/views/accessibility/accessibility_paint_checks.h"
#include "ui/views/animation/ink_drop.h"
#include "ui/views/controls/highlight_path_generator.h"
#include "ui/views/controls/scroll_view.h"
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

  // views::Button:
  void OnThemeChanged() override {
    Button::OnThemeChanged();
    auto* cp = GetColorProvider();
    CHECK(cp);

    auto color = cp->GetColor(kColorBraveVerticalTabHeaderButtonColor);
    icon_ = gfx::CreateVectorIcon(kVerticalTabStripToggleButtonIcon, color);
  }

  void PaintIcon(gfx::Canvas* canvas) override {
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

  void PaintFill(gfx::Canvas* canvas) const override {
    // dont' fill
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

 private:
  raw_ptr<VerticalTabStripRegionView> region_view_ = nullptr;
  raw_ptr<const TabStrip> tab_strip_ = nullptr;

  gfx::ImageSkia icon_;
};

BEGIN_METADATA(ToggleButton, views::Button)
END_METADATA

// A custom scroll view to avoid crash on Mac
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
  }

  ~VerticalTabSearchButton() override = default;

  // BraveTabSearchButton:
  SkPath GetBorderPath(const gfx::Point& origin,
                       float scale,
                       bool extend_to_top) const override {
    // Return empty path in order not to fill the background.
    return {};
  }

  void FrameColorsChanged() override {
    TabSearchButton::FrameColorsChanged();

    SetImageModel(views::Button::STATE_NORMAL,
                  ui::ImageModel::FromVectorIcon(
                      kLeoSearchIcon, kColorBraveVerticalTabHeaderButtonColor,
                      /* icon_size= */ 16));
  }
};

BEGIN_METADATA(VerticalTabSearchButton, BraveTabSearchButton)
END_METADATA

class VerticalTabNewTabButton : public BraveNewTabButton {
 public:
  METADATA_HEADER(VerticalTabNewTabButton);

  static constexpr int kPadding = 10;
  static constexpr int kHeight = 50;

  VerticalTabNewTabButton(TabStrip* tab_strip,
                          PressedCallback callback,
                          const std::u16string& shortcut_text,
                          VerticalTabStripRegionView* region_view)
      : BraveNewTabButton(tab_strip, std::move(callback)),
        region_view_(region_view) {
    text_ = AddChildView(std::make_unique<views::Label>(
        l10n_util::GetStringUTF16(IDS_ACCNAME_NEWTAB)));
    text_->SetHorizontalAlignment(gfx::HorizontalAlignment::ALIGN_LEFT);
    text_->SetVerticalAlignment(gfx::VerticalAlignment::ALIGN_MIDDLE);
    text_->SetVisible(false);

    constexpr int kFontSize = 12;
    const auto text_font = text_->font_list();
    text_->SetFontList(
        text_font.DeriveWithSizeDelta(kFontSize - text_font.GetFontSize()));

    shortcut_text_ = AddChildView(std::make_unique<views::Label>());
    shortcut_text_->SetHorizontalAlignment(
        gfx::HorizontalAlignment::ALIGN_RIGHT);
    shortcut_text_->SetVerticalAlignment(gfx::VerticalAlignment::ALIGN_MIDDLE);
    shortcut_text_->SetVisible(false);
    const auto shortcut_font = shortcut_text_->font_list();
    shortcut_text_->SetFontList(shortcut_font.DeriveWithSizeDelta(
        kFontSize - shortcut_font.GetFontSize()));

    SetTooltipText(l10n_util::GetStringUTF16(IDS_TOOLTIP_NEW_TAB));
    SetAccessibleName(l10n_util::GetStringUTF16(IDS_ACCNAME_NEWTAB));
    SetShortcutText(shortcut_text);

    SetImageVerticalAlignment(
        views::ImageButton::VerticalAlignment::ALIGN_MIDDLE);
  }

  ~VerticalTabNewTabButton() override = default;

  // BraveNewTabButton:
  SkPath GetBorderPath(const gfx::Point& origin,
                       float scale,
                       bool extend_to_top) const override {
    auto contents_bounds = GetContentsBounds();
    const float radius = tabs::kUnpinnedTabBorderRadius * scale;
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

    // Set additional horizontal inset for '+' icon.
    if (region_view_->state() ==
        VerticalTabStripRegionView::State::kCollapsed) {
      // When |text_| is empty, the vertical tab strip could be minimized.
      // In this case we should align icon center.
      // TODO(sko) Should we keep this padding when it's transitioned to
      // floating mode?
      SetImageHorizontalAlignment(
          views::ImageButton::HorizontalAlignment::ALIGN_CENTER);
    } else {
      SetImageHorizontalAlignment(
          views::ImageButton::HorizontalAlignment::ALIGN_LEFT);
      canvas->Translate({6, 0});
    }

    // Use vector icon
    ImageButton::PaintButtonContents(canvas);
  }

  gfx::Insets GetInsets() const override {
    return gfx::Insets(tabs::kMarginForVerticalTabContainers);
  }

  void OnPaintFill(gfx::Canvas* canvas) const override {
    if (tab_strip()
            ->GetCustomBackgroundId(BrowserFrameActiveState::kUseCurrent)
            .has_value()) {
      BraveNewTabButton::OnPaintFill(canvas);
      return;
    }

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

  void FrameColorsChanged() override {
    BraveNewTabButton::FrameColorsChanged();

    CHECK(text_ && shortcut_text_);

    auto* cp = GetColorProvider();
    CHECK(cp);

    SetImageModel(views::Button::STATE_NORMAL,
                  ui::ImageModel::FromVectorIcon(
                      kLeoPlusAddIcon, kColorBraveVerticalTabNTBIconColor,
                      /* icon_size= */ 16));
    text_->SetEnabledColor(cp->GetColor(kColorBraveVerticalTabNTBTextColor));
    shortcut_text_->SetEnabledColor(
        cp->GetColor(kColorBraveVerticalTabNTBShortcutTextColor));
  }

  void Layout() override {
    BraveNewTabButton::Layout();

    CHECK(text_ && shortcut_text_);
    const bool show_vertical_tabs =
        tabs::utils::ShouldShowVerticalTabs(tab_strip()->GetBrowser());
    text_->SetVisible(show_vertical_tabs);
    shortcut_text_->SetVisible(show_vertical_tabs);
    if (!text_->GetVisible()) {
      return;
    }

    constexpr int kTextLeftMargin = 32 + kPadding;
    gfx::Rect text_bounds = GetContentsBounds();
    text_bounds.Inset(
        gfx::Insets().set_left(kTextLeftMargin).set_right(kPadding));
    text_->SetBoundsRect(text_bounds);
    shortcut_text_->SetBoundsRect(text_bounds);
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
  raw_ptr<views::Label> text_ = nullptr;
  raw_ptr<views::Label> shortcut_text_ = nullptr;
};

BEGIN_METADATA(VerticalTabNewTabButton, BraveNewTabButton)
END_METADATA

}  // namespace

class VerticalTabStripScrollContentsView : public views::View {
 public:
  METADATA_HEADER(VerticalTabStripScrollContentsView);

  VerticalTabStripScrollContentsView(VerticalTabStripRegionView* container,
                                     TabStrip* tab_strip)
      : container_(container), tab_strip_(tab_strip) {}
  ~VerticalTabStripScrollContentsView() override = default;

  // views::View:
  void ChildPreferredSizeChanged(views::View* child) override {
    if (base::FeatureList::IsEnabled(features::kScrollableTabStrip))
      return;

    if (in_preferred_size_changed_)
      return;

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

class VerticalTabStripRegionView::ScrollHeaderView : public views::View {
 public:
  METADATA_HEADER(ScrollHeaderView);

  ScrollHeaderView(views::Button::PressedCallback toggle_callback,
                   VerticalTabStripRegionView* region_view)
      : region_view_(region_view), tab_strip_(region_view->tab_strip()) {
    SetBorder(views::CreateEmptyBorder(gfx::Insets(kHeaderInset)));

    layout_ = SetLayoutManager(std::make_unique<views::BoxLayout>(
        views::BoxLayout::Orientation::kHorizontal));
    layout_->set_cross_axis_alignment(
        views::BoxLayout::CrossAxisAlignment::kCenter);

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
  ~ScrollHeaderView() override = default;

  BraveTabSearchButton* tab_search_button() { return tab_search_button_; }

  void UpdateTabSearchButtonVisibility() {
    tab_search_button_->SetVisible(
        !WindowFrameUtil::IsWin10TabSearchCaptionButtonEnabled(
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

  // views::View:
  void OnThemeChanged() override {
    View::OnThemeChanged();

    // We should call SetImageModel() after FrameColorChanged() to override
    // the icon.
    toggle_button_->FrameColorsChanged();
    tab_search_button_->FrameColorsChanged();
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

BEGIN_METADATA(VerticalTabStripRegionView, ScrollHeaderView, views::View)
END_METADATA

VerticalTabStripRegionView::VerticalTabStripRegionView(
    BrowserView* browser_view,
    TabStripRegionView* region_view)
    : browser_(browser_view->browser()),
      original_region_view_(region_view),
      tab_style_(TabStyle::Get()) {
  CHECK(base::FeatureList::IsEnabled(tabs::features::kBraveVerticalTabs))
      << "This view should be created only when this flag is on";

  browser_->tab_strip_model()->AddObserver(this);

  SetNotifyEnterExitOnChild(true);

  scroll_view_ = AddChildView(std::make_unique<CustomScrollView>());
  scroll_view_->SetDrawOverflowIndicator(false);
  scroll_view_header_ =
      scroll_view_->SetHeader(std::make_unique<ScrollHeaderView>(
          base::BindRepeating(
              [](VerticalTabStripRegionView* container,
                 const ui::Event& event) {
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

  scroll_contents_view_ = scroll_view_->SetContents(
      std::make_unique<VerticalTabStripScrollContentsView>(
          this, original_region_view_->tab_strip_));
  scroll_contents_view_->SetLayoutManager(
      std::make_unique<views::FillLayout>());
  scroll_view_->SetVerticalScrollBarMode(
      base::FeatureList::IsEnabled(features::kScrollableTabStrip)
          ? views::ScrollView::ScrollBarMode::kDisabled
          : views::ScrollView::ScrollBarMode::kHiddenButEnabled);

  new_tab_button_ = AddChildView(std::make_unique<VerticalTabNewTabButton>(
      original_region_view_->tab_strip_,
      base::BindRepeating(&TabStrip::NewTabButtonPressed,
                          base::Unretained(original_region_view_->tab_strip_)),
      GetShortcutTextForNewTabButton(browser_view), this));

  auto* prefs = browser_->profile()->GetOriginalProfile()->GetPrefs();
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
}

VerticalTabStripRegionView::~VerticalTabStripRegionView() {
  // We need to move tab strip region to its original parent to avoid crash
  // during drag and drop session.
  UpdateLayout(true);
}

bool VerticalTabStripRegionView::IsTabFullscreen() const {
  auto* exclusive_access_manager = browser_->exclusive_access_manager();
  if (!exclusive_access_manager)
    return false;

  auto* fullscreen_controller =
      exclusive_access_manager->fullscreen_controller();
  if (!fullscreen_controller)
    return false;

  return fullscreen_controller->IsWindowFullscreenForTabOrPending();
}

void VerticalTabStripRegionView::SetState(State state) {
  if (state_ == state)
    return;

  mouse_enter_timer_.Stop();
  state_ = state;

  auto tab_strip = original_region_view_->tab_strip_;
  tab_strip->SetAvailableWidthCallback(base::BindRepeating(
      &VerticalTabStripRegionView::GetAvailableWidthForTabContainer,
      base::Unretained(this)));
  tab_strip->tab_container_->InvalidateIdealBounds();
  tab_strip->tab_container_->CompleteAnimationAndLayout();

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
  if (state_ == State::kExpanded)
    return {};

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
  return {0, scroll_view_header_->GetPreferredSize().height()};
}

int VerticalTabStripRegionView::GetAvailableWidthForTabContainer() {
  return GetPreferredWidthForState(state_, /*include_border=*/false);
}

gfx::Size VerticalTabStripRegionView::CalculatePreferredSize() const {
  return GetPreferredSizeForState(state_, /*include_border=*/true);
}

gfx::Size VerticalTabStripRegionView::GetMinimumSize() const {
  if (state_ == State::kFloating) {
    return GetPreferredSizeForState(State::kCollapsed,
                                    /* include_border=*/true);
  }

  return GetPreferredSizeForState(state_, /* include_border= */ true);
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
  auto contents_bounds = GetContentsBounds();
  new_tab_button_->SetSize(
      {contents_bounds.width(), new_tab_button_->GetPreferredSize().height()});
  new_tab_button_->SetPosition(
      {contents_bounds.x(),
       contents_bounds.bottom() - new_tab_button_->height()});

  // 2. ScrollView takes the rest of space.
  // Set preferred size for scroll view to know this.
  const gfx::Size header_size{contents_bounds.width(),
                              tabs::kVerticalTabHeight + kHeaderInset * 2};
  scroll_view_header_->SetPreferredSize(header_size);
  scroll_view_header_->SetSize(header_size);

  scroll_view_->SetSize({contents_bounds.width(),
                         contents_bounds.height() - new_tab_button_->height()});
  scroll_view_->SetPosition(contents_bounds.origin());

  auto scroll_viewport_height = scroll_view_->height() - header_size.height();
  if (scroll_view_->GetMaxHeight() != scroll_viewport_height)
    scroll_view_->ClipHeightTo(0, scroll_viewport_height);

  if (base::FeatureList::IsEnabled(features::kScrollableTabStrip) &&
      tabs::utils::ShouldShowVerticalTabs(browser_)) {
    scroll_contents_view_->SetSize(
        {scroll_view_->width(), scroll_view_->height()});
    auto* nested_scroll_view = GetTabStripScrollContainer()->scroll_view_.get();
    nested_scroll_view->SetSize(
        {scroll_view_->width(), scroll_viewport_height});
    nested_scroll_view->ClipHeightTo(0, scroll_viewport_height);
  } else {
    scroll_contents_view_->SetSize(
        {scroll_view_->width(),
         std::max(scroll_viewport_height,
                  scroll_contents_view_->GetPreferredSize().height())});
  }
  UpdateTabSearchButtonVisibility();
}

void VerticalTabStripRegionView::OnShowVerticalTabsPrefChanged() {
  auto* profile = browser_->profile()->GetOriginalProfile();
  auto* sidebar_service =
      sidebar::SidebarServiceFactory::GetForProfile(profile);
  DCHECK(sidebar_service);

  if (*show_vertical_tabs_) {
    sidebar_service->MoveSidebarToRightTemporarily();
  } else {
    sidebar_service->RestoreSidebarAlignmentIfNeeded();
  }

  UpdateLayout(/* in_destruction= */ false);
}

void VerticalTabStripRegionView::UpdateLayout(bool in_destruction) {
  layout_dirty_ = true;
  if (tabs::utils::ShouldShowVerticalTabs(browser_) && !in_destruction) {
    if (!Contains(original_region_view_)) {
      original_parent_of_region_view_ = original_region_view_->parent();
      original_parent_of_region_view_->RemoveChildView(original_region_view_);
      scroll_view_->contents()->AddChildView(original_region_view_.get());
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
      scroll_view_->contents()->RemoveChildView(original_region_view_);
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
  scroll_view_->SetBackgroundColor(background_color);

  new_tab_button_->FrameColorsChanged();

  SetBorder(views::CreateSolidSidedBorder(
      gfx::Insets().set_right(1),
      cp->GetColor(kColorBraveVerticalTabSeparator)));
}

void VerticalTabStripRegionView::OnMouseExited(const ui::MouseEvent& event) {
  if (IsMouseHovered() && !mouse_events_for_test_) {
    // On Windows, when mouse moves into the area which intersects with web
    // view, OnMouseExited() is invoked even mouse is on this view.
    return;
  }

  mouse_enter_timer_.Stop();
  if (state_ == State::kFloating)
    SetState(State::kCollapsed);
}

void VerticalTabStripRegionView::OnMouseEntered(const ui::MouseEvent& event) {
  if (!tabs::utils::IsFloatingVerticalTabsEnabled(browser_)) {
    return;
  }

  // During tab dragging, this could be already expanded.
  if (state_ == State::kExpanded)
    return;

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

    ScrollActiveTabToBeVisible();
  }

#if DCHECK_IS_ON()
  if (auto width = GetContentsBounds().width(); width) {
    CHECK_GE(width, tabs::kVerticalTabMinWidth +
                        tabs::kMarginForVerticalTabContainers * 2);
  }
#endif
}

void VerticalTabStripRegionView::PreferredSizeChanged() {
  layout_dirty_ = true;
  views::View::PreferredSizeChanged();
}

void VerticalTabStripRegionView::OnTabStripModelChanged(
    TabStripModel* tab_strip_model,
    const TabStripModelChange& change,
    const TabStripSelectionChange& selection) {
  if (!selection.active_tab_changed())
    return;

  ScrollActiveTabToBeVisible();
}

void VerticalTabStripRegionView::UpdateNewTabButtonVisibility() {
  const bool is_vertical_tabs = tabs::utils::ShouldShowVerticalTabs(browser_);
  auto* original_ntb = original_region_view_->new_tab_button();
  original_ntb->SetVisible(!is_vertical_tabs);
  new_tab_button_->SetVisible(is_vertical_tabs);
}

TabSearchBubbleHost* VerticalTabStripRegionView::GetTabSearchBubbleHost() {
  return scroll_view_header_->tab_search_button()->tab_search_bubble_host();
}

int VerticalTabStripRegionView::GetScrollViewViewportHeight() const {
  return scroll_view_->GetMaxHeight();
}

void VerticalTabStripRegionView::UpdateTabSearchButtonVisibility() {
  const bool is_vertical_tabs = tabs::utils::ShouldShowVerticalTabs(browser_);
  if (auto* tab_search_button = original_region_view_->tab_search_button()) {
    tab_search_button->SetVisible(!is_vertical_tabs);
  }
}

void VerticalTabStripRegionView::OnCollapsedPrefChanged() {
  SetState(collapsed_pref_.GetValue() ? State::kCollapsed : State::kExpanded);
}

void VerticalTabStripRegionView::OnFloatingModePrefChanged() {
  if (!tabs::utils::IsFloatingVerticalTabsEnabled(browser_)) {
    if (state_ == State::kFloating)
      SetState(State::kCollapsed);
    return;
  }

  if (IsMouseHovered())
    ScheduleFloatingModeTimer();
}

gfx::Size VerticalTabStripRegionView::GetPreferredSizeForState(
    State state,
    bool include_border) const {
  if (!tabs::utils::ShouldShowVerticalTabs(browser_)) {
    return {};
  }

  if (IsTabFullscreen())
    return {};

  return {GetPreferredWidthForState(state, include_border),
          View::CalculatePreferredSize().height()};
}

int VerticalTabStripRegionView::GetPreferredWidthForState(
    State state,
    bool include_border) const {
  if (state == State::kExpanded || state == State::kFloating)
    return tab_style_->GetStandardWidth() +
           (include_border ? GetInsets().width() : 0);

  CHECK_EQ(state, State::kCollapsed) << "If a new state was added, "
                                     << __FUNCTION__ << " should be revisited.";
  return tabs::kVerticalTabMinWidth +
         tabs::kMarginForVerticalTabContainers * 2 +
         (include_border ? GetInsets().width() : 0);
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

void VerticalTabStripRegionView::ScheduleFloatingModeTimer() {
  if (mouse_events_for_test_) {
    SetState(State::kFloating);
    return;
  }

  mouse_enter_timer_.Stop();
  if (state_ == State::kCollapsed) {
    mouse_enter_timer_.Start(
        FROM_HERE, base::Milliseconds(400),
        base::BindOnce(&VerticalTabStripRegionView::SetState,
                       base::Unretained(this), State::kFloating));
  }
}

void VerticalTabStripRegionView::ScrollActiveTabToBeVisible() {
  if (!tabs::utils::ShouldShowVerticalTabs(browser_)) {
    return;
  }

  auto active_index = browser_->tab_strip_model()->active_index();
  if (active_index == TabStripModel::kNoTab) {
    // This could happen on destruction.
    return;
  }

  auto* active_tab = original_region_view_->tab_strip_->tab_at(active_index);
  CHECK(active_tab);

  gfx::RectF tab_bounds_in_contents_view(active_tab->GetLocalBounds());
  views::View::ConvertRectToTarget(active_tab, scroll_contents_view_,
                                   &tab_bounds_in_contents_view);

  auto visible_rect = scroll_view_->GetVisibleRect();
  if (visible_rect.Contains(gfx::Rect(0, tab_bounds_in_contents_view.y(),
                                      1 /*in order to ignore width */,
                                      tab_bounds_in_contents_view.height()))) {
    return;
  }

  // Unfortunately, ScrollView's API doesn't work well for us. So we manually
  // adjust scroll offset. Note that we change contents view's position as
  // we disabled layered scroll view.
  if (visible_rect.y() > tab_bounds_in_contents_view.bottom()) {
    scroll_contents_view_->SetPosition(
        {0, -static_cast<int>(tab_bounds_in_contents_view.y())});
  } else {
    scroll_contents_view_->SetPosition(
        {0, -static_cast<int>(tab_bounds_in_contents_view.bottom()) +
                scroll_view_->height() - scroll_view_header_->height()});
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
