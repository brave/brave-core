/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_rewards/rewards_tab_helper.h"

#include <utility>

#include "base/check.h"
#include "brave/browser/brave_rewards/rewards_service_factory.h"
#include "brave/components/brave_rewards/content/rewards_service.h"
#include "brave/components/brave_rewards/core/publisher_utils.h"
#include "chrome/browser/profiles/profile.h"
#include "components/sessions/content/session_tab_helper.h"
#include "components/tabs/public/tab_interface.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "third_party/blink/public/mojom/loader/resource_load_info.mojom.h"

namespace brave_rewards {

DEFINE_USER_DATA(RewardsTabHelper);

// static
RewardsTabHelper* RewardsTabHelper::From(tabs::TabInterface* tab) {
  return Get(tab->GetUnownedUserDataHost());
}

RewardsTabHelper::RewardsTabHelper(tabs::TabInterface& tab)
    : tabs::ContentsObservingTabFeature(tab),
      tab_id_(sessions::SessionTabHelper::IdForTab(tab.GetContents())),
      rewards_service_(RewardsServiceFactory::GetForProfile(
          Profile::FromBrowserContext(tab.GetContents()->GetBrowserContext()))),
      scoped_unowned_user_data_(tab.GetUnownedUserDataHost(), *this) {
  CHECK(tab_id_.is_valid());
  if (rewards_service_) {
    rewards_service_observation_.Observe(rewards_service_);
  }
}

RewardsTabHelper::~RewardsTabHelper() = default;

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
    creator_detection_->MaybeInjectScript(
        navigation_handle->GetRenderFrameHost());
  }

  creator_detection_->DetectCreator(
      navigation_handle->GetRenderFrameHost(),
      base::BindOnce(&RewardsTabHelper::OnCreatorDetected,
                     base::Unretained(this)));
}

void RewardsTabHelper::OnDiscardContents(tabs::TabInterface* tab,
                                         content::WebContents* old_contents,
                                         content::WebContents* new_contents) {
  // The script injector and any pending detection are for the old contents'
  // page.
  creator_detection_ = std::make_unique<CreatorDetectionScriptInjector>();
  tab_id_ = sessions::SessionTabHelper::IdForTab(new_contents);
  tabs::ContentsObservingTabFeature::OnDiscardContents(tab, old_contents,
                                                       new_contents);
  SetPublisherIdForTab("");
}

void RewardsTabHelper::OnRewardsInitialized(RewardsService* rewards_service) {
  MaybeSavePublisherInfo();
}

void RewardsTabHelper::MaybeSavePublisherInfo() {
  if (!rewards_service_) {
    return;
  }

  // The Rewards system currently assumes that the |publisher_info| table is
  // populated by calling `NotifyPublisherPageVisit` as the user navigates
  // the web.
  rewards_service_->NotifyPublisherPageVisit(
      tab_id_.id(), web_contents()->GetLastCommittedURL().spec());
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
    rewards_service_->NotifyPublisherPageVisit(visit->Clone());
  }
}

}  // namespace brave_rewards
