/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/frame/brave_tab_strip_region_view.h"

#include "base/feature_list.h"
#include "base/functional/bind.h"
#include "brave/browser/ui/tabs/brave_tab_prefs.h"
#include "brave/browser/ui/views/tabs/brave_tab_container.h"
#include "brave/browser/ui/views/tabs/brave_tab_strip.h"
#include "brave/browser/ui/views/tabs/vertical_tab_utils.h"
#include "brave/components/vector_icons/vector_icons.h"
#include "build/build_config.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/layout_constants.h"
#include "chrome/browser/ui/tab_search_feature.h"
#include "chrome/browser/ui/tabs/features.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/views/frame/browser_frame_view.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/tabs/shared/tab_strip_combo_button.h"
#include "chrome/browser/ui/views/tabs/tab_search_button.h"
#include "chrome/browser/ui/views/tabs/tab_strip_control_button.h"
#include "chrome/grit/generated_resources.h"
#include "components/prefs/pref_service.h"
#include "components/vector_icons/vector_icons.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/metadata/metadata_header_macros.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/events/event.h"
#include "ui/views/controls/button/button.h"
#include "ui/views/controls/button/button_controller.h"
#include "ui/views/layout/flex_layout_types.h"
#include "ui/views/repeat_controller.h"
#include "ui/views/view_class_properties.h"
#include "ui/views/view_utils.h"

namespace {
// Pattern from ui/views/controls/scrollbar/scroll_bar_button.h (scroll bar
// line/page repeat). Uses kOnPress so the primary action is not fired again on
// release.
class BraveTabStripScrollButton : public TabStripControlButton {
  METADATA_HEADER(BraveTabStripScrollButton, TabStripControlButton)

 public:
  BraveTabStripScrollButton(BrowserWindowInterface* browser_window_interface,
                            base::RepeatingClosure scroll_action,
                            const gfx::VectorIcon& icon);

  BraveTabStripScrollButton(const BraveTabStripScrollButton&) = delete;
  BraveTabStripScrollButton& operator=(const BraveTabStripScrollButton&) =
      delete;

  ~BraveTabStripScrollButton() override;

  // views::View
  bool OnMousePressed(const ui::MouseEvent& event) override;
  void OnMouseReleased(const ui::MouseEvent& event) override;
  void OnMouseCaptureLost() override;
  void OnGestureEvent(ui::GestureEvent* event) override;

 private:
  void OnRepeaterFired();

  base::RepeatingClosure scroll_action_;
  views::RepeatController repeater_;
};

BraveTabStripScrollButton::BraveTabStripScrollButton(
    BrowserWindowInterface* browser_window_interface,
    base::RepeatingClosure scroll_action,
    const gfx::VectorIcon& icon)
    : TabStripControlButton(browser_window_interface,
                            views::Button::PressedCallback(scroll_action),
                            icon,
                            Edge::kNone,
                            Edge::kNone),
      scroll_action_(std::move(scroll_action)),
      repeater_(base::BindRepeating(&BraveTabStripScrollButton::OnRepeaterFired,
                                    base::Unretained(this))) {
  button_controller()->set_notify_action(
      views::ButtonController::NotifyAction::kOnPress);
}

BraveTabStripScrollButton::~BraveTabStripScrollButton() {
  repeater_.Stop();
}

void BraveTabStripScrollButton::OnRepeaterFired() {
  scroll_action_.Run();
}

bool BraveTabStripScrollButton::OnMousePressed(const ui::MouseEvent& event) {
  const bool result = TabStripControlButton::OnMousePressed(event);
  if (GetState() != views::Button::STATE_DISABLED &&
      event.IsOnlyLeftMouseButton()) {
    repeater_.Start();
  }
  return result;
}

void BraveTabStripScrollButton::OnMouseReleased(const ui::MouseEvent& event) {
  repeater_.Stop();
  TabStripControlButton::OnMouseReleased(event);
}

void BraveTabStripScrollButton::OnMouseCaptureLost() {
  repeater_.Stop();
  TabStripControlButton::OnMouseCaptureLost();
}

void BraveTabStripScrollButton::OnGestureEvent(ui::GestureEvent* event) {
  if (GetState() == views::Button::STATE_DISABLED) {
    TabStripControlButton::OnGestureEvent(event);
    return;
  }

  if (event->type() == ui::EventType::kGestureTapDown) {
    TabStripControlButton::OnGestureEvent(event);
    if (GetState() == views::Button::STATE_PRESSED) {
      scroll_action_.Run();
      repeater_.Start();
    }
    event->SetHandled();
    return;
  }

  if (event->type() == ui::EventType::kGestureLongPress) {
    return;
  }

  repeater_.Stop();

  if (event->type() == ui::EventType::kGestureTap) {
    SetState(views::Button::STATE_HOVERED);
    event->SetHandled();
    return;
  }

  TabStripControlButton::OnGestureEvent(event);
}

BEGIN_METADATA(BraveTabStripScrollButton)
END_METADATA

#if BUILDFLAG(IS_LINUX)
ui::DropTargetEvent ConvertRootLocation(views::View* view,
                                        const ui::DropTargetEvent& event) {
  ui::DropTargetEvent converted_event = event;
  auto root_location = event.location();
  views::View::ConvertPointToScreen(view, &root_location);
  converted_event.set_root_location(root_location);
  return converted_event;
}
#endif

}  // namespace

