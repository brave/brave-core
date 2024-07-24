/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/sidebar/sidebar_container_view.h"

#include <algorithm>
#include <limits>
#include <optional>
#include <utility>

#include "base/functional/bind.h"
#include "base/time/time.h"
#include "brave/browser/ui/brave_browser.h"
#include "brave/browser/ui/color/brave_color_id.h"
#include "brave/browser/ui/sidebar/sidebar_controller.h"
#include "brave/browser/ui/sidebar/sidebar_model.h"
#include "brave/browser/ui/sidebar/sidebar_service_factory.h"
#include "brave/browser/ui/sidebar/sidebar_utils.h"
#include "brave/browser/ui/tabs/features.h"
#include "brave/browser/ui/tabs/shared_pinned_tab_service.h"
#include "brave/browser/ui/tabs/shared_pinned_tab_service_factory.h"
#include "brave/browser/ui/views/frame/brave_browser_view.h"
#include "brave/browser/ui/views/frame/brave_browser_view_layout.h"
#include "brave/browser/ui/views/frame/brave_contents_view_util.h"
#include "brave/browser/ui/views/side_panel/brave_side_panel.h"
#include "brave/browser/ui/views/side_panel/playlist/playlist_side_panel_coordinator.h"
#include "brave/browser/ui/views/sidebar/sidebar_control_view.h"
#include "brave/browser/ui/views/toolbar/brave_toolbar_view.h"
#include "brave/browser/ui/views/toolbar/side_panel_button.h"
#include "brave/components/constants/pref_names.h"
#include "brave/components/constants/webui_url_constants.h"
#include "brave/components/sidebar/browser/sidebar_item.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/browser/ui/color/chrome_color_id.h"
#include "chrome/browser/ui/exclusive_access/exclusive_access_manager.h"
#include "chrome/browser/ui/exclusive_access/fullscreen_controller.h"
#include "chrome/browser/ui/exclusive_access/fullscreen_within_tab_helper.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/side_panel/side_panel_entry.h"
#include "chrome/browser/ui/views/side_panel/side_panel_registry.h"
#include "chrome/browser/ui/views/side_panel/side_panel_web_ui_view.h"
#include "chrome/browser/ui/views/toolbar/toolbar_view.h"
#include "components/grit/brave_components_strings.h"
#include "components/input/native_web_keyboard_event.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/web_contents.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/compositor/layer.h"
#include "ui/events/event_observer.h"
#include "ui/events/types/event_type.h"
#include "ui/gfx/geometry/point.h"
#include "ui/views/border.h"
#include "ui/views/controls/webview/webview.h"
#include "ui/views/event_monitor.h"
#include "ui/views/view_class_properties.h"
#include "ui/views/view_utils.h"
#include "ui/views/widget/widget.h"
#include "url/gurl.h"

namespace {

using ShowSidebarOption = sidebar::SidebarService::ShowSidebarOption;

sidebar::SidebarService* GetSidebarService(BraveBrowser* browser) {
  return sidebar::SidebarServiceFactory::GetForProfile(browser->profile());
}

SharedPinnedTabService* GetSharedPinnedTabService(Profile* profile) {
  if (base::FeatureList::IsEnabled(tabs::features::kBraveSharedPinnedTabs)) {
    return SharedPinnedTabServiceFactory::GetForProfile(profile);
  }

  return nullptr;
}

}  // namespace

class SidebarContainerView::BrowserWindowEventObserver
    : public ui::EventObserver {
 public:
  explicit BrowserWindowEventObserver(SidebarContainerView& host)
      : host_(host) {}
  ~BrowserWindowEventObserver() override = default;
  BrowserWindowEventObserver(const BrowserWindowEventObserver&) = delete;
  BrowserWindowEventObserver& operator=(const BrowserWindowEventObserver&) =
      delete;

  void OnEvent(const ui::Event& event) override {
    DCHECK(event.IsMouseEvent());
    const auto* mouse_event = event.AsMouseEvent();

    gfx::Point window_event_position = mouse_event->location();
    // Convert window position to sidebar view's coordinate and check whether
    // it's included in sidebar ui or not.
    // If it's not included and sidebar could be hidden, stop monitoring and
    // hide UI.
    views::View::ConvertPointFromWidget(host_->sidebar_control_view_,
                                        &window_event_position);
    if (!host_->sidebar_control_view_->GetLocalBounds().Contains(
            window_event_position) &&
        !host_->ShouldForceShowSidebar()) {
      host_->StopBrowserWindowEventMonitoring();
      host_->HideSidebarAll();
    }
  }

 private:
  const raw_ref<SidebarContainerView> host_;
};

