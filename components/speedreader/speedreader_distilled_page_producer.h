/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_SPEEDREADER_SPEEDREADER_DISTILLED_PAGE_PRODUCER_H_
#define BRAVE_COMPONENTS_SPEEDREADER_SPEEDREADER_DISTILLED_PAGE_PRODUCER_H_

#include <memory>
#include <string>

#include "base/memory/weak_ptr.h"
#include "brave/components/body_sniffer/body_sniffer_url_loader.h"

namespace speedreader {

class SpeedreaderDelegate;

class SpeedreaderDistilledPageProducer : public body_sniffer::BodyProducer {
 public:
  ~SpeedreaderDistilledPageProducer() override;

  static std::unique_ptr<SpeedreaderDistilledPageProducer> MaybeCreate(
      base::WeakPtr<SpeedreaderDelegate> speedreader_delegate);

  void UpdateResponseHead(
      network::mojom::URLResponseHead* response_head) override;
  std::string TakeContent() override;

  void OnBeforeSending() override;
  void OnComplete() override;

 private:
  explicit SpeedreaderDistilledPageProducer(
      base::WeakPtr<SpeedreaderDelegate> speedreader_delegate);
  base::WeakPtr<SpeedreaderDelegate> speedreader_delegate_;
};

}  // namespace speedreader

#endif  // BRAVE_COMPONENTS_SPEEDREADER_SPEEDREADER_DISTILLED_PAGE_PRODUCER_H_
