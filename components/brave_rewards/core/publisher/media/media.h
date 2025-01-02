/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_PUBLISHER_MEDIA_MEDIA_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_PUBLISHER_MEDIA_MEDIA_H_

#include <string>

#include "base/containers/flat_map.h"
#include "base/memory/raw_ref.h"
#include "brave/components/brave_rewards/core/publisher/media/youtube.h"
#include "brave/components/brave_rewards/core/rewards_callbacks.h"

namespace brave_rewards::internal {

class RewardsEngine;

class Media {
 public:
  explicit Media(RewardsEngine& engine);

  ~Media();

  static std::string GetLinkType(const std::string& url,
                                 const std::string& first_party_url,
                                 const std::string& referrer);

  void ProcessMedia(const base::flat_map<std::string, std::string>& parts,
                    const std::string& type,
                    mojom::VisitDataPtr visit_data);

  void GetMediaActivityFromUrl(uint64_t windowId,
                               mojom::VisitDataPtr visit_data,
                               const std::string& type,
                               const std::string& publisher_blob);

  void SaveMediaInfo(const std::string& type,
                     const base::flat_map<std::string, std::string>& data,
                     PublisherInfoCallback callback);

  static std::string GetShareURL(
      const std::string& type,
      const base::flat_map<std::string, std::string>& args);

 private:
  void OnMediaActivityError(mojom::VisitDataPtr visit_data,
                            const std::string& type,
                            uint64_t windowId);

  const raw_ref<RewardsEngine> engine_;
  YouTube media_youtube_;
};

}  // namespace brave_rewards::internal

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_PUBLISHER_MEDIA_MEDIA_H_
