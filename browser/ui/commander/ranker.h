// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_COMMANDER_RANKER_H_
#define BRAVE_BROWSER_UI_COMMANDER_RANKER_H_

#include <memory>
#include <string>
#include <tuple>
#include <vector>

#include "base/time/time.h"
#include "brave/browser/ui/commander/command_source.h"
#include "components/prefs/pref_service.h"

namespace commander {

class Ranker {
 public:
  explicit Ranker(PrefService* prefs);
  Ranker(const Ranker&) = delete;
  Ranker& operator=(const Ranker&) = delete;
  ~Ranker();

  void Visit(const CommandItem& item);
  double GetRank(const CommandItem& item);

  void Rank(std::vector<std::unique_ptr<CommandItem>>& items,
            size_t max_results);

 private:
  std::string GetId(const CommandItem& item) const;
  std::tuple<int, base::Time> GetInfo(const std::string& id) const;

  raw_ptr<PrefService> prefs_;
};

}  // namespace commander

#endif  // BRAVE_BROWSER_UI_COMMANDER_RANKER_H_
