// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_shields/content/browser/ad_block_localhost_filters_provider.h"

#include <utility>
#include <vector>

#include "base/task/single_thread_task_runner.h"
#include "brave/components/brave_shields/core/browser/ad_block_filters_provider_manager.h"
#include "components/prefs/pref_service.h"

namespace brave_shields {

namespace {

// static
void PairAndCallBack(
    base::OnceCallback<void(std::pair<uint8_t, DATFileDataBuffer>)> cb,
    uint8_t permission_mask,
    DATFileDataBuffer buffer) {
  std::move(cb).Run({permission_mask, buffer});
}

const char kLocalhostBadfilters[] = R"(
||0.0.0.0^$third-party,domain=~[::]|~[::ffff:0:0],badfilter
||[::]^$third-party,domain=~0.0.0.0|~[::ffff:0:0],badfilter
||[::ffff:0:0]^$third-party,domain=~0.0.0.0|~[::],badfilter
||localhost^$third-party,domain=~127.0.0.1|~[::1]|~[::ffff:7f00:1],badfilter
||127.0.0.1^$third-party,domain=~localhost|~[::1]|~[::ffff:7f00:1],badfilter
||[::1]^$third-party,domain=~localhost|~127.0.0.1|~[::ffff:7f00:1],badfilter
||[::ffff:7f00:1]^$third-party,domain=~localhost|~127.0.0.1|~[::1],badfilter
)";

}  // namespace

AdBlockLocalhostFiltersProvider::AdBlockLocalhostFiltersProvider()
    : AdBlockFiltersProvider(true) {
  NotifyObservers(engine_is_default_);
}

AdBlockLocalhostFiltersProvider::~AdBlockLocalhostFiltersProvider() {}

std::string AdBlockLocalhostFiltersProvider::GetNameForDebugging() {
  return "AdBlockLocalhostFiltersProvider";
}

void AdBlockLocalhostFiltersProvider::LoadFilterSet(
    base::OnceCallback<void(std::pair<uint8_t, DATFileDataBuffer>)> cb) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  auto buffer = std::vector<unsigned char>(std::begin(kLocalhostBadfilters),
                                           std::end(kLocalhostBadfilters));

  // PostTask so this has an async return to match other loaders
  base::SingleThreadTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE, base::BindOnce(&PairAndCallBack, std::move(cb), 0, buffer));
}

}  // namespace brave_shields
