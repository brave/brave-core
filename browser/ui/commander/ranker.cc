// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/commander/ranker.h"

#include <algorithm>
#include <cstdlib>
#include <iterator>
#include <memory>
#include <string>
#include <tuple>

#include "base/strings/utf_string_conversions.h"
#include "base/time/time.h"
#include "brave/components/commander/common/pref_names.h"
#include "chrome/browser/ui/commander/command_source.h"
#include "components/history/core/browser/keyword_search_term_util.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"

namespace commander {

namespace {
constexpr double kDoubleComparisonSlop = 0.001;
}

Ranker::Ranker(PrefService* prefs) : prefs_(prefs) {}
Ranker::~Ranker() = default;

void Ranker::Visit(const CommandItem& item) {
  const auto id = GetId(item);
  ScopedDictPrefUpdate update(prefs_, prefs::kCommanderFrecencies);

  auto* entry = update->EnsureDict(id);
  auto visit_count = entry->FindInt("visit_count").value_or(0);
  entry->Set("visit_count", visit_count + 1);
  entry->Set("last_visit", base::Time::Now().InMillisecondsFSinceUnixEpoch());
}

double Ranker::GetRank(const CommandItem& item) {
  const auto [visits, visit_time] = GetInfo(GetId(item));
  return history::GetFrecencyScore(visits, visit_time, base::Time::Now());
}

void Ranker::Rank(std::vector<std::unique_ptr<CommandItem>>& items,
                  size_t max_results) {
  max_results = std::min(items.size(), max_results);
  std::partial_sort(
      std::begin(items), std::begin(items) + max_results, std::end(items),
      [this](const std::unique_ptr<CommandItem>& left,
             const std::unique_ptr<CommandItem>& right) {
        auto l_rank = (0.5 + this->GetRank(*left.get())) * left->score;
        auto r_rank = (0.5 + this->GetRank(*right.get())) * right->score;
        return abs(l_rank - r_rank) < kDoubleComparisonSlop
                   ? left->title < right->title
                   : l_rank > r_rank;
      });
}

std::string Ranker::GetId(const CommandItem& item) const {
  // TODO(fallaciousreasoning): Introduce a more stable id for commands.
  return base::UTF16ToUTF8(item.title);
}

std::tuple<int, base::Time> Ranker::GetInfo(const std::string& id) const {
  auto* entry = prefs_->GetDict(prefs::kCommanderFrecencies).FindDict(id);
  if (!entry) {
    return std::make_tuple(0, base::Time::Min());
  }

  auto visit_count = entry->FindInt("visit_count").value_or(0);
  auto last_visit = base::Time::FromMillisecondsSinceUnixEpoch(
      entry->FindDouble("last_visit").value_or(0));
  return std::make_tuple(visit_count, last_visit);
}

}  // namespace commander
