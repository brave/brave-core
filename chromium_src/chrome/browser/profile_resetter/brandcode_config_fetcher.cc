/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/profile_resetter/brandcode_config_fetcher.h"

#include <utility>

#include "base/functional/bind.h"
#include "base/task/sequenced_task_runner.h"

namespace {

// Brave disables the brandcode config fetch: tell the caller it's done,
// asynchronously as a real fetch would.
void ReportFetchSkipped(BrandcodeConfigFetcher::FetchCallback& callback) {
  base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE, base::BindOnce(std::move(callback)));
}

}  // namespace

#include <chrome/browser/profile_resetter/brandcode_config_fetcher.cc>
