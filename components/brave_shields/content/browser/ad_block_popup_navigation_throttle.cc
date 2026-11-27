// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_shields/content/browser/ad_block_popup_navigation_throttle.h"

#include <memory>
#include <optional>
#include <utility>

#include "base/functional/bind.h"
#include "base/task/single_thread_task_runner.h"
#include "base/time/clock.h"
#include "base/time/time.h"
#include "brave/components/brave_shields/core/browser/ad_block_engine_wrapper.h"
#include "brave/components/brave_shields/core/browser/ad_block_request_type.h"
#include "brave/components/brave_shields/core/browser/ad_block_service.h"
#include "brave/components/brave_shields/core/browser/brave_shields_settings_service.h"
#include "brave/components/brave_shields/core/common/shields_settings.mojom.h"
#include "content/public/browser/navigation_controller.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_user_data.h"
#include "ui/base/window_open_disposition.h"
#include "url/gurl.h"

namespace brave_shields {

namespace {

// How long after its last commit a window's navigations are still checked.
// Same as uBO.
constexpr base::TimeDelta kCandidateLifetime = base::Seconds(10);

const base::Clock* g_clock_for_testing = nullptr;

// Kept for a window a page opened.
class PopupState : public content::WebContentsUserData<PopupState> {
 public:
  struct Opener {
    std::optional<content::GlobalRenderFrameHostToken> frame_token;
    url::Origin origin;
    // The opener's top-level page, which its Shields settings are read for.
    // An opaque top-level origin has no URL, so it gets the default settings.
    GURL settings_url;
  };

  ~PopupState() override = default;

  // The frame that opened the window, as it was then.
  std::optional<Opener> opener;
  // Like uBO, a window that stops being a candidate never is one again.
  bool expired = false;

 private:
  friend WebContentsUserData;

  explicit PopupState(content::WebContents* web_contents)
      : content::WebContentsUserData<PopupState>(*web_contents) {}

