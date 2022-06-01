/* Copyright 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/frame/brave_browser_view.h"

#include <utility>

#include "base/bind.h"
#include "brave/browser/sparkle_buildflags.h"
#include "brave/browser/ui/brave_browser.h"
#include "brave/browser/ui/views/brave_actions/brave_actions_container.h"
#include "brave/browser/ui/views/brave_actions/brave_shields_action_view.h"
#include "brave/browser/ui/views/location_bar/brave_location_bar_view.h"
#include "brave/browser/ui/views/toolbar/bookmark_button.h"
#include "brave/browser/ui/views/toolbar/brave_toolbar_view.h"
#include "brave/browser/ui/views/toolbar/wallet_button.h"
#include "brave/browser/ui/views/window_closing_confirm_dialog_view.h"
#include "brave/components/constants/pref_names.h"
#include "brave/components/speedreader/buildflags.h"
#include "brave/components/translate/core/common/buildflags.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/browser/ui/frame/window_frame_util.h"
#include "chrome/browser/ui/views/frame/tab_strip_region_view.h"
#include "chrome/browser/ui/views/tabs/tab_search_button.h"
#include "chrome/browser/ui/views/toolbar/browser_app_menu_button.h"
#include "extensions/buildflags/buildflags.h"
#include "third_party/abseil-cpp/absl/types/optional.h"
#include "ui/events/event_observer.h"
#include "ui/views/bubble/bubble_dialog_delegate_view.h"
#include "ui/views/event_monitor.h"

#if BUILDFLAG(ENABLE_BRAVE_VPN)
#include "brave/browser/ui/views/toolbar/brave_vpn_button.h"
#include "brave/components/brave_vpn/pref_names.h"
#endif

#if BUILDFLAG(ENABLE_SIDEBAR)
#include "brave/browser/ui/sidebar/sidebar_utils.h"
#include "brave/browser/ui/views/frame/brave_contents_layout_manager.h"
#include "brave/browser/ui/views/sidebar/sidebar_container_view.h"
#include "chrome/browser/ui/views/frame/contents_layout_manager.h"
#include "ui/views/layout/fill_layout.h"
#endif

#if BUILDFLAG(ENABLE_SPARKLE)
#include "brave/browser/ui/views/update_recommended_message_box_mac.h"
#endif

#if BUILDFLAG(ENABLE_SPEEDREADER)
#include "brave/browser/speedreader/speedreader_tab_helper.h"
#include "brave/browser/ui/speedreader/speedreader_bubble_view.h"
#include "brave/browser/ui/views/speedreader/reader_mode_bubble.h"
#include "brave/browser/ui/views/speedreader/speedreader_mode_bubble.h"
#include "chrome/browser/ui/views/location_bar/location_bar_bubble_delegate_view.h"
#endif

#if BUILDFLAG(ENABLE_BRAVE_TRANSLATE_EXTENSION) || \
    BUILDFLAG(ENABLE_BRAVE_TRANSLATE_GO)
#include "brave/browser/translate/brave_translate_utils.h"
#endif

namespace {
absl::optional<bool> g_download_confirm_return_allow_for_testing;
}  // namespace

// static
void BraveBrowserView::SetDownloadConfirmReturnForTesting(bool allow) {
  g_download_confirm_return_allow_for_testing = allow;
}

class BraveBrowserView::TabCyclingEventHandler : public ui::EventObserver,
                                                 public views::WidgetObserver {
 public:
  explicit TabCyclingEventHandler(BraveBrowserView* browser_view)
      : browser_view_(browser_view) {
    Start();
  }

  ~TabCyclingEventHandler() override {
    Stop();
  }

  TabCyclingEventHandler(const TabCyclingEventHandler&) = delete;
  TabCyclingEventHandler& operator=(const TabCyclingEventHandler&) = delete;

 private:
  // ui::EventObserver overrides:
  void OnEvent(const ui::Event& event) override {
    if (event.type() == ui::ET_KEY_RELEASED &&
        event.AsKeyEvent()->key_code() == ui::VKEY_CONTROL) {
      // Ctrl key was released, stop the tab cycling
      Stop();
      return;
    }

    if (event.type() == ui::ET_MOUSE_PRESSED)
      Stop();
  }

  // views::WidgetObserver overrides:
  void OnWidgetActivationChanged(views::Widget* widget, bool active) override {
    // We should stop cycling if other application gets active state.
    if (!active)
      Stop();
  }

  // Handle Browser widget closing while tab Cycling is in-progress.
  void OnWidgetClosing(views::Widget* widget) override {
    Stop();
  }

  void Start() {
    // Add the event handler
    auto* widget = browser_view_->GetWidget();
    if (widget->GetNativeWindow()) {
      monitor_ = views::EventMonitor::CreateWindowMonitor(
          this,
          widget->GetNativeWindow(),
          {ui::ET_MOUSE_PRESSED, ui::ET_KEY_RELEASED});
    }

    widget->AddObserver(this);
  }

  void Stop() {
    if (!monitor_.get())
      // We already stopped
      return;

    // Remove event handler
    auto* widget = browser_view_->GetWidget();
    monitor_.reset();
    widget->RemoveObserver(this);
    browser_view_->StopTabCycling();
  }

  raw_ptr<BraveBrowserView> browser_view_ = nullptr;
  std::unique_ptr<views::EventMonitor> monitor_;
};

BraveBrowserView::BraveBrowserView(std::unique_ptr<Browser> browser)
    : BrowserView(std::move(browser)) {
  pref_change_registrar_.Init(GetProfile()->GetPrefs());
  if (!WindowFrameUtil::IsWin10TabSearchCaptionButtonEnabled(browser_.get())) {
    pref_change_registrar_.Add(
        kTabsSearchShow,
        base::BindRepeating(&BraveBrowserView::OnPreferenceChanged,
                            base::Unretained(this)));
    // Show the correct value in settings on initial start
    UpdateSearchTabsButtonState();
  }

#if BUILDFLAG(ENABLE_BRAVE_VPN)
  pref_change_registrar_.Add(
      brave_vpn::prefs::kBraveVPNShowButton,
      base::BindRepeating(&BraveBrowserView::OnPreferenceChanged,
                          base::Unretained(this)));
#endif

#if BUILDFLAG(ENABLE_SIDEBAR)
  // Only normal window (tabbed) should have sidebar.
  if (!sidebar::CanUseSidebar(browser_.get())) {
    return;
  }

  auto brave_contents_container = std::make_unique<views::View>();

  // Wrap |contents_container_| within our new |brave_contents_container_|.
  // |brave_contents_container_| also contains sidebar.
  auto orignal_contents_container = RemoveChildViewT(contents_container_.get());
  sidebar_container_view_ = brave_contents_container->AddChildView(
      std::make_unique<SidebarContainerView>(GetBraveBrowser()));
  original_contents_container_ = brave_contents_container->AddChildView(
      std::move(orignal_contents_container));
  brave_contents_container->SetLayoutManager(
      std::make_unique<BraveContentsLayoutManager>(
          sidebar_container_view_, original_contents_container_));
  contents_container_ = AddChildView(std::move(brave_contents_container));
  set_contents_view(contents_container_);

  sidebar_host_view_ = AddChildView(std::make_unique<views::View>());

  // Make sure |find_bar_host_view_| is the last child of BrowserView by
  // re-ordering. FindBarHost widgets uses this view as a  kHostViewKey.
  // See the comments of BrowserView::find_bar_host_view().
  ReorderChildView(find_bar_host_view_, -1);
#endif
}

void BraveBrowserView::OnPreferenceChanged(const std::string& pref_name) {
  if (pref_name == kTabsSearchShow) {
    UpdateSearchTabsButtonState();
    return;
  }

#if BUILDFLAG(ENABLE_BRAVE_VPN)
  if (pref_name == brave_vpn::prefs::kBraveVPNShowButton) {
    vpn_panel_controller_.ResetBubbleManager();
    return;
  }
#endif
}

void BraveBrowserView::UpdateSearchTabsButtonState() {
  if (auto* button = tab_strip_region_view()->tab_search_button()) {
    if (button) {
      auto is_tab_search_visible =
          GetProfile()->GetPrefs()->GetBoolean(kTabsSearchShow);
      button->SetVisible(is_tab_search_visible);
    }
  }
}

BraveBrowserView::~BraveBrowserView() {
  tab_cycling_event_handler_.reset();
  DCHECK(!tab_cycling_event_handler_);
}

#if BUILDFLAG(ENABLE_SIDEBAR)
sidebar::Sidebar* BraveBrowserView::InitSidebar() {
  // Start Sidebar UI initialization.
  DCHECK(sidebar_container_view_);
  sidebar_container_view_->Init();
  return sidebar_container_view_;
}

ContentsLayoutManager* BraveBrowserView::GetContentsLayoutManager() const {
  if (sidebar::CanUseSidebar(browser_.get())) {
    return static_cast<ContentsLayoutManager*>(
        original_contents_container_->GetLayoutManager());
  }

  return BrowserView::GetContentsLayoutManager();
}
#endif

void BraveBrowserView::ShowBraveVPNBubble() {
#if BUILDFLAG(ENABLE_BRAVE_VPN)
  vpn_panel_controller_.ShowBraveVPNPanel();
#endif
}

views::View* BraveBrowserView::GetAnchorViewForBraveVPNPanel() {
#if BUILDFLAG(ENABLE_BRAVE_VPN)
  auto* vpn_button =
      static_cast<BraveToolbarView*>(toolbar())->brave_vpn_button();
  if (vpn_button->GetVisible())
    return vpn_button;
  return toolbar()->app_menu_button();
#else
  return nullptr;
#endif
}

gfx::Rect BraveBrowserView::GetShieldsBubbleRect() {
  auto* brave_location_bar_view =
      static_cast<BraveLocationBarView*>(GetLocationBarView());
  if (!brave_location_bar_view)
    return gfx::Rect();

  auto* shields_action_view =
      brave_location_bar_view->brave_actions_contatiner_view()
          ->GetShieldsActionView();
  if (!shields_action_view)
    return gfx::Rect();

  auto* bubble_widget = shields_action_view->GetBubbleWidget();
  if (!bubble_widget)
    return gfx::Rect();

  return bubble_widget->GetClientAreaBoundsInScreen();
}

void BraveBrowserView::SetStarredState(bool is_starred) {
  BookmarkButton* button =
      static_cast<BraveToolbarView*>(toolbar())->bookmark_button();
  if (button)
    button->SetToggled(is_starred);
}

void BraveBrowserView::ShowUpdateChromeDialog() {
#if BUILDFLAG(ENABLE_SPARKLE)
  // On mac, sparkle frameworks's relaunch api is used.
  UpdateRecommendedMessageBoxMac::Show(GetNativeWindow());
#else
  BrowserView::ShowUpdateChromeDialog();
#endif
}

// The translate bubble will be shown if ENABLE_BRAVE_TRANSLATE_GO or
// ENABLE_BRAVE_TRANSLATE_EXTENSIONS build flag is enabled and Google Translate
// is not installed. In ENABLE_BRAVE_TRANSLATE case, we utilize chromium's
// translate UI directly along with go-translate. In
// ENABLE_BRAVE_TRANSLATE_EXTENSION case, we repurpose the translate bubble to
// offer Google Translate extension installation, and the bubble will only be
// shown when Google Translate is not installed.
ShowTranslateBubbleResult BraveBrowserView::ShowTranslateBubble(
    content::WebContents* web_contents,
    translate::TranslateStep step,
    const std::string& source_language,
    const std::string& target_language,
    translate::TranslateErrors::Type error_type,
    bool is_user_gesture) {
#if BUILDFLAG(ENABLE_BRAVE_TRANSLATE_EXTENSION) || \
    BUILDFLAG(ENABLE_BRAVE_TRANSLATE_GO)
  if (translate::ShouldOfferExtensionInstallation(GetProfile()) ||
      translate::IsInternalTranslationEnabled(GetProfile())) {
    return BrowserView::ShowTranslateBubble(web_contents, step, source_language,
                                            target_language, error_type,
                                            is_user_gesture);
  }
#endif  // BUILDFLAG(ENABLE_BRAVE_TRANSLATE_EXTENSION) ||
        // BUILDFLAG(ENABLE_BRAVE_TRANSLATE_GO)
  return ShowTranslateBubbleResult::BROWSER_WINDOW_NOT_VALID;
}

speedreader::SpeedreaderBubbleView* BraveBrowserView::ShowSpeedreaderBubble(
    speedreader::SpeedreaderTabHelper* tab_helper,
    bool is_enabled) {
#if BUILDFLAG(ENABLE_SPEEDREADER)
  speedreader::SpeedreaderBubbleView* bubble = nullptr;
  if (is_enabled) {
    auto* speedreader_mode_bubble = new speedreader::SpeedreaderModeBubble(
        GetLocationBarView(), tab_helper);
    views::BubbleDialogDelegateView::CreateBubble(speedreader_mode_bubble);
    bubble = speedreader_mode_bubble;
  } else {
    auto* reader_mode_bubble =
        new speedreader::ReaderModeBubble(GetLocationBarView(), tab_helper);
    views::BubbleDialogDelegateView::CreateBubble(reader_mode_bubble);
    bubble = reader_mode_bubble;
  }

  bubble->Show();

  return bubble;
#else
  return nullptr;
#endif
}

WalletButton* BraveBrowserView::GetWalletButton() {
  return static_cast<BraveToolbarView*>(toolbar())->wallet_button();
}

views::View* BraveBrowserView::GetWalletButtonAnchorView() {
  return static_cast<BraveToolbarView*>(toolbar())
      ->wallet_button()
      ->GetAsAnchorView();
}

void BraveBrowserView::CreateWalletBubble() {
  DCHECK(GetWalletButton());
  GetWalletButton()->ShowWalletBubble();
}

void BraveBrowserView::CreateApproveWalletBubble() {
  DCHECK(GetWalletButton());
  GetWalletButton()->ShowApproveWalletBubble();
}

void BraveBrowserView::CloseWalletBubble() {
  if (GetWalletButton())
    GetWalletButton()->CloseWalletBubble();
}

void BraveBrowserView::OnTabStripModelChanged(
    TabStripModel* tab_strip_model,
    const TabStripModelChange& change,
    const TabStripSelectionChange& selection) {
  BrowserView::OnTabStripModelChanged(tab_strip_model, change, selection);

  if (change.type() != TabStripModelChange::kSelectionOnly) {
    // Stop tab cycling if tab is closed dusing the cycle.
    // This can happen when tab is closed by shortcut (ex, ctrl + F4).
    // After stopping, current tab cycling, new tab cycling will be started.
    StopTabCycling();
  }
}

views::CloseRequestResult BraveBrowserView::OnWindowCloseRequested() {
  if (GetBraveBrowser()->ShouldAskForBrowserClosingBeforeHandlers()) {
    WindowClosingConfirmDialogView::Show(
        browser(),
        base::BindOnce(&BraveBrowserView::OnWindowClosingConfirmResponse,
                       weak_ptr_.GetWeakPtr()));
    return views::CloseRequestResult::kCannotClose;
  }

  return BrowserView::OnWindowCloseRequested();
}

void BraveBrowserView::OnWindowClosingConfirmResponse(bool allowed_to_close) {
  auto* browser = GetBraveBrowser();
  // Set to Browser instance because Browser instance knows about the result
  // of any warning handlers or beforeunload handlers.
  browser->set_confirmed_to_close(allowed_to_close);
  if (allowed_to_close) {
    // Start close window again as user allowed to close it.
    // Confirm dialog will not be launched for this closing request
    // as we set BraveBrowser::confirmed_to_closed_window_ to true.
    // If user cancels this window closing via additional warnings
    // or beforeunload handler, this dialog will be shown again.
    chrome::CloseWindow(browser);
  }
}

void BraveBrowserView::ConfirmBrowserCloseWithPendingDownloads(
    int download_count,
    Browser::DownloadCloseType dialog_type,
    base::OnceCallback<void(bool)> callback) {
  // Simulate user response.
  if (g_download_confirm_return_allow_for_testing) {
    base::SequencedTaskRunnerHandle::Get()->PostTask(
        FROM_HERE,
        base::BindOnce(std::move(callback),
                       *g_download_confirm_return_allow_for_testing));
    return;
  }
  BrowserView::ConfirmBrowserCloseWithPendingDownloads(
      download_count, dialog_type, std::move(callback));
}

void BraveBrowserView::MaybeShowReadingListInSidePanelIPH() {
  // Do nothing.
}

BraveBrowser* BraveBrowserView::GetBraveBrowser() const {
  return static_cast<BraveBrowser*>(browser_.get());
}

void BraveBrowserView::StartTabCycling() {
  tab_cycling_event_handler_ = std::make_unique<TabCyclingEventHandler>(this);
}

void BraveBrowserView::StopTabCycling() {
  tab_cycling_event_handler_.reset();
  static_cast<BraveTabStripModel*>(browser()->tab_strip_model())->
      StopMRUCycling();
}
