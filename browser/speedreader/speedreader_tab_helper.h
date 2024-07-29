/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_SPEEDREADER_SPEEDREADER_TAB_HELPER_H_
#define BRAVE_BROWSER_SPEEDREADER_SPEEDREADER_TAB_HELPER_H_

#include <memory>
#include <string>

#include "base/memory/weak_ptr.h"
#include "base/scoped_observation.h"
#include "brave/browser/speedreader/page_distiller.h"
#include "brave/components/speedreader/common/speedreader.mojom.h"
#include "brave/components/speedreader/common/speedreader_toolbar.mojom.h"
#include "brave/components/speedreader/speedreader_delegate.h"
#include "brave/components/speedreader/speedreader_service.h"
#include "brave/components/speedreader/speedreader_util.h"
#include "brave/components/speedreader/tts_player.h"
#include "components/dom_distiller/content/browser/distillable_page_utils.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"
#include "mojo/public/cpp/bindings/associated_receiver.h"

namespace content {
class NavigationEntry;
class NavigationHandle;
class WebContents;
}  // namespace content

#if BUILDFLAG(IS_ANDROID)
namespace ui {
class ViewAndroid;
}
#endif

namespace speedreader {

class SpeedreaderBubbleView;

enum class SpeedreaderBubbleLocation : int {
  kLocationBar,
  kToolbar,
};

// Determines if speedreader should be active for a given top-level navigation.
class SpeedreaderTabHelper
    : public content::WebContentsObserver,
      public content::WebContentsUserData<SpeedreaderTabHelper>,
      public PageDistiller,
      public SpeedreaderDelegate,
      public mojom::SpeedreaderHost,
      public SpeedreaderService::Observer,
      public dom_distiller::DistillabilityObserver,
      public speedreader::TtsPlayer::Observer {
 public:
  struct Observer : public base::CheckedObserver {
    ~Observer() override = default;

    virtual void OnTuneBubbleClosed() {}
    virtual void OnContentsReady() {}
  };

  ~SpeedreaderTabHelper() override;

  SpeedreaderTabHelper(const SpeedreaderTabHelper&) = delete;
  SpeedreaderTabHelper& operator=(SpeedreaderTabHelper&) = delete;

  // Provides endpoint for renderer to call browser's fucntion. (f.e
  // OnShowOriginalPage)
  static void BindSpeedreaderHost(
      mojo::PendingAssociatedReceiver<mojom::SpeedreaderHost> receiver,
      content::RenderFrameHost* rfh);

  static void MaybeCreateForWebContents(content::WebContents* contents);

  static PageDistiller* GetPageDistiller(content::WebContents* contents);

  base::WeakPtr<SpeedreaderTabHelper> GetWeakPtr();

  // In Speedreader mode shows bubble. In Reader mode toggles state.
  void ProcessIconClick();

  DistillState PageDistillState() const { return distill_state_; }

  // returns nullptr if no bubble currently shown
  SpeedreaderBubbleView* speedreader_bubble_view() const;

  // Displays speedreader information
  void ShowSpeedreaderBubble(SpeedreaderBubbleLocation location);

  // Hides speedreader information
  void HideSpeedreaderBubble();

  // Handler for when the bubble is dismissed.
  void OnBubbleClosed();

  void AddObserver(Observer* observer);
  void RemoveObserver(Observer* observer);

  // mojom::SpeedreaderHost:
  void OnShowOriginalPage() override;
  void OnTtsPlayPause(int index) override;

  void OnToolbarStateChanged(mojom::MainButtonType button);

  void ForceUIUpdate();

 private:
  friend class content::WebContentsUserData<SpeedreaderTabHelper>;
  explicit SpeedreaderTabHelper(content::WebContents* web_contents,
                                SpeedreaderRewriterService* rewriter_service);

  void BindReceiver(
      mojo::PendingAssociatedReceiver<mojom::SpeedreaderHost> receiver);

  bool MaybeUpdateCachedState(content::NavigationHandle* handle);

  void ClearPersistedData();
  void ReloadContents();

  // Applies the distill state & updates UI for the navigation.
  void ProcessNavigation(content::NavigationHandle* navigation_handle,
                         bool finish_navigation = false);

  // Updates UI if the tab is visible.
  void UpdateUI();

  // content::WebContentsObserver:
  void ReadyToCommitNavigation(
      content::NavigationHandle* navigation_handle) override;
  void DidStartNavigation(
      content::NavigationHandle* navigation_handle) override;
  void DidRedirectNavigation(
      content::NavigationHandle* navigation_handle) override;
  void DidFinishNavigation(
      content::NavigationHandle* navigation_handle) override;
  void DidStopLoading() override;
  void DOMContentLoaded(content::RenderFrameHost* render_frame_host) override;
  void OnVisibilityChanged(content::Visibility visibility) override;
  void WebContentsDestroyed() override;

  // SpeedreaderDelegate:
  bool IsPageDistillationAllowed() override;
  bool IsPageContentPresent() override;
  std::string TakePageContent() override;
  void OnDistillComplete(DistillationResult result) override;
  void OnDistilledDocumentSent() override;

  // speedreader::TtsPlayer::Observer:
  void OnReadingStart(content::WebContents* web_contents) override;
  void OnReadingStop(content::WebContents* web_contents) override;
  void OnReadingProgress(content::WebContents* web_contents,
                         int paragraph_index,
                         int char_index,
                         int length) override;

  // SpeedreaderService::Observer:
  void OnSiteEnableSettingChanged(content::WebContents* site,
                                  bool enabled_on_site) override;
  void OnAllSitesEnableSettingChanged(bool enabled_on_all_sites) override;
  void OnAppearanceSettingsChanged(
      const mojom::AppearanceSettings& view_settings) override;

  // dom_distiller::DistillabilityObserver:
  void OnResult(const dom_distiller::DistillabilityResult& result) override;

  void SetDocumentAttribute(const std::string& attribute,
                            const std::string& value);

  void OnGetDocumentSource(bool success, std::string html);

  SpeedreaderService* GetSpeedreaderService();

  void TransitStateTo(const DistillState& desired_state,
                      bool no_reload = false);

#if BUILDFLAG(IS_ANDROID)
  friend class ui::ViewAndroid;
  // Non-JNI equivalent of ui::EventForwarder::OnGestureEvent
  bool SendGestureEvent(ui::ViewAndroid* view,
                        int type,
                        int64_t time_ms,
                        float scale);
#endif
  bool is_visible_ = false;

  std::string single_show_content_;

  DistillState distill_state_{DistillStates::ViewOriginal()};

  const raw_ptr<SpeedreaderRewriterService> rewriter_service_ =
      nullptr;  // NOT OWNED
  raw_ptr<SpeedreaderBubbleView> speedreader_bubble_ = nullptr;

  mojo::AssociatedReceiver<mojom::SpeedreaderHost> receiver_{this};

  base::ScopedObservation<speedreader::TtsPlayer,
                          speedreader::TtsPlayer::Observer>
      tts_player_observation_{this};

  base::ScopedObservation<SpeedreaderService, SpeedreaderService::Observer>
      speedreader_service_observation_{this};

  base::ObserverList<Observer> observers_;

  base::WeakPtrFactory<SpeedreaderTabHelper> weak_factory_{this};

  WEB_CONTENTS_USER_DATA_KEY_DECL();
};

}  // namespace speedreader

#endif  // BRAVE_BROWSER_SPEEDREADER_SPEEDREADER_TAB_HELPER_H_
