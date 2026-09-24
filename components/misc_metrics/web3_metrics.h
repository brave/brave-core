/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_MISC_METRICS_WEB3_METRICS_H_
#define BRAVE_COMPONENTS_MISC_METRICS_WEB3_METRICS_H_

#include "brave/components/misc_metrics/common/misc_metrics.mojom.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/receiver_set.h"

namespace misc_metrics {

inline constexpr char kWeb3DappVisitHistogramName[] = "Brave.Web3.DappVisited";

class Web3Metrics : public mojom::Web3Metrics {
 public:
  Web3Metrics();
  ~Web3Metrics() override;

  Web3Metrics(const Web3Metrics&) = delete;
  Web3Metrics& operator=(const Web3Metrics&) = delete;

  void BindReceiver(mojo::PendingReceiver<mojom::Web3Metrics> receiver);

  // mojom::Web3Metrics:
  void RecordDappVisit() override;

 private:
  mojo::ReceiverSet<mojom::Web3Metrics> receivers_;
};

}  // namespace misc_metrics

#endif  // BRAVE_COMPONENTS_MISC_METRICS_WEB3_METRICS_H_