BraveHorizontalTabStripRegionView::~BraveHorizontalTabStripRegionView() =
    default;

void BraveHorizontalTabStripRegionView::CreateScrollButtonsIfNeeded() {
  if (!base::FeatureList::IsEnabled(tabs::kBraveScrollableTabStrip)) {
    return;
  }

  if (HaveScrollButtons()) {
    return;
  }
  auto* bwi = tab_strip_->GetBrowserWindowInterface();
  CHECK(bwi);

  const std::optional<size_t> strip_idx = GetIndexOf(tab_strip_);
  CHECK(strip_idx.has_value());

  // Child order for FlexLayout: leading scroll, tab strip, trailing scroll,
  // then (via base layout) new tab button after the strip cluster.
  tab_scroll_next_button_ = AddChildViewAt(
      std::make_unique<BraveTabStripScrollButton>(
          bwi,
          base::BindRepeating(
              &BraveHorizontalTabStripRegionView::OnScrollNextPressed,
              weak_factory_.GetWeakPtr()),
          vector_icons::kForwardArrowIcon),
      strip_idx.value() + 1);
  tab_scroll_previous_button_ = AddChildViewAt(
      std::make_unique<BraveTabStripScrollButton>(
          bwi,
          base::BindRepeating(
              &BraveHorizontalTabStripRegionView::OnScrollPreviousPressed,
              weak_factory_.GetWeakPtr()),
          vector_icons::kBackArrowIcon),
      strip_idx.value());

  tab_scroll_previous_button_->SetProperty(views::kCrossAxisAlignmentKey,
                                           views::LayoutAlignment::kCenter);
  tab_scroll_next_button_->SetProperty(views::kCrossAxisAlignmentKey,
                                       views::LayoutAlignment::kCenter);
  tab_scroll_previous_button_->SetProperty(
      views::kFlexBehaviorKey,
      views::FlexSpecification(views::MinimumFlexSizeRule::kPreferred,
                               views::MaximumFlexSizeRule::kPreferred));
  tab_scroll_next_button_->SetProperty(
      views::kFlexBehaviorKey,
      views::FlexSpecification(views::MinimumFlexSizeRule::kPreferred,
                               views::MaximumFlexSizeRule::kPreferred));

  // Back/forward arrows are horizontal; mirror when the UI is RTL (see
  // views::ImageButton for the same pattern).
  tab_scroll_previous_button_->SetFlipCanvasOnPaintForRTLUI(true);
  tab_scroll_next_button_->SetFlipCanvasOnPaintForRTLUI(true);

  tab_scroll_previous_button_->SetTooltipText(
      l10n_util::GetStringUTF16(IDS_TOOLTIP_TAB_SCROLL_LEADING));
  tab_scroll_next_button_->SetTooltipText(
      l10n_util::GetStringUTF16(IDS_TOOLTIP_TAB_SCROLL_TRAILING));
  tab_scroll_previous_button_->GetViewAccessibility().SetName(
      l10n_util::GetStringUTF16(IDS_ACCNAME_TAB_SCROLL_LEADING));
  tab_scroll_next_button_->GetViewAccessibility().SetName(
      l10n_util::GetStringUTF16(IDS_ACCNAME_TAB_SCROLL_TRAILING));

  tab_scroll_previous_button_->SetVisible(false);
  tab_scroll_next_button_->SetVisible(false);
}