SidebarContainerView::SidebarContainerView(
    BraveBrowser* browser,
    SidePanelCoordinator* side_panel_coordinator,
    std::unique_ptr<BraveSidePanel> side_panel)
    : views::AnimationDelegateViews(this),
      browser_(browser),
      side_panel_coordinator_(side_panel_coordinator),
      browser_window_event_observer_(
          std::make_unique<BrowserWindowEventObserver>(*this)) {
  constexpr int kAnimationDurationMS = 150;
  width_animation_.SetSlideDuration(base::Milliseconds(kAnimationDurationMS));

  SetNotifyEnterExitOnChild(true);
  side_panel_ = AddChildView(std::move(side_panel));
}

SidebarContainerView::~SidebarContainerView() = default;

void SidebarContainerView::Init() {
  initialized_ = true;

  sidebar_model_ = browser_->sidebar_controller()->model();
  sidebar_model_observation_.Observe(sidebar_model_);
  browser_->tab_strip_model()->AddObserver(this);

  auto* browser_view = BrowserView::GetBrowserViewForBrowser(browser_);
  DCHECK(browser_view);

  auto* side_panel_registry =
      SidePanelCoordinator::GetGlobalSidePanelRegistry(browser_);
  panel_registry_observations_.AddObservation(side_panel_registry);

  for (const auto& entry : side_panel_registry->entries()) {
    DVLOG(1) << "Observing panel entry in ctor: "
             << SidePanelEntryIdToString(entry->key().id());
    panel_entry_observations_.AddObservation(entry.get());
  }

  show_side_panel_button_.Init(
      kShowSidePanelButton, browser_->profile()->GetPrefs(),
      base::BindRepeating(&SidebarContainerView::UpdateToolbarButtonVisibility,
                          base::Unretained(this)));

  AddChildViews();
  UpdateToolbarButtonVisibility();
  SetSidebarShowOption(GetSidebarService(browser_)->GetSidebarShowOption());
}

void SidebarContainerView::SetSidebarOnLeft(bool sidebar_on_left) {
  DCHECK(initialized_);

  if (sidebar_on_left_ == sidebar_on_left) {
    return;
  }

  sidebar_on_left_ = sidebar_on_left;

  DCHECK(sidebar_control_view_);
  sidebar_control_view_->SetSidebarOnLeft(sidebar_on_left_);

  DCHECK(side_panel_);
  side_panel_->SetHorizontalAlignment(
      sidebar_on_left ? BraveSidePanel::kHorizontalAlignLeft
                      : BraveSidePanel::kHorizontalAlignRight);

  GetEventDetectWidget()->SetSidebarOnLeft(sidebar_on_left_);
}

bool SidebarContainerView::IsSidebarVisible() const {
  return sidebar_control_view_ && sidebar_control_view_->GetVisible();
}

bool SidebarContainerView::IsFullscreenForCurrentEntry() const {
  // For now, we only supports fullscreen from playlist.
  if (side_panel_coordinator_->GetCurrentEntryId() !=
      SidePanelEntryId::kPlaylist) {
    return false;
  }

  // TODO(sko) Do we have a more general way to get WebContents of the active
  // entry?
  auto web_view = PlaylistSidePanelCoordinator::FromBrowser(browser_)
                      ->side_panel_web_view();
  if (!web_view) {
    return false;
  }

  auto* contents = web_view->web_contents();
  if (!contents) {
    return false;
  }

  if (auto* fullscreen_tab_helper =
          FullscreenWithinTabHelper::FromWebContents(contents);
      fullscreen_tab_helper &&
      fullscreen_tab_helper->is_fullscreen_within_tab()) {
    return true;
  }

  return false;
}

