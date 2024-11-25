/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_rewards/rewards_tab_helper.h"

#include <utility>

#include "brave/browser/brave_rewards/rewards_service_factory.h"
#include "brave/components/brave_rewards/browser/rewards_service.h"
#include "brave/components/brave_rewards/common/publisher_utils.h"
#include "chrome/browser/profiles/profile.h"
#include "components/sessions/content/session_tab_helper.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_user_data.h"
#include "third_party/blink/public/mojom/loader/resource_load_info.mojom.h"

#if !BUILDFLAG(IS_ANDROID)
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/browser_list.h"
#include "chrome/browser/ui/browser_list_observer.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#endif

namespace brave_rewards {

#if !BUILDFLAG(IS_ANDROID)
class BraveBrowserListObserver : public BrowserListObserver {
 public:
  explicit BraveBrowserListObserver(RewardsTabHelper* tab_helper)
      : tab_helper_(tab_helper) {}
  ~BraveBrowserListObserver() override {}
  void OnBrowserSetLastActive(Browser* browser) override {
    tab_helper_->OnBrowserSetLastActive(browser);
  }
  void OnBrowserNoLongerActive(Browser* browser) override {
    tab_helper_->OnBrowserNoLongerActive(browser);
  }

 private:
  const raw_ptr<RewardsTabHelper> tab_helper_ = nullptr;  // Not owned.
};
#endif

RewardsTabHelper::RewardsTabHelper(content::WebContents* web_contents)
    : content::WebContentsUserData<RewardsTabHelper>(*web_contents),
      WebContentsObserver(web_contents),
#if !BUILDFLAG(IS_ANDROID)
      browser_list_observer_(new BraveBrowserListObserver(this)),
#endif
      tab_id_(sessions::SessionTabHelper::IdForTab(web_contents)) {
  if (tab_id_.is_valid()) {
    rewards_service_ = RewardsServiceFactory::GetForProfile(
        Profile::FromBrowserContext(GetWebContents().GetBrowserContext()));
    if (rewards_service_) {
      rewards_service_observation_.Observe(rewards_service_);
    }
  }

#if !BUILDFLAG(IS_ANDROID)
  BrowserList::AddObserver(browser_list_observer_.get());
#endif
}

RewardsTabHelper::~RewardsTabHelper() {
#if !BUILDFLAG(IS_ANDROID)
  BrowserList::RemoveObserver(browser_list_observer_.get());
#endif
}

void RewardsTabHelper::AddObserver(Observer* observer) {
  observer_list_.AddObserver(observer);
}

void RewardsTabHelper::RemoveObserver(Observer* observer) {
  observer_list_.RemoveObserver(observer);
}

void RewardsTabHelper::SetPublisherIdForTab(const std::string& publisher_id) {
  if (publisher_id != publisher_id_) {
    publisher_id_ = publisher_id;
    for (auto& observer : observer_list_) {
      observer.OnPublisherForTabUpdated(publisher_id_);
    }
  }
}

void RewardsTabHelper::DidFinishLoad(
    content::RenderFrameHost* render_frame_host,
    const GURL& validated_url) {
  if (!rewards_service_ || render_frame_host->GetParent()) {
    return;
  }

  rewards_service_->OnLoad(tab_id_, validated_url);
}

void RewardsTabHelper::DidFinishNavigation(
    content::NavigationHandle* navigation_handle) {
  if (!navigation_handle->HasCommitted() ||
      !navigation_handle->IsInPrimaryMainFrame() ||
      navigation_handle->IsDownload()) {
    return;
  }

  if (!rewards_service_) {
    return;
  }

  if (!navigation_handle->IsSameDocument()) {
    auto id = GetPublisherIdFromURL(navigation_handle->GetURL());
    SetPublisherIdForTab(id ? *id : "");
    MaybeSavePublisherInfo();
    rewards_service_->OnUnload(tab_id_);
    creator_detection_.MaybeInjectScript(
        navigation_handle->GetRenderFrameHost());
  }

  creator_detection_.DetectCreator(
      navigation_handle->GetRenderFrameHost(),
      base::BindOnce(&RewardsTabHelper::OnCreatorDetected,
                     base::Unretained(this)));
}

void RewardsTabHelper::ResourceLoadComplete(
    content::RenderFrameHost* render_frame_host,
    const content::GlobalRequestID& request_id,
    const blink::mojom::ResourceLoadInfo& resource_load_info) {
  if (!rewards_service_ || !render_frame_host) {
    return;
  }

  switch (resource_load_info.request_destination) {
    case network::mojom::RequestDestination::kAudio:
    case network::mojom::RequestDestination::kTrack:
    case network::mojom::RequestDestination::kVideo:
    case network::mojom::RequestDestination::kEmpty:
    case network::mojom::RequestDestination::kImage:
    case network::mojom::RequestDestination::kScript:
      rewards_service_->OnXHRLoad(tab_id_, GURL(resource_load_info.final_url),
                                  GetWebContents().GetVisibleURL(),
                                  resource_load_info.referrer);
      break;
    default:
      break;
  }
}

void RewardsTabHelper::OnVisibilityChanged(content::Visibility visibility) {
  if (!rewards_service_) {
    return;
  }

  if (visibility == content::Visibility::HIDDEN) {
    rewards_service_->OnHide(tab_id_);
  } else if (visibility == content::Visibility::OCCLUDED) {
    rewards_service_->OnBackground(tab_id_);
  } else if (visibility == content::Visibility::VISIBLE) {
    rewards_service_->OnShow(tab_id_);
  }
}

void RewardsTabHelper::WebContentsDestroyed() {
  if (rewards_service_) {
    rewards_service_->OnUnload(tab_id_);
  }
}

#if !BUILDFLAG(IS_ANDROID)
void RewardsTabHelper::OnBrowserSetLastActive(Browser* browser) {
  if (rewards_service_ && BrowserHasWebContents(browser)) {
    rewards_service_->OnForeground(tab_id_);
  }
}

void RewardsTabHelper::OnBrowserNoLongerActive(Browser* browser) {
  if (rewards_service_ && BrowserHasWebContents(browser)) {
    rewards_service_->OnBackground(tab_id_);
  }
}

bool RewardsTabHelper::BrowserHasWebContents(Browser* browser) {
  return chrome::FindBrowserWithTab(&GetWebContents()) == browser;
}
#endif

void RewardsTabHelper::OnRewardsInitialized(RewardsService* rewards_service) {
  MaybeSavePublisherInfo();

  // When Rewards is initialized for the current profile, we need to inform the
  // utility process about the currently active tab so that it can start
  // measuring auto-contribute correctly.
  rewards_service->OnShow(tab_id_);
  rewards_service->OnLoad(tab_id_, GetWebContents().GetLastCommittedURL());
}

void RewardsTabHelper::MaybeSavePublisherInfo() {
  if (!rewards_service_) {
    return;
  }

  // The Rewards system currently assumes that the |publisher_info| table is
  // populated by calling `GetPublisherActivityFromUrl` as the user navigates
  // the web. Previously, this was accomplished within the background script of
  // the Rewards extension.
  rewards_service_->GetPublisherActivityFromUrl(
      tab_id_.id(), GetWebContents().GetLastCommittedURL().spec(), "", "");
}

void RewardsTabHelper::OnCreatorDetected(
    std::optional<CreatorDetectionScriptInjector::Result> result) {
  if (!result) {
    return;
  }

  SetPublisherIdForTab(result->id);

  if (!result->id.empty()) {
    auto visit = mojom::VisitData::New();
    visit->tab_id = static_cast<uint32_t>(tab_id_.id());
    visit->domain = result->id;
    visit->name = result->name;
    visit->path = "";
    visit->url = result->url;
    visit->favicon_url = result->image_url;
    if (auto platform = GetMediaPlatformFromPublisherId(result->id)) {
      visit->provider = *platform;
    }

    CHECK(rewards_service_);

    // When a creator has been detected for the current tab, we must send the
    // creator data to the utility process so that the "publisher_info" database
    // table can be populated.
    // TODO(https://github.com/brave/brave-browser/issues/41832): Rename and
    // possibly refactor this API.
    rewards_service_->GetPublisherActivityFromVisitData(visit->Clone());

    // Notify the Rewards service that a "page view" has started for this
    // creator.
    rewards_service_->OnShow(tab_id_);
    rewards_service_->OnLoad(std::move(visit));
  }
}

WEB_CONTENTS_USER_DATA_KEY_IMPL(RewardsTabHelper);

}  // namespace brave_rewards
