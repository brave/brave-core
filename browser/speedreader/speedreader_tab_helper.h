/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_SPEEDREADER_SPEEDREADER_TAB_HELPER_H_
#define BRAVE_BROWSER_SPEEDREADER_SPEEDREADER_TAB_HELPER_H_

#include <memory>
#include <string>

#include "base/memory/weak_ptr.h"
#include "brave/browser/ui/webui/speedreader/speedreader_panel_ui.h"
#include "brave/components/speedreader/common/speedreader.mojom.h"
#include "brave/components/speedreader/speedreader_result_delegate.h"
#include "brave/components/speedreader/speedreader_util.h"
#include "chrome/browser/ui/views/bubble/webui_bubble_manager.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"
#include "mojo/public/cpp/bindings/associated_receiver.h"

class PrefChangeRegistrar;
namespace content {
class NavigationEntry;
class NavigationHandle;
class WebContents;
}  // namespace content

using speedreader::mojom::Theme;

namespace speedreader {

namespace test {
void SetShowOriginalLinkTitle(const std::u16string* title);
}

class SpeedreaderBubbleView;

// Determines if speedreader should be active for a given top-level navigation.
class SpeedreaderTabHelper
    : public content::WebContentsObserver,
      public content::WebContentsUserData<SpeedreaderTabHelper>,
      public SpeedreaderResultDelegate,
      public mojom::SpeedreaderHost {
 public:
  ~SpeedreaderTabHelper() override;

  SpeedreaderTabHelper(const SpeedreaderTabHelper&) = delete;
  SpeedreaderTabHelper& operator=(SpeedreaderTabHelper&) = delete;

  // Provides endpoint for renderer to call browser's fucntion. (f.e
  // OnShowOriginalPage)
  static void BindSpeedreaderHost(
      mojo::PendingAssociatedReceiver<mojom::SpeedreaderHost> receiver,
      content::RenderFrameHost* rfh);

  static void MaybeCreateForWebContents(content::WebContents* contents);

  base::WeakPtr<SpeedreaderTabHelper> GetWeakPtr();

  // Returns |true| if Speedreader is turned on for all sites.
  bool IsSpeedreaderEnabled() const;

  // Returns |true| if the user has enabled Speedreader but the domain in the
  // active web contents is blacklisted.
  bool IsEnabledForSite();

  // In Speedreader mode shows bubble. In Reader mode toggles state.
  void ProcessIconClick();

  DistillState PageDistillState() const { return distill_state_; }

  // Allow or deny a site from being run through speedreader if |on| toggles
  // the setting. Triggers page reload on toggle.
  void MaybeToggleEnabledForSite(bool on);

  // Reload the page and mark the next request to run through Speedreader,
  // without turning it on. This mimics the standard reader mode.
  void SingleShotSpeedreader();

  // returns nullptr if no bubble currently shown
  SpeedreaderBubbleView* speedreader_bubble_view() const;

  // returns nullptr if no bubble currently shown
  views::BubbleDialogDelegateView* speedreader_webui_bubble_delegate_view()
      const;

  // Displays speedreader information
  void ShowSpeedreaderBubble();

  // Displays reader mode information
  void ShowReaderModeBubble();

  // Displays speedreader UI controls
  void ShowSpeedreaderWebUIBubble();

  // Hides speedreader information
  void HideBubble();

  // Handler for when the bubble is dismissed.
  void OnBubbleClosed();

  // mojom::SpeedreaderHost:
  void OnShowOriginalPage() override;

  void SetTheme(Theme theme);
  Theme GetTheme();

 private:
  friend class content::WebContentsUserData<SpeedreaderTabHelper>;
  explicit SpeedreaderTabHelper(content::WebContents* web_contents);

  void BindReceiver(
      mojo::PendingAssociatedReceiver<mojom::SpeedreaderHost> receiver);

  Profile* GetProfile() const;

  // Called by ShowSpeedreaderBubble and ShowReaderModeBubble.
  // |is_bubble_speedreader| will show a bubble for pages in Speedreader if set
  // to true, otherwise pages in reader mode.
  void ShowBubble(bool is_bubble_speedreader);

  // Returns |true| if the user has enabled Speedreader but the URL is
  // blacklisted. This method is used when the URL we want to check has not been
  // committed to the WebContents.
  bool IsEnabledForSite(const GURL& url);

  bool MaybeUpdateCachedState(content::NavigationHandle* handle);
  void UpdateActiveState(const GURL& url);
  void SetNextRequestState(DistillState state);

  void ClearPersistedData();
  void ReloadContents();

  // Applies the distill state & updates UI for the navigation.
  void ProcessNavigation(content::NavigationHandle* navigation_handle);

  // Updates the distill state when the global speedreader state is changed.
  void OnPrefChanged();

  // Updates UI if the tab is visible.
  void UpdateButtonIfNeeded();

  // content::WebContentsObserver:
  void DidStartNavigation(
      content::NavigationHandle* navigation_handle) override;
  void DidRedirectNavigation(
      content::NavigationHandle* navigation_handle) override;
  void DidStopLoading() override;
  void DOMContentLoaded(content::RenderFrameHost* render_frame_host) override;
  void OnVisibilityChanged(content::Visibility visibility) override;
  void WebContentsDestroyed() override;

  // SpeedreaderResultDelegate:
  void OnDistillComplete() override;

  std::unique_ptr<PrefChangeRegistrar> pref_change_registrar_;

  bool is_visible_ = false;

  bool single_shot_next_request_ =
      false;  // run speedreader once on next page load

  bool show_original_page_ = false;   // next request should not be distilled
  bool original_page_shown_ = false;  // true if reload was performed using the
                                      // 'show original page' link

  DistillState distill_state_ = DistillState::kNone;
  raw_ptr<SpeedreaderBubbleView> speedreader_bubble_ = nullptr;
  std::unique_ptr<WebUIBubbleManagerT<SpeedreaderPanelUI>>
      speedreader_webui_bubble_manager_;
  raw_ptr<HostContentSettingsMap> content_rules_ = nullptr;

  mojo::AssociatedReceiver<mojom::SpeedreaderHost> receiver_{this};

  base::WeakPtrFactory<SpeedreaderTabHelper> weak_factory_{this};

  WEB_CONTENTS_USER_DATA_KEY_DECL();
};

}  // namespace speedreader

#endif  // BRAVE_BROWSER_SPEEDREADER_SPEEDREADER_TAB_HELPER_H_