void SidebarContainerView::SetSidebarShowOption(ShowSidebarOption show_option) {
  DVLOG(2) << __func__;

  // Hide event detect widget when option is chaged from mouse over to others.
  if (show_sidebar_option_ == ShowSidebarOption::kShowOnMouseOver) {
    ShowOptionsEventDetectWidget(false);
  }

  show_sidebar_option_ = show_option;

  const bool is_panel_visible = side_panel_->GetVisible();
  if (show_sidebar_option_ == ShowSidebarOption::kShowAlways) {
    is_panel_visible ? ShowSidebarAll() : ShowSidebarControlView();
    return;
  }

  if (show_sidebar_option_ == ShowSidebarOption::kShowNever) {
    if (!is_panel_visible) {
      HideSidebarAll();
    }
    return;
  }

  if (IsMouseHovered() || is_panel_visible) {
    is_panel_visible ? ShowSidebarAll() : ShowSidebarControlView();
    return;
  }

  HideSidebarAll();
}

void SidebarContainerView::UpdateSidebarItemsState() {
  // control view has items.
  sidebar_control_view_->Update();
}

void SidebarContainerView::MenuClosed() {
  DVLOG(1) << __func__;

  // Don't need to to auto hide sidebar UI for other options.
  if (show_sidebar_option_ != ShowSidebarOption::kShowOnMouseOver) {
    return;
  }

  // Don't hide sidebar with below conditions.
  if (IsMouseHovered() || ShouldForceShowSidebar()) {
    return;
  }

  HideSidebarAll();
}

void SidebarContainerView::UpdateBackground() {
  if (const ui::ColorProvider* color_provider = GetColorProvider()) {
    // Fill background because panel's color uses alpha value.
    SetBackground(
        views::CreateSolidBackground(color_provider->GetColor(kColorToolbar)));
  }
}

void SidebarContainerView::AddChildViews() {
  sidebar_control_view_ =
      AddChildView(std::make_unique<SidebarControlView>(this, browser_));
  sidebar_control_view_->SetPaintToLayer();

  // To prevent showing layered-children while its bounds is invisible.
  sidebar_control_view_->layer()->SetMasksToBounds(true);

  // Hide by default. Visibility will be controlled by show options callback
  // later.
  sidebar_control_view_->SetVisible(false);
}

void SidebarContainerView::Layout(PassKey) {
  if (!initialized_) {
    LayoutSuperclass<views::View>(this);
    return;
  }

  // As control view uses its own layer, we should set its size exactly.
  // Otherwise, it's rendered even parent rect width is zero.
  int control_view_width =
      std::min(sidebar_control_view_->GetPreferredSize().width(), width());

  // Control view should not be shown in panel-initiated fullscreen.
  if (IsFullscreenForCurrentEntry()) {
    control_view_width = 0;
  }

  const int control_view_x =
      sidebar_on_left_ ? 0 : width() - control_view_width;
  const int side_panel_x = sidebar_on_left_ ? control_view_width : 0;
  sidebar_control_view_->SetBounds(control_view_x, 0, control_view_width,
                                   height());
  if (side_panel_->GetVisible()) {
    gfx::Rect side_panel_bounds(side_panel_x, 0, width() - control_view_width,
                                height());
    side_panel_bounds.Inset(*side_panel_->GetProperty(views::kMarginsKey));

    side_panel_->SetBoundsRect(side_panel_bounds);
  }
}

