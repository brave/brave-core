/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/publisher/media/media.h"

#include <utility>

#include "base/containers/flat_map.h"
#include "base/feature_list.h"
#include "brave/components/brave_rewards/common/features.h"
#include "brave/components/brave_rewards/core/publisher/publisher.h"
#include "brave/components/brave_rewards/core/publisher/static_values.h"
#include "brave/components/brave_rewards/core/rewards_engine.h"
#include "build/build_config.h"

namespace brave_rewards::internal {

Media::Media(RewardsEngine& engine)
    : engine_(engine), media_youtube_(engine), media_github_(engine) {}

Media::~Media() = default;

// static
std::string Media::GetLinkType(const std::string& url,
                               const std::string& first_party_url,
                               const std::string& referrer) {
  if (base::FeatureList::IsEnabled(
          features::kPlatformCreatorDetectionFeature)) {
    return "";
  }

  std::string type = YouTube::GetLinkType(url);

  if (type.empty()) {
    type = GitHub::GetLinkType(url);
  }

  return type;
}

void Media::ProcessMedia(const base::flat_map<std::string, std::string>& parts,
                         const std::string& type,
                         mojom::VisitDataPtr visit_data) {
  if (parts.empty() || !visit_data) {
    return;
  }

  if (type == YOUTUBE_MEDIA_TYPE) {
    media_youtube_.ProcessMedia(parts, *visit_data);
    return;
  }

  if (type == GITHUB_MEDIA_TYPE) {
    media_github_.ProcessMedia(parts, *visit_data);
    return;
  }
}

void Media::GetMediaActivityFromUrl(uint64_t window_id,
                                    mojom::VisitDataPtr visit_data,
                                    const std::string& type,
                                    const std::string& publisher_blob) {
  if (type == YOUTUBE_MEDIA_TYPE) {
    media_youtube_.ProcessActivityFromUrl(window_id, *visit_data);
  } else if (type == GITHUB_MEDIA_TYPE) {
    media_github_.ProcessActivityFromUrl(window_id, *visit_data);
  } else {
    OnMediaActivityError(std::move(visit_data), type, window_id);
  }
}

void Media::OnMediaActivityError(mojom::VisitDataPtr visit_data,
                                 const std::string& type,
                                 uint64_t window_id) {
  std::string url;
  std::string name;

  if (type == YOUTUBE_MEDIA_TYPE) {
    url = YOUTUBE_DOMAIN;
    name = YOUTUBE_MEDIA_TYPE;
  }

  if (url.empty()) {
    engine_->LogError(FROM_HERE) << "Media activity error";
    return;
  }

  visit_data->domain = url;
  visit_data->url = "https://" + url;
  visit_data->path = "/";
  visit_data->name = name;

  engine_->publisher()->GetPublisherActivityFromUrl(window_id,
                                                    std::move(visit_data), "");
}

void Media::SaveMediaInfo(const std::string& type,
                          const base::flat_map<std::string, std::string>& data,
                          PublisherInfoCallback callback) {
  if (type == GITHUB_MEDIA_TYPE) {
    media_github_.SaveMediaInfo(data, std::move(callback));
    return;
  }
}

}  // namespace brave_rewards::internal
