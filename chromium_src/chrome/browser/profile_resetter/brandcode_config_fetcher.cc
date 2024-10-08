/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <utility>

#include "base/functional/bind.h"
#include "base/task/sequenced_task_runner.h"

// Run the fetch callback asynchronously to tell the caller that we're done, but
// don't actually do anything else as this feature is disabled in Brave
#define BRAVE_BRANDCODE_CONFIG_FETCHER                        \
  base::SequencedTaskRunner::GetCurrentDefault()->PostTask(   \
      FROM_HERE, base::BindOnce(std::move(fetch_callback_))); \
  return;

#include "src/chrome/browser/profile_resetter/brandcode_config_fetcher.cc"

#undef BRAVE_BRANDCODE_CONFIG_FETCHER