gfx::Size SidebarContainerView::CalculatePreferredSize(
    const views::SizeBounds& available_size) const {
  if (!initialized_ || !sidebar_control_view_->GetVisible() ||
      IsFullscreenByTab()) {
    return View::CalculatePreferredSize(available_size);
  }

  if (IsFullscreenForCurrentEntry()) {
    return {std::numeric_limits<int>::max(), 0};
  }

  auto start_width = animation_start_width_;
  auto end_width = animation_end_width_;
  if (width_animation_.IsClosing()) {
    start_width = animation_end_width_;
    end_width = animation_start_width_;
  }

  if (width_animation_.is_animating()) {
    return {gfx::Tween::IntValueBetween(width_animation_.GetCurrentValue(),
                                        start_width, end_width),
            0};
  }

  int preferred_width = 0;
  if (sidebar_control_view_->GetVisible()) {
    preferred_width = sidebar_control_view_->GetPreferredSize().width();
  }

  if (side_panel_->GetVisible()) {
    preferred_width += side_panel_->GetPreferredSize().width() +
                       side_panel_->GetProperty(views::kMarginsKey)->width();
  }

  return {preferred_width, 0};
}

void SidebarContainerView::OnThemeChanged() {
  View::OnThemeChanged();

  UpdateBackground();
}

bool SidebarContainerView::IsFullscreenByTab() const {
  DCHECK(browser_->exclusive_access_manager() &&
         browser_->exclusive_access_manager()->fullscreen_controller());
  return browser_->exclusive_access_manager()
      ->fullscreen_controller()
      ->IsWindowFullscreenForTabOrPending();
}

bool SidebarContainerView::ShouldForceShowSidebar() const {
  // It is more reliable to check whether coordinator has current entry rather
  // than checking if side_panel_ is visible.
  return side_panel_coordinator_->GetCurrentEntryId() ||
         sidebar_control_view_->IsItemReorderingInProgress() ||
         sidebar_control_view_->IsBubbleWidgetVisible();
}

void SidebarContainerView::OnMouseEntered(const ui::MouseEvent& event) {
  if (show_sidebar_option_ != ShowSidebarOption::kShowOnMouseOver) {
    return;
  }

  // Cancel hide schedule when mouse entered again quickly.
  sidebar_hide_timer_.Stop();
}

void SidebarContainerView::OnMouseExited(const ui::MouseEvent& event) {
  if (show_sidebar_option_ != ShowSidebarOption::kShowOnMouseOver) {
    return;
  }

  // When context menu is shown, this view can get this exited callback.
  // In that case, ignore this callback because mouse is still in this view.
  if (IsMouseHovered()) {
    return;
  }

  if (ShouldForceShowSidebar()) {
    StartBrowserWindowEventMonitoring();
    return;
  }

  // Give some delay for hiding to prevent flickering by open/hide quickly.
  // when mouse is moved around the sidebar.
  constexpr int kHideDelayInMS = 400;
  sidebar_hide_timer_.Start(
      FROM_HERE, base::Milliseconds(kHideDelayInMS),
      base::BindOnce(&SidebarContainerView::HideSidebarAll,
                     base::Unretained(this)));
}

void SidebarContainerView::AnimationProgressed(
    const gfx::Animation* animation) {
  PreferredSizeChanged();
}

void SidebarContainerView::AnimationEnded(const gfx::Animation* animation) {
  side_panel_->set_fixed_contents_width(std::nullopt);

  PreferredSizeChanged();

  // Handle children's visibility after animation completes.
  const bool hide_animation_ended = (width_animation_.GetCurrentValue() == 0);
  if (hide_animation_ended) {
    DVLOG(1) << __func__ << " :  Hide animation ended.";
    // Hide all means panel and control view both are hidden.
    // Otherwise, only panel is hidden.
    const bool did_hide_all = animation_end_width_ == 0;
    if (did_hide_all) {
      ShowOptionsEventDetectWidget(true);
      sidebar_control_view_->SetVisible(false);
    } else {
      sidebar_control_view_->SetVisible(true);
    }
    side_panel_->SetVisible(false);
  } else {
    DVLOG(1) << __func__ << " : Show animation ended.";
  }

  animation_start_width_ = animation_end_width_ = 0;
}

