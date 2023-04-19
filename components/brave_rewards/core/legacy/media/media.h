/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_LEGACY_MEDIA_MEDIA_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_LEGACY_MEDIA_MEDIA_H_

#include <memory>
#include <string>

#include "base/containers/flat_map.h"
#include "base/memory/raw_ref.h"
#include "brave/components/brave_rewards/core/ledger_callbacks.h"
#include "brave/components/brave_rewards/core/legacy/media/github.h"
#include "brave/components/brave_rewards/core/legacy/media/youtube.h"

namespace ledger {
class LedgerImpl;
}

namespace braveledger_media {

class Media {
 public:
  explicit Media(ledger::LedgerImpl& ledger);

  ~Media();

  static std::string GetLinkType(const std::string& url,
                                 const std::string& first_party_url,
                                 const std::string& referrer);

  void ProcessMedia(const base::flat_map<std::string, std::string>& parts,
                    const std::string& type,
                    ledger::mojom::VisitDataPtr visit_data);

  void GetMediaActivityFromUrl(uint64_t windowId,
                               ledger::mojom::VisitDataPtr visit_data,
                               const std::string& type,
                               const std::string& publisher_blob);

  void SaveMediaInfo(const std::string& type,
                     const base::flat_map<std::string, std::string>& data,
                     ledger::PublisherInfoCallback callback);

  static std::string GetShareURL(
      const std::string& type,
      const base::flat_map<std::string, std::string>& args);

 private:
  void OnMediaActivityError(ledger::mojom::VisitDataPtr visit_data,
                            const std::string& type,
                            uint64_t windowId);

  const raw_ref<ledger::LedgerImpl> ledger_;
  std::unique_ptr<braveledger_media::YouTube> media_youtube_;
  std::unique_ptr<braveledger_media::GitHub> media_github_;
};

}  // namespace braveledger_media

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_LEGACY_MEDIA_MEDIA_H_
