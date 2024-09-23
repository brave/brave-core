/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/speedreader/speedreader_distilled_page_producer.h"

#include <memory>
#include <string>
#include <utility>

#include "brave/components/speedreader/speedreader_delegate.h"
#include "brave/components/speedreader/speedreader_util.h"
#include "services/network/public/mojom/url_response_head.mojom.h"

namespace speedreader {

SpeedreaderDistilledPageProducer::SpeedreaderDistilledPageProducer(
    base::WeakPtr<SpeedreaderDelegate> speedreader_delegate)
    : speedreader_delegate_(std::move(speedreader_delegate)) {}

SpeedreaderDistilledPageProducer::~SpeedreaderDistilledPageProducer() = default;

// static
std::unique_ptr<SpeedreaderDistilledPageProducer>
SpeedreaderDistilledPageProducer::MaybeCreate(
    base::WeakPtr<SpeedreaderDelegate> speedreader_delegate) {
  if (!speedreader_delegate || !speedreader_delegate->IsPageContentPresent()) {
    return nullptr;
  }
  return base::WrapUnique(
      new SpeedreaderDistilledPageProducer(std::move(speedreader_delegate)));
}

void SpeedreaderDistilledPageProducer::UpdateResponseHead(
    network::mojom::URLResponseHead* response_head) {
  // We already got the content of the page and we know it is an utf-8
  // encoded html. So ignore any encodings from the headers.
  response_head->charset = "utf-8";
  if (response_head->headers) {
    response_head->headers->SetHeader("Content-Type",
                                      "text/html; charset=utf-8");
  }
}

std::string SpeedreaderDistilledPageProducer::TakeContent() {
  if (speedreader_delegate_) {
    return speedreader_delegate_->TakePageContent();
  }
  return {};
}

void SpeedreaderDistilledPageProducer::OnBeforeSending() {
  if (speedreader_delegate_) {
    speedreader_delegate_->OnDistillComplete(DistillationResult::kSuccess);
  }
}

void SpeedreaderDistilledPageProducer::OnComplete() {
  if (speedreader_delegate_) {
    speedreader_delegate_->OnDistilledDocumentSent();
  }
}

}  // namespace speedreader