void SidebarContainerView::OnActiveIndexChanged(
    std::optional<size_t> old_index,
    std::optional<size_t> new_index) {
  DVLOG(1) << "OnActiveIndexChanged: "
           << (old_index ? std::to_string(*old_index) : "none") << " to "
           << (new_index ? std::to_string(*new_index) : "none");
  if (new_index) {
    ShowSidebarAll();
  } else {
    // If sidebar model's active index is changed to none,
    // there are two possible scenarios.
    // 1. Managed entry is de-activated and no other entry is shown.
    //    In this case, we should hide panel.
    // 2. Managed entry is de-activated and non-managed entry is shown.
    //    In this case, we should not hide panel.
    // When changing panel entry from managed to non-managed by calling
    // SidePanelCoordinator::Show(), OnEntryShown() for non-managed is
    // arrived first and then OnEntryHidden() for managed is called.
    // And this method is called by last OnEntryHidden(). So, coordinator
    // already has non-managed entry.
    if (side_panel_coordinator_->GetCurrentEntryId()) {
      return;
    }

    HideSidebarForShowOption();
  }
}

void SidebarContainerView::OnItemAdded(const sidebar::SidebarItem& item,
                                       size_t index,
                                       bool user_gesture) {
  UpdateToolbarButtonVisibility();
}

void SidebarContainerView::OnItemRemoved(size_t index) {
  UpdateToolbarButtonVisibility();
}

SidebarShowOptionsEventDetectWidget*
SidebarContainerView::GetEventDetectWidget() {
  if (!show_options_widget_) {
    show_options_widget_ =
        std::make_unique<SidebarShowOptionsEventDetectWidget>(
            *static_cast<BraveBrowserView*>(
                BrowserView::GetBrowserViewForBrowser(browser_)),
            *this);
    show_options_widget_->Hide();
  }

  return show_options_widget_.get();
}

void SidebarContainerView::ShowOptionsEventDetectWidget(bool show) {
  if (show_sidebar_option_ != ShowSidebarOption::kShowOnMouseOver) {
    return;
  }

  show ? GetEventDetectWidget()->Show() : GetEventDetectWidget()->Hide();
}

void SidebarContainerView::ShowSidebarControlView() {
  DVLOG(1) << __func__;
  ShowSidebar(false);
}

void SidebarContainerView::ShowSidebar(bool show_side_panel) {
  DVLOG(1) << __func__ << ": show panel: " << show_side_panel;

  // Don't need to show again if it's showing now.
  if (width_animation_.is_animating() && width_animation_.IsShowing()) {
    DVLOG(1) << __func__ << ": showing in-progress.";
    return;
  }

  // Stop closing animation and will start showing from there.
  // Unfortunately, this optimization doesn't have much effect
  // because showing can start only after panel's contents is ready.
  if (width_animation_.is_animating() && width_animation_.IsClosing()) {
    DVLOG(1) << __func__ << ": stop hiding and start showing from there.";
    width_animation_.Stop();
  } else {
    // Otherwise, reset animation to start from the beginning.
    width_animation_.Reset();
  }

  // Calculate the start & end width for animation. Both are used when
  // calculating preferred width during the show animation.
  animation_start_width_ = width();
  animation_end_width_ = sidebar_control_view_->GetPreferredSize().width();
  if (show_side_panel) {
    animation_end_width_ += side_panel_->GetPreferredSize().width();
  }

  // Don't need event detect widget when sidebar gets visible.
  ShowOptionsEventDetectWidget(false);

  DVLOG(1) << __func__ << ": show animation (start, end) width: ("
           << animation_start_width_ << ", " << animation_end_width_ << ")";

  sidebar_control_view_->SetVisible(true);
  side_panel_->SetVisible(show_side_panel);

  if (animation_start_width_ == animation_end_width_) {
    DVLOG(1) << __func__ << ": already at the target width.";
    return;
  }

  // Don't do show animation for control view when show always options is used.
  // This animation can cause upstream browser test
  // PersistentBackground/ExtensionApiTabTestWithContextType.Size failure
  // because it checks the tab size of initial tab and dupclited tab.
  // Initial tab width could be more wider than later opened tab because of
  // sidebar show animation.
  if (show_sidebar_option_ == ShowSidebarOption::kShowAlways &&
      !show_side_panel) {
    DVLOG(1) << __func__ << ": show w/o animation";
    InvalidateLayout();
    return;
  }

  // Animation will trigger layout by changing preferred size.
  if (ShouldUseAnimation()) {
    DVLOG(1) << __func__ << ": show with animation";
    if (show_side_panel) {
      // To show side panel with animation, we need to know exact fianl end
      // width and `BraveBrowserViewLayout` only knows it because side panel's
      // preferred size could be different with current width by resizing window
      // size. If window size doesn't have sufficent width for sidebar's
      // preferred width, `BraveBrowserViewLayout` allocates more smaller width
      // to it.
      auto* browser_view = BrowserView::GetBrowserViewForBrowser(browser_);
      const int target_sidebar_width =
          static_cast<BraveBrowserViewLayout*>(browser_view->GetLayoutManager())
              ->GetIdealSideBarWidth();
      animation_end_width_ =
          std::min(animation_end_width_, target_sidebar_width);
      side_panel_->set_fixed_contents_width(
          animation_end_width_ -
          sidebar_control_view_->GetPreferredSize().width());
    }

    width_animation_.Show();
    return;
  }

  DVLOG(1) << __func__ << ": show w/o animation";
  // Otherwise, layout should be requested here.
  side_panel_->SetVisible(true);
  InvalidateLayout();
}