void BraveHorizontalTabStripRegionView::
    OnShowHorizontalTabScrollButtonsChanged() {
  InvalidateLayout();
}

void BraveHorizontalTabStripRegionView::UpdateScrollButtonsVisibility() {
  if (!HaveScrollButtons()) {
    return;
  }
  auto* strip = views::AsViewClass<BraveTabStrip>(tab_strip_);
  CHECK(strip);
  BraveTabContainer* container = strip->GetBraveTabContainer();
  if (!container) {
    // Can be null if the container isn't yet created.
    return;
  }
  const bool show = container->ShouldShowHorizontalScrollButton() &&
                    *show_horizontal_tab_scroll_buttons_;
  tab_scroll_previous_button_->SetVisible(show);
  tab_scroll_next_button_->SetVisible(show);
  if (show) {
    tab_scroll_previous_button_->SetEnabled(container->CanScrollTabsStart());
    tab_scroll_next_button_->SetEnabled(container->CanScrollTabsEnd());
  }
}

void BraveHorizontalTabStripRegionView::OnScrollPreviousPressed() {
  auto* strip = views::AsViewClass<BraveTabStrip>(tab_strip_);
  CHECK(strip);
  BraveTabContainer* container = strip->GetBraveTabContainer();
  CHECK(container);
  container->ScrollTabsBy(container->GetHorizontalTabScrollStep());
  UpdateScrollButtonsVisibility();
}

void BraveHorizontalTabStripRegionView::OnScrollNextPressed() {
  auto* strip = views::AsViewClass<BraveTabStrip>(tab_strip_);
  CHECK(strip);
  BraveTabContainer* container = strip->GetBraveTabContainer();
  CHECK(container);
  container->ScrollTabsBy(-container->GetHorizontalTabScrollStep());
  UpdateScrollButtonsVisibility();
}

bool BraveHorizontalTabStripRegionView::HaveScrollButtons() const {
  return tab_scroll_previous_button_ && tab_scroll_next_button_;
}

views::View::Views BraveHorizontalTabStripRegionView::GetChildrenInZOrder() {
  views::View::Views order =
      HorizontalTabStripRegionView::GetChildrenInZOrder();
  if (!HaveScrollButtons()) {
    return order;
  }

  for (auto& scroll_button :
       {tab_scroll_previous_button_, tab_scroll_next_button_}) {
    order.push_back(scroll_button.get());
  }
  return order;
}

