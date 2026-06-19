/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/misc_metrics/web3_metrics.h"

#include <utility>

#include "base/metrics/histogram_macros.h"

namespace misc_metrics {

Web3Metrics::Web3Metrics() = default;

Web3Metrics::~Web3Metrics() = default;

void Web3Metrics::BindReceiver(
    mojo::PendingReceiver<mojom::Web3Metrics> receiver) {
  receivers_.Add(this, std::move(receiver));
}

void Web3Metrics::RecordDappVisit() {
  UMA_HISTOGRAM_BOOLEAN(kWeb3DappVisitHistogramName, true);
}

}  // namespace misc_metrics