void SidebarContainerView::ShowSidebarAll() {
  ShowSidebar(true);
}

void SidebarContainerView::HideSidebar(bool hide_sidebar_control) {
  DVLOG(1) << __func__ << ": hide control: " << hide_sidebar_control;

  // Don't need to close again if it's closing now.
  if (width_animation_.is_animating() && width_animation_.IsClosing()) {
    DVLOG(1) << __func__ << ": hiding in-progress.";
    return;
  }

  // Stop showing animation and start closing immediately from there.
  if (width_animation_.is_animating() && width_animation_.IsShowing()) {
    DVLOG(1) << __func__ << ": stop showing and start hiding from there.";
    width_animation_.Stop();
  } else {
    // Otherwise, reset animation to hide from the end.
    width_animation_.Reset(1.0);
  }

  // Calculate the start & end width for animation. Both are used when
  // calculating preferred width during the hide animation.
  animation_start_width_ = width();
  animation_end_width_ = 0;
  if (!hide_sidebar_control) {
    animation_end_width_ = sidebar_control_view_->GetPreferredSize().width();
  }

  if (animation_start_width_ == animation_end_width_) {
    DVLOG(1) << __func__ << ": already at the target width.";

    // At startup, make event detect widget visible even if children's
    // visibility state is not changed.
    if (animation_end_width_ == 0) {
      ShowOptionsEventDetectWidget(true);
    }

    sidebar_control_view_->SetVisible(!hide_sidebar_control);
    side_panel_->SetVisible(false);
    return;
  }

  DVLOG(1) << __func__ << ": hide animation (start, end) width: ("
           << animation_start_width_ << ", " << animation_end_width_ << ")";

  if (ShouldUseAnimation()) {
    DVLOG(1) << __func__ << ": hide with animation";

    if (side_panel_->GetVisible()) {
      side_panel_->set_fixed_contents_width(side_panel_->width());
    }

    width_animation_.Hide();
    return;
  }

  DVLOG(1) << __func__ << ": hide w/o animation";
  if (animation_end_width_ == 0) {
    ShowOptionsEventDetectWidget(true);
  }

  sidebar_control_view_->SetVisible(!hide_sidebar_control);
  side_panel_->SetVisible(false);
  InvalidateLayout();
}

void SidebarContainerView::HideSidebarAll() {
  HideSidebar(true);
}

void SidebarContainerView::HideSidebarPanel() {
  HideSidebar(false);
}

void SidebarContainerView::HideSidebarForShowOption() {
  if (show_sidebar_option_ == ShowSidebarOption::kShowAlways) {
    HideSidebarPanel();
    return;
  }

  if (show_sidebar_option_ == ShowSidebarOption::kShowOnMouseOver) {
    // Hide all if mouse is outside of control view.
    sidebar_control_view_->IsMouseHovered() ? HideSidebarPanel()
                                            : HideSidebarAll();
    return;
  }

  if (show_sidebar_option_ == ShowSidebarOption::kShowNever) {
    HideSidebarAll();
    return;
  }
}