  WEB_CONTENTS_USER_DATA_KEY_DECL();
};

WEB_CONTENTS_USER_DATA_KEY_IMPL(PopupState);

// A blank window's initial about:blank commit has a timestamp, so it's a
// candidate from when it opens.
bool IsCandidate(content::WebContents& popup) {
  const base::Time committed =
      popup.GetController().GetLastCommittedEntry()->GetTimestamp();

  if (committed.is_null()) {
    return true;
  }

  const base::Time now =
      g_clock_for_testing ? g_clock_for_testing->Now() : base::Time::Now();

  return now - committed <= kCandidateLifetime;
}

// Returns the opener that a popup's navigation is checked for, or nullopt if
// it isn't checked.
std::optional<AdBlockPopupOpener> GetOpener(
    content::NavigationHandle& handle,
    BraveShieldsSettingsService& settings_service) {
  content::WebContents* popup = handle.GetWebContents();
  std::optional<content::GlobalRenderFrameHostToken> initiator_token;
  content::RenderFrameHost* initiator = nullptr;

  if (handle.GetInitiatorFrameToken()) {
    initiator_token.emplace(handle.GetInitiatorProcessId(),
                            *handle.GetInitiatorFrameToken());
    initiator = content::RenderFrameHost::FromFrameToken(*initiator_token);
  }

  PopupState* state = PopupState::GetOrCreateForWebContents(popup);
  PopupState::Opener opener;

  if (initiator_token && handle.GetInitiatorOrigin() &&
      (!initiator ||
       content::WebContents::FromRenderFrameHost(initiator) != popup)) {
    // Another window started the navigation, as the opener does for the
    // popup's first one.
    opener.frame_token = *initiator_token;
    opener.origin = *handle.GetInitiatorOrigin();

    if (initiator) {
      opener.settings_url =
          initiator->GetOutermostMainFrame()->GetLastCommittedOrigin().GetURL();
    }
  } else if (state->opener) {
    // The popup navigated itself, so it's checked against the frame that
    // opened it, whatever that frame's tab shows now.
    opener = *state->opener;
  } else {
    // A blank window navigated itself before anything else navigated it. Its
    // document has the origin of the frame that opened it.
    if (!handle.GetInitiatorOrigin()) {
      return std::nullopt;
    }

    opener.origin = *handle.GetInitiatorOrigin();

    // The opener frame is only used if it still has that origin. If it has
    // navigated elsewhere, or the page cleared `window.opener`, the default
    // settings apply and the popup isn't listed.
    content::RenderFrameHost* opener_frame = popup->GetOpener();

    if (opener_frame &&
        opener_frame->GetLastCommittedOrigin() == opener.origin) {
      opener.frame_token = opener_frame->GetGlobalFrameToken();
      opener.settings_url = opener_frame->GetOutermostMainFrame()
                                ->GetLastCommittedOrigin()
                                .GetURL();
    }
  }

  if (!state->opener) {
    state->opener = opener;
  }

  if (!settings_service.IsBraveShieldsEnabled(opener.settings_url)) {
    return std::nullopt;
  }

  const mojom::AdBlockMode ad_block_mode =
      settings_service.GetAdBlockMode(opener.settings_url);

  if (ad_block_mode == mojom::AdBlockMode::ALLOW) {
    return std::nullopt;
  }

  return AdBlockPopupOpener{
      .frame_token = opener.frame_token,
      .origin = opener.origin,
      .aggressive = ad_block_mode == mojom::AdBlockMode::AGGRESSIVE,
  };
}

bool ShouldBlockOnTaskRunner(const GURL& url,
                             const url::Origin& source,
                             bool aggressive,
                             AdBlockEngineWrapper* engine_wrapper) {
  auto result = engine_wrapper->ShouldStartRequest(
      url, AdBlockRequestType::kPopup, source, "GET", aggressive, false, false,
      false);
  return result.important || (result.matched && !result.has_exception);
}

}  // namespace

// static
void AdBlockPopupNavigationThrottle::MaybeCreateAndAdd(
    content::NavigationThrottleRegistry& registry,
    AdBlockService* ad_block_service,
    BraveShieldsSettingsService* settings_service,
    std::unique_ptr<Delegate> delegate) {
  if (!ad_block_service || !settings_service) {
    return;
  }

  content::NavigationHandle& handle = registry.GetNavigationHandle();

  if (!handle.IsInPrimaryMainFrame()) {
    return;
  }

  // Only a window a page opens has an original disposition. A middle-click or
  // cmd-click tab doesn't.
  content::WebContents* popup = handle.GetWebContents();

  if (popup->GetOriginalWindowOpenDisposition() ==
      WindowOpenDisposition::kUnknown) {
    return;
  }

  PopupState* state = PopupState::GetOrCreateForWebContents(popup);

  if (state->expired) {
    return;
  }

  if (!IsCandidate(*popup)) {
    state->expired = true;
    return;
  }

  // After the expiry check, so a navigation the user starts can still expire
  // the window.
  if (!handle.IsRendererInitiated()) {
    return;
  }

  std::optional<AdBlockPopupOpener> opener =
      GetOpener(handle, *settings_service);

  if (!opener) {
    return;
  }

  registry.AddThrottle(std::make_unique<AdBlockPopupNavigationThrottle>(
      registry, ad_block_service, std::move(*opener), std::move(delegate)));
}

// static
void AdBlockPopupNavigationThrottle::SetClockForTesting(
    const base::Clock* clock) {
  g_clock_for_testing = clock;
}

AdBlockPopupNavigationThrottle::AdBlockPopupNavigationThrottle(
    content::NavigationThrottleRegistry& registry,
    AdBlockService* ad_block_service,
    AdBlockPopupOpener opener,
    std::unique_ptr<Delegate> delegate)
    : content::NavigationThrottle(registry),
      opener_(std::move(opener)),
      delegate_(std::move(delegate)),
      ad_block_service_(ad_block_service) {}

AdBlockPopupNavigationThrottle::~AdBlockPopupNavigationThrottle() = default;

content::NavigationThrottle::ThrottleCheckResult
AdBlockPopupNavigationThrottle::WillStartRequest() {
  const GURL& url = navigation_handle()->GetURL();

  if (!url.SchemeIsHTTPOrHTTPS()) {
    return PROCEED;
  }

  ad_block_service_->AsyncCallAndReplyWithResult<bool>(
      base::BindOnce(&ShouldBlockOnTaskRunner, url, opener_.origin,
                     opener_.aggressive),
      base::BindOnce(&AdBlockPopupNavigationThrottle::OnShouldBlock,
                     weak_ptr_factory_.GetWeakPtr()));
  return DEFER;
}

content::NavigationThrottle::ThrottleCheckResult
AdBlockPopupNavigationThrottle::WillRedirectRequest() {
  return WillStartRequest();
}

void AdBlockPopupNavigationThrottle::OnShouldBlock(bool should_block) {
  if (!should_block) {
    Resume();
    return;
  }

  content::NavigationHandle* handle = navigation_handle();
  delegate_->OnPopupBlocked(opener_, *handle);

  // Closing the window destroys this throttle, so it's posted.
  base::SingleThreadTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE, base::BindOnce(&content::WebContents::Close,
                                handle->GetWebContents()->GetWeakPtr()));
  // Deletes `this`.
  CancelDeferredNavigation(CANCEL_AND_IGNORE);
}

const char* AdBlockPopupNavigationThrottle::GetNameForLogging() {
  return "AdBlockPopupNavigationThrottle";
}

}  // namespace brave_shields
