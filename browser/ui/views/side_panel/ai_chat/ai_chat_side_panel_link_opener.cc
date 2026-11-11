// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/views/side_panel/ai_chat/ai_chat_side_panel_link_opener.h"

#include <memory>
#include <utility>

#include "brave/components/ai_chat/core/common/features.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
#include "chrome/browser/ui/navigator/browser_navigator.h"
#include "chrome/browser/ui/navigator/browser_navigator_params.h"
#include "components/tabs/public/tab_interface.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_user_data.h"
#include "content/public/common/referrer.h"
#include "third_party/blink/public/mojom/window_features/window_features.mojom.h"
#include "ui/base/page_transition_types.h"
#include "ui/base/window_open_disposition.h"
#include "url/gurl.h"
#include "url/origin.h"

namespace ai_chat {

namespace {

// The frame that created a side panel window, stored on the new contents.
class SidePanelLinkOpener
    : public content::WebContentsUserData<SidePanelLinkOpener> {
 public:
  ~SidePanelLinkOpener() override = default;

  SidePanelLinkOpener(const SidePanelLinkOpener&) = delete;
  SidePanelLinkOpener& operator=(const SidePanelLinkOpener&) = delete;

  // Makes `params` a link navigation initiated by the opener.
  void ApplyTo(NavigateParams& params) const {
    params.initiator_origin = initiator_origin_;
    params.referrer = referrer_;
    params.is_renderer_initiated = true;
    // No `source_site_instance`, as content does for noopener windows.
  }

 private:
  friend WebContentsUserData;

  SidePanelLinkOpener(content::WebContents* new_contents,
                      content::RenderFrameHost& opener,
                      const content::Referrer& referrer)
      : WebContentsUserData(*new_contents),
        initiator_origin_(opener.GetLastCommittedOrigin()),
        referrer_(referrer) {}

  const url::Origin initiator_origin_;
  const content::Referrer referrer_;

  WEB_CONTENTS_USER_DATA_KEY_DECL();
};

WEB_CONTENTS_USER_DATA_KEY_IMPL(SidePanelLinkOpener);

}  // namespace

void RecordSidePanelLinkOpener(content::WebContents* new_contents,
                               content::RenderFrameHost* opener,
                               const content::Referrer& referrer) {
  if (!new_contents || !opener) {
    return;
  }
  // Already a tab (e.g. opened via `OpenURLFromTab`); nothing to open later.
  if (tabs::TabInterface::MaybeGetFromContents(new_contents)) {
    return;
  }
  SidePanelLinkOpener::CreateForWebContents(new_contents, *opener, referrer);
}

content::WebContents* OpenSidePanelLinkContents(
    BrowserWindowInterface* browser,
    std::unique_ptr<content::WebContents> new_contents,
    const GURL& target_url,
    const blink::mojom::WindowFeatures& window_features,
    bool user_gesture) {
  // Without a window or contents there is nowhere to open the link, so it is
  // dropped, matching how content drops a popup nobody adopts.
  if (!browser || !new_contents) {
    return nullptr;
  }

  std::unique_ptr<base::SupportsUserData::Data> opener_data =
      new_contents->TakeUserData(SidePanelLinkOpener::UserDataKey());
  const auto* opener = static_cast<SidePanelLinkOpener*>(opener_data.get());

  const bool navigate_active_tab =
      features::IsAIChatGlobalSidePanelEverywhereEnabled();

  // Prefer keeping the renderer-created contents so content navigates it as
  // usual. Not if it has an opener, which would expose the AI Chat frame.
  if (!navigate_active_tab && !new_contents->HasOpener()) {
    NavigateParams params(browser, std::move(new_contents));
    params.url = target_url;
    params.disposition = WindowOpenDisposition::kNewForegroundTab;
    params.window_features = window_features;
    params.window_action = NavigateParams::WindowAction::kNoAction;
    // Mirrors `chrome::AddWebContents()`.
    params.user_gesture = true;
    params.original_user_gesture = user_gesture;
    Navigate(&params);
    return params.navigated_or_inserted_contents;
  }

  // Re-issue on the opener's behalf, or not at all.
  if (!opener) {
    return nullptr;
  }

  NavigateParams params(browser, target_url, ui::PAGE_TRANSITION_LINK);
  opener->ApplyTo(params);
  if (navigate_active_tab) {
    tabs::TabInterface* active_tab = browser->GetActiveTabInterface();
    // No tab to navigate next to the panel (e.g. the window is closing), and
    // opening a new tab instead would defy the global panel's behavior.
    if (!active_tab) {
      return nullptr;
    }
    params.source_contents = active_tab->GetContents();
    params.disposition = WindowOpenDisposition::kCurrentTab;
  } else {
    params.disposition = WindowOpenDisposition::kNewForegroundTab;
  }
  params.window_action = NavigateParams::WindowAction::kNoAction;
  params.user_gesture = user_gesture;
  Navigate(&params);
  return params.navigated_or_inserted_contents;
}

}  // namespace ai_chat