bool SidebarContainerView::ShouldUseAnimation() {
  return !operation_from_active_tab_change_ &&
         gfx::Animation::ShouldRenderRichAnimation();
}

void SidebarContainerView::UpdateToolbarButtonVisibility() {
  // Coordinate sidebar toolbar button visibility based on
  // whether there are any sibar items with a sidepanel.
  // This is similar to how chromium's side_panel_coordinator View
  // also has some control on the toolbar button.
  auto has_panel_item =
      GetSidebarService(browser_)->GetDefaultPanelItem().has_value();
  auto* browser_view = BrowserView::GetBrowserViewForBrowser(browser_);
  auto* brave_toolbar = static_cast<BraveToolbarView*>(browser_view->toolbar());
  if (brave_toolbar && brave_toolbar->side_panel_button()) {
    brave_toolbar->side_panel_button()->SetVisible(
        has_panel_item && show_side_panel_button_.GetValue());
  }
}

void SidebarContainerView::StartBrowserWindowEventMonitoring() {
  if (browser_window_event_monitor_) {
    return;
  }

  DVLOG(1) << __func__;
  browser_window_event_monitor_ = views::EventMonitor::CreateWindowMonitor(
      browser_window_event_observer_.get(), GetWidget()->GetNativeWindow(),
      {ui::EventType::kMouseMoved});
}

void SidebarContainerView::StopBrowserWindowEventMonitoring() {
  DVLOG(1) << __func__;
  browser_window_event_monitor_.reset();
}

void SidebarContainerView::OnEntryShown(SidePanelEntry* entry) {
  // Make sure item is selected. We need to observe the SidePanel system
  // as well as Sidebar as there are other ways than Sidebar for SidePanel
  // items to be shown and hidden, e.g. toolbar button.
  DVLOG(1) << "Panel shown: " << SidePanelEntryIdToString(entry->key().id());
  auto* controller = browser_->sidebar_controller();

  // Handling if |entry| is managed one.
  for (const auto& item : sidebar_model_->GetAllSidebarItems()) {
    if (!item.open_in_panel) {
      continue;
    }
    if (entry->key().id() == sidebar::SidePanelIdFromSideBarItem(item)) {
      const auto sidebar_index = sidebar_model_->GetIndexOf(item);
      controller->ActivateItemAt(sidebar_index);
      return;
    }
  }

  // Add item for this entry if it's hidden in sidebar but shown its panel.
  if (auto item =
          sidebar::AddItemForSidePanelIdIfNeeded(browser_, entry->key().id())) {
    const auto sidebar_index = sidebar_model_->GetIndexOf(*item);
    controller->ActivateItemAt(sidebar_index);
    return;
  }

  // Handling non-managed entry. It should be shown here instead of
  // asking to SidebarModel.
  // If side panel is shown by this kind of panel, showing should
  // be done here because it is not controlled by our sidebar model.
  ShowSidebarAll();
}

void SidebarContainerView::OnEntryHidden(SidePanelEntry* entry) {
  // Make sure item is deselected
  DVLOG(1) << "Panel hidden: " << SidePanelEntryIdToString(entry->key().id());
  auto* controller = browser_->sidebar_controller();

  // Handling if |entry| is managed one.
  for (const auto& item : sidebar_model_->GetAllSidebarItems()) {
    if (!item.open_in_panel) {
      continue;
    }

    if (entry->key().id() == sidebar::SidePanelIdFromSideBarItem(item)) {
      const auto sidebar_index = sidebar_model_->GetIndexOf(item);
      // Only deactivate sidebar item for hidden |entry| when it was active
      // and it's not active one now.
      // It can happen when shown & hidden entries have same sidebar item(ex,
      // different tab uses ai-chat). In this case, don't need to deactivate
      // item because same item should be activated.
      if (controller->IsActiveIndex(sidebar_index) &&
          side_panel_coordinator_->GetCurrentEntryId() != entry->key().id()) {
        controller->ActivateItemAt(std::nullopt);
        return;
      }
    }
  }

  // Handling non-managed entry.
  // If non-managed entry is hidden and there is no active entry,
  // panel should be hidden here.
  if (!side_panel_coordinator_->GetCurrentEntryId()) {
    HideSidebarForShowOption();
  }
}