void BraveHorizontalTabStripRegionView::Layout(PassKey) {
  auto* widget = GetWidget();
  if (!widget || widget->IsClosed()) {
    return;
  }

  UpdateTabStripMargin();
  UpdateScrollButtonsVisibility();

  if (!tabs::utils::ShouldShowBraveVerticalTabs(
          tab_strip_->GetBrowserWindowInterface())) {
    LayoutSuperclass<HorizontalTabStripRegionView>(this);

    // NTB is ignored by flex (`kViewIgnoredByLayoutKey`) and positioned
    // manually by `HorizontalTabStripRegionView::Layout` relative to the tab
    // strip edge. When scroll buttons are visible, leave a gap using layout
    // constants (same family as toolbar spacing). That can overlap the combo's
    // flex slot; we paint NTB above the combo in GetChildrenInZOrder so it
    // stays clickable.
    if (new_tab_button_ && tab_scroll_next_button_ &&
        tab_scroll_next_button_->GetVisible()) {
      const gfx::Size button_size = new_tab_button_->GetPreferredSize();
      const int x = tab_scroll_next_button_->bounds().right() +
                    GetLayoutConstant(LayoutConstant::kTabStripPadding) +
                    GetLayoutConstant(LayoutConstant::kToolbarDividerSpacing);
      new_tab_button_->SetBoundsRect(gfx::Rect(gfx::Point(x, 0), button_size));
    }
    return;
  }

  // in vertical tabs mode, we make tab strip's height is the same with this
  // view's height to avoid extra gaps.
  tab_strip_->SetBoundsRect(gfx::Rect(0, 0, width(), height()));
}

void BraveHorizontalTabStripRegionView::UpdateTabStripMargin() {
  HorizontalTabStripRegionView::UpdateTabStripMargin();

  bool vertical_tabs = tabs::utils::ShouldShowBraveVerticalTabs(
      tab_strip_->GetBrowserWindowInterface());

  UpdateTrailingScrollButtonMargin(vertical_tabs);

  gfx::Insets margins;

  // In horizontal mode, take the current right margin. It is required so that
  // the new tab button will not be covered by the frame grab handle.
  if (!vertical_tabs) {
    if (auto* current = tab_strip_->GetProperty(views::kMarginsKey)) {
      margins.set_right(current->right());
    }
  }

  // Ensure that the correct amount of left margin is applied to the tabstrip.
  // When we are in a fullscreen/condensed mode, we want the tabstrip to meet
  // the frame edge so that the leftmost tab can be selected at the edge of the
  // screen.
  if (tabs::HorizontalTabsUpdateEnabled()) {
    BrowserWindowInterface* browser_window_interface =
        tab_strip_->GetBrowserWindowInterface();
    BrowserView* browser_view =
        BrowserView::GetBrowserViewForBrowser(browser_window_interface);
    BrowserFrameView* browser_frame_view =
        browser_view ? browser_view->browser_widget()->GetFrameView() : nullptr;
    bool is_frame_condensed =
        browser_frame_view && browser_frame_view->IsFrameCondensed();
    if (!is_frame_condensed && !vertical_tabs) {
      margins.set_left(tabs::kHorizontalTabStripLeftMargin);
    } else {
      margins.set_left(0);
    }
  }

  tab_strip_->SetProperty(views::kMarginsKey, margins);
}

void BraveHorizontalTabStripRegionView::UpdateTrailingScrollButtonMargin(
    bool vertical_tabs) {
  if (!HaveScrollButtons()) {
    return;
  }

  if (vertical_tabs) {
    tab_scroll_next_button_->ClearProperty(views::kMarginsKey);
    return;
  }

  auto* strip = views::AsViewClass<BraveTabStrip>(tab_strip_.get());
  if (!strip) {
    tab_scroll_next_button_->ClearProperty(views::kMarginsKey);
    return;
  }

  BraveTabContainer* container = strip->GetBraveTabContainer();
  if (!container) {
    // Can be null if the container isn't yet created.
    tab_scroll_next_button_->ClearProperty(views::kMarginsKey);
    return;
  }

  const bool scroll_active = container->ShouldShowHorizontalScrollButton() &&
                             *show_horizontal_tab_scroll_buttons_;

  if (scroll_active) {
    // Upstream reserves a right margin on the tab strip so the layered NTB can
    // overlap it.  Move that reserve to the trailing scroll button: the strip
    // stays flush against the button, and Layout reads this margin to position
    // the NTB after the gap.
    int upstream_right = 0;
    if (auto* current = tab_strip_->GetProperty(views::kMarginsKey)) {
      upstream_right = current->right();
    }
    tab_scroll_next_button_->SetProperty(
        views::kMarginsKey, gfx::Insets::TLBR(0, 0, 0, upstream_right));
    // Clear the strip's right margin so tabs extend to the trailing button.
    if (auto* current = tab_strip_->GetProperty(views::kMarginsKey)) {
      tab_strip_->SetProperty(views::kMarginsKey,
                              gfx::Insets::TLBR(0, current->left(), 0, 0));
    }
  } else {
    tab_scroll_next_button_->ClearProperty(views::kMarginsKey);
  }
}

