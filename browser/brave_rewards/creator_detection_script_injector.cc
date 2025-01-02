// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/brave_rewards/creator_detection_script_injector.h"

#include <utility>

#include "base/containers/fixed_flat_map.h"
#include "base/feature_list.h"
#include "base/functional/bind.h"
#include "base/strings/strcat.h"
#include "base/strings/utf_string_conversions.h"
#include "brave/browser/brave_rewards/rewards_util.h"
#include "brave/components/brave_rewards/common/features.h"
#include "brave/components/brave_rewards/common/pref_names.h"
#include "brave/components/brave_rewards/common/publisher_utils.h"
#include "brave/components/brave_rewards/resources/grit/creator_detection_generated.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/chrome_isolated_world_ids.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/common/content_client.h"
#include "third_party/blink/public/common/associated_interfaces/associated_interface_provider.h"
#include "ui/base/resource/resource_bundle.h"

namespace brave_rewards {

namespace {

constexpr auto kScriptMap = base::MakeFixedFlatMap<std::string_view, int>(
    {{"reddit.com", IDR_CREATOR_DETECTION_REDDIT_BUNDLE_JS},
     {"www.reddit.com", IDR_CREATOR_DETECTION_REDDIT_BUNDLE_JS},
     {"twitch.tv", IDR_CREATOR_DETECTION_TWITCH_BUNDLE_JS},
     {"www.twitch.tv", IDR_CREATOR_DETECTION_TWITCH_BUNDLE_JS},
     {"twitter.com", IDR_CREATOR_DETECTION_TWITTER_BUNDLE_JS},
     {"x.com", IDR_CREATOR_DETECTION_TWITTER_BUNDLE_JS},
     {"vimeo.com", IDR_CREATOR_DETECTION_VIMEO_BUNDLE_JS},
     {"www.youtube.com", IDR_CREATOR_DETECTION_YOUTUBE_BUNDLE_JS},
     {"m.youtube.com", IDR_CREATOR_DETECTION_YOUTUBE_BUNDLE_JS}});

std::string LoadScriptResource(int id) {
  auto& bundle = ui::ResourceBundle::GetSharedInstance();
  if (bundle.IsGzipped(id)) {
    return bundle.LoadDataResourceString(id);
  }
  return std::string(bundle.GetRawDataResource(id));
}

std::optional<std::string> GetDetectionScript(content::RenderFrameHost* rfh) {
  // Only run scripts for the main frame.
  if (!rfh || !rfh->IsInPrimaryMainFrame()) {
    return std::nullopt;
  }

  // Only run scripts if the creator detection feature is enabled.
  if (!base::FeatureList::IsEnabled(
          features::kPlatformCreatorDetectionFeature)) {
    return std::nullopt;
  }

  auto* profile = Profile::FromBrowserContext(rfh->GetBrowserContext());

  // Only run scripts if the Rewards service is available for this profile.
  if (!IsSupportedForProfile(profile)) {
    return std::nullopt;
  }

  // Only run scripts if the user has enabled Brave Rewards.
  if (!profile || !profile->GetPrefs()->GetBoolean(prefs::kEnabled)) {
    return std::nullopt;
  }

  // Only run scripts for known "media platform" sites.
  GURL url = rfh->GetLastCommittedURL();
  if (!IsMediaPlatformURL(url)) {
    return std::nullopt;
  }

  // Only run scripts when there is an exact hostname match.
  auto iter = kScriptMap.find(url.host_piece());
  if (iter == kScriptMap.end()) {
    return std::nullopt;
  }

  return LoadScriptResource(iter->second);
}

}  // namespace

CreatorDetectionScriptInjector::CreatorDetectionScriptInjector() = default;
CreatorDetectionScriptInjector::~CreatorDetectionScriptInjector() = default;

void CreatorDetectionScriptInjector::MaybeInjectScript(
    content::RenderFrameHost* rfh) {
  injector_.reset();
  injector_host_token_ = content::GlobalRenderFrameHostToken();
  last_detection_url_ = GURL();

  if (!rfh) {
    return;
  }

  auto script_source = GetDetectionScript(rfh);
  if (!script_source) {
    return;
  }

  injector_host_token_ = rfh->GetGlobalFrameToken();
  rfh->GetRemoteAssociatedInterfaces()->GetInterface(&injector_);

  // Execute the detection script. It must set `braveRewards.detectCreator` to a
  // function. That function will be called by `DetectCreator`.
  ExecuteScript(script_source.value(), base::DoNothing());
}

void CreatorDetectionScriptInjector::DetectCreator(
    content::RenderFrameHost* rfh,
    DetectCreatorCallback callback) {
  // Return asynchronously with `nullopt` if `rfh` is invalid or was not
  // previously set up via `MaybeInjectScript`, or if the previous call was for
  // the same URL.
  if (!rfh || rfh->GetGlobalFrameToken() != injector_host_token_ ||
      !injector_.is_bound() ||
      rfh->GetLastCommittedURL() == last_detection_url_) {
    base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
        FROM_HERE,
        base::BindOnce(&CreatorDetectionScriptInjector::OnDetectionCancelled,
                       weak_factory_.GetWeakPtr(), std::move(callback)));
    return;
  }

  last_detection_url_ = rfh->GetLastCommittedURL();
  ++current_request_id_;

  // Call the detection function set up by the detection script.
  ExecuteScript(
      "braveRewards.detectCreator()",
      base::BindOnce(&CreatorDetectionScriptInjector::OnCreatorDetected,
                     weak_factory_.GetWeakPtr(), std::move(callback),
                     current_request_id_));
}

void CreatorDetectionScriptInjector::ExecuteScript(
    std::string_view script,
    ExecuteScriptCallback callback) {
  CHECK(injector_.is_bound());
  injector_->RequestAsyncExecuteScript(
      ISOLATED_WORLD_ID_BRAVE_INTERNAL, base::UTF8ToUTF16(script),
      blink::mojom::UserActivationOption::kDoNotActivate,
      blink::mojom::PromiseResultOption::kAwait, std::move(callback));
}

void CreatorDetectionScriptInjector::OnDetectionCancelled(
    DetectCreatorCallback callback) {
  std::move(callback).Run(std::nullopt);
}

void CreatorDetectionScriptInjector::OnCreatorDetected(
    DetectCreatorCallback callback,
    uint64_t request_id,
    base::Value value) {
  // Return `nullopt` if this result was for a previous request.
  if (request_id != current_request_id_) {
    std::move(callback).Run(std::nullopt);
    return;
  }

  Result result;
  if (auto* dict = value.GetIfDict()) {
    if (auto* id = dict->FindString("id")) {
      result.id = *id;
    }
    if (auto* name = dict->FindString("name")) {
      result.name = *name;
    }
    if (auto* url = dict->FindString("url")) {
      result.url = *url;
    }
    if (auto* image_url = dict->FindString("imageURL")) {
      result.image_url = *image_url;
    }
  }
  std::move(callback).Run(std::move(result));
}

CreatorDetectionScriptInjector::Result::Result() = default;
CreatorDetectionScriptInjector::Result::~Result() = default;
CreatorDetectionScriptInjector::Result::Result(const Result&) = default;
CreatorDetectionScriptInjector::Result&
CreatorDetectionScriptInjector::Result::operator=(const Result&) = default;

}  // namespace brave_rewards