void SidebarContainerView::OnEntryRegistered(SidePanelRegistry* registry,
                                             SidePanelEntry* entry) {
  // Observe when it's shown or hidden
  DVLOG(1) << "Observing panel entry in registry observer: "
           << SidePanelEntryIdToString(entry->key().id());
  panel_entry_observations_.AddObservation(entry);
}

void SidebarContainerView::OnEntryWillDeregister(SidePanelRegistry* registry,
                                                 SidePanelEntry* entry) {
  // Stop observing
  DVLOG(1) << "Unobserving panel entry in registry observer: "
           << SidePanelEntryIdToString(entry->key().id());
  panel_entry_observations_.RemoveObservation(entry);
}

void SidebarContainerView::OnTabStripModelChanged(
    TabStripModel* tab_strip_model,
    const TabStripModelChange& change,
    const TabStripSelectionChange& selection) {
  // Need to [de]register contextual registry when tab is replaced.
  if ((change.type() == TabStripModelChange::kReplaced)) {
    auto* replace = change.GetReplace();
    StartObservingContextualSidePanelRegistry(replace->new_contents);
    StopObservingContextualSidePanelRegistry(replace->old_contents);
    return;
  }

  if (change.type() == TabStripModelChange::kInserted) {
    for (const auto& contents : change.GetInsert()->contents) {
      StartObservingContextualSidePanelRegistry(contents.contents);
    }
    return;
  }

  if (change.type() == TabStripModelChange::kRemoved) {
    for (const auto& contents : change.GetRemove()->contents) {
      StopObservingContextualSidePanelRegistry(contents.contents);
    }
    return;
  }
}

void SidebarContainerView::StopObservingContextualSidePanelRegistry(
    content::WebContents* contents) {
  auto* registry = SidePanelRegistry::Get(contents);
  if (!registry) {
    return;
  }

  panel_registry_observations_.RemoveObservation(registry);

  for (const auto& entry : registry->entries()) {
    if (panel_entry_observations_.IsObservingSource(entry.get())) {
      DVLOG(1) << "Removing panel entry observation from removed contextual "
                  "registry : "
               << SidePanelEntryIdToString(entry->key().id());
      panel_entry_observations_.RemoveObservation(entry.get());
    }
  }
}

void SidebarContainerView::StartObservingContextualSidePanelRegistry(
    content::WebContents* contents) {
  auto* registry = SidePanelRegistry::Get(contents);
  if (!registry) {
    return;
  }

  panel_registry_observations_.AddObservation(registry);

  for (const auto& entry : registry->entries()) {
    if (!panel_entry_observations_.IsObservingSource(entry.get())) {
      DVLOG(1) << "Observing existing panel entry from newly added contextual "
                  "registry : "
               << SidePanelEntryIdToString(entry->key().id());
      panel_entry_observations_.AddObservation(entry.get());
    }
  }

  SharedPinnedTabService* shared_pinned_tab_service =
      GetSharedPinnedTabService(browser_->profile());

  // When a tab is moved from another window and it has active contextual entry,
  // SidePanelCoordinator handles it and make it visible after it's moved to new
  // window. However, SidePanelCoordinator doesn't handle shared pinned tab's
  // activation because it only have interests about active tab changing. We
  // switch shared pinned tab by replacing tab. With below special handling,
  // shared pinned tab across multiple windows will have proper panel open
  // state. If per-tab side panel is opened for shared pinned tab, all other
  // windows also should have same visible side panel.
  if (shared_pinned_tab_service &&
      shared_pinned_tab_service->IsSharedContents(contents)) {
    if (auto active_entry = registry->active_entry()) {
      OnEntryShown(*active_entry);
    }
  }
}

BEGIN_METADATA(SidebarContainerView)
END_METADATA