void BraveHorizontalTabStripRegionView::OnDragEntered(
    const ui::DropTargetEvent& event) {
#if BUILDFLAG(IS_LINUX)
  if (!tabs::utils::ShouldShowBraveVerticalTabs(
          tab_strip_->GetBrowserWindowInterface())) {
    return HorizontalTabStripRegionView::OnDragEntered(event);
  }

  // Upstream calls TabDragController::Drag() with event.root_location().
  // It should be screen cooridanated location but
  // |event|'s root_location() gives vertical tab widget coordinated location.
  return HorizontalTabStripRegionView::OnDragEntered(
      ConvertRootLocation(this, event));
#else
  return HorizontalTabStripRegionView::OnDragEntered(event);
#endif
}

int BraveHorizontalTabStripRegionView::OnDragUpdated(
    const ui::DropTargetEvent& event) {
#if BUILDFLAG(IS_LINUX)
  if (!tabs::utils::ShouldShowBraveVerticalTabs(
          tab_strip_->GetBrowserWindowInterface())) {
    return HorizontalTabStripRegionView::OnDragUpdated(event);
  }

  // Upstream calls TabDragController::Drag() with event.root_location().
  // It should be screen cooridanated location but
  // |event|'s root_location() gives vertical tab widget coordinated location.
  return HorizontalTabStripRegionView::OnDragUpdated(
      ConvertRootLocation(this, event));
#else
  return HorizontalTabStripRegionView::OnDragUpdated(event);
#endif
}

void BraveHorizontalTabStripRegionView::Initialize() {
  // Use our own icon for the new tab button.
  if (auto* ntb = views::AsViewClass<TabStripControlButton>(new_tab_button_)) {
    ntb->SetVectorIcon(kLeoPlusAddIcon);
  }

  if (features::HasTabSearchToolbarButton() && tab_search_button_) {
    // We have tab search button on toolbar, so we don't need to show the
    // tab search container in horizontal tab strip region view.
    tab_search_button_->SetVisible(false);
  }

  CreateScrollButtonsIfNeeded();

  if (base::FeatureList::IsEnabled(tabs::kBraveScrollableTabStrip)) {
    if (auto* strip = views::AsViewClass<BraveTabStrip>(tab_strip_)) {
      if (BraveTabContainer* container = strip->GetBraveTabContainer()) {
        horizontal_scroll_offset_changed_subscription_ =
            container->RegisterHorizontalScrollOffsetChangedCallback(
                base::BindRepeating(&BraveHorizontalTabStripRegionView::
                                        UpdateScrollButtonsVisibility,
                                    weak_factory_.GetWeakPtr()));
      }
    }

    auto* bwi = tab_strip_->GetBrowserWindowInterface();
    CHECK(bwi);
    PrefService* prefs = bwi->GetProfile()->GetPrefs();
    CHECK(prefs);
    show_horizontal_tab_scroll_buttons_.Init(
        brave_tabs::kShowHorizontalTabScrollButtons, prefs,
        base::BindRepeating(&BraveHorizontalTabStripRegionView::
                                OnShowHorizontalTabScrollButtonsChanged,
                            base::Unretained(this)));
  }
}

BEGIN_METADATA(BraveHorizontalTabStripRegionView)
END_METADATA
