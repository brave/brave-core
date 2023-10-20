// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_shields/browser/ad_block_localhost_filters_provider.h"

#include <utility>
#include <vector>

#include "base/task/single_thread_task_runner.h"
#include "brave/components/brave_shields/browser/ad_block_filters_provider_manager.h"
#include "components/prefs/pref_service.h"

namespace brave_shields {

AdBlockLocalhostFiltersProvider::AdBlockLocalhostFiltersProvider()
    : AdBlockFiltersProvider(true) {}

AdBlockLocalhostFiltersProvider::~AdBlockLocalhostFiltersProvider() {}

std::string AdBlockLocalhostFiltersProvider::GetNameForDebugging() {
  return "AdBlockLocalhostFiltersProvider";
}

void AdBlockLocalhostFiltersProvider::LoadDATBuffer(
    base::OnceCallback<void(bool deserialize, const DATFileDataBuffer& dat_buf)>
        cb) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  const std::string& badfilters_for_localhost =
      R"(
   ||0.0.0.0^$third-party,domain=~[::]|~[::ffff:0:0],badfilter
   ||[::]^$third-party,domain=~0.0.0.0|~[::ffff:0:0],badfilter
   ||[::ffff:0:0]^$third-party,domain=~0.0.0.0|~[::],badfilter
   ||localhost^$third-party,domain=~127.0.0.1|~[::1]|~[::ffff:7f00:1],badfilter
   ||127.0.0.1^$third-party,domain=~localhost|~[::1]|~[::ffff:7f00:1],badfilter
   ||[::1]^$third-party,domain=~localhost|~127.0.0.1|~[::ffff:7f00:1],badfilter
   ||[::ffff:7f00:1]^$third-party,domain=~localhost|~127.0.0.1|~[::1],badfilter
  )";

  auto buffer = std::vector<unsigned char>(badfilters_for_localhost.begin(),
                                           badfilters_for_localhost.end());

  // PostTask so this has an async return to match other loaders
  base::SingleThreadTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE, base::BindOnce(std::move(cb), false, std::move(buffer)));
}

void AdBlockLocalhostFiltersProvider::AddObserver(
    AdBlockFiltersProvider::Observer* observer) {
  AdBlockFiltersProvider::AddObserver(observer);
  NotifyObservers(engine_is_default_);
}

}  // namespace brave_shields
