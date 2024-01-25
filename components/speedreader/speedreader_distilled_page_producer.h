/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_SPEEDREADER_SPEEDREADER_LOCAL_URL_LOADER_H_
#define BRAVE_COMPONENTS_SPEEDREADER_SPEEDREADER_LOCAL_URL_LOADER_H_

#include "base/memory/weak_ptr.h"

#include "brave/components/body_sniffer/body_sniffer_url_loader.h"

namespace speedreader {

class SpeedreaderDelegate;

class SpeedreaderDistilledPageProducer : public body_sniffer::BodyProducer {
 public:
  ~SpeedreaderDistilledPageProducer() override;

  static std::unique_ptr<SpeedreaderDistilledPageProducer> Create(
      base::WeakPtr<SpeedreaderDelegate> speedreader_delegate);

  void UpdateResponseHead(
      network::mojom::URLResponseHead* response_head) override;
  std::string TakeContent() override;

  void OnComplete() override;

 private:
  explicit SpeedreaderDistilledPageProducer(
      base::WeakPtr<SpeedreaderDelegate> speedreader_delegate);
  base::WeakPtr<SpeedreaderDelegate> speedreader_delegate_;
};

}  // namespace speedreader

#endif  // BRAVE_COMPONENTS_SPEEDREADER_SPEEDREADER_LOCAL_URL_LOADER_H_
