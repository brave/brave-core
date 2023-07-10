/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_DATABASE_DATABASE_PUBLISHER_PREFIX_LIST_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_DATABASE_DATABASE_PUBLISHER_PREFIX_LIST_H_

#include <string>

#include "brave/components/brave_rewards/core/database/database_table.h"
#include "brave/components/brave_rewards/core/publisher/prefix_list_reader.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace brave_rewards::internal {
namespace database {

using SearchPublisherPrefixListCallback = std::function<void(bool)>;

class DatabasePublisherPrefixList : public DatabaseTable {
 public:
  explicit DatabasePublisherPrefixList(RewardsEngineImpl& engine);
  ~DatabasePublisherPrefixList() override;

  void Reset(PrefixListReader reader, LegacyResultCallback callback);

  void Search(const std::string& publisher_key,
              SearchPublisherPrefixListCallback callback);

 private:
  void InsertNext(PrefixIterator begin, LegacyResultCallback callback);

  absl::optional<PrefixListReader> reader_;
};

}  // namespace database
}  // namespace brave_rewards::internal

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_DATABASE_DATABASE_PUBLISHER_PREFIX_LIST_H_
