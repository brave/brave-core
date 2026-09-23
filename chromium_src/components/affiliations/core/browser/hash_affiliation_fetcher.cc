/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "components/affiliations/core/browser/hash_affiliation_fetcher.h"

#include <utility>

#include "base/functional/bind.h"
#include "base/location.h"
#include "base/task/sequenced_task_runner.h"
#include "net/base/net_errors.h"

namespace affiliations {

void HashAffiliationFetcher::CompleteStubbedRequest() {
  AffiliationFetcherInterface::FetchResult result;
  result.network_status = net::OK;
  std::move(result_callback_).Run(std::move(result));
}

}  // namespace affiliations

#include <components/affiliations/core/browser/hash_affiliation_fetcher.cc>
