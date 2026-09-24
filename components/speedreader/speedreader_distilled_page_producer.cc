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

namespace {

// The reload which sends the distilled content doesn't include the fragment in
// the network request, so the URLs are compared ignoring it.
bool IsSameUrlIgnoringRef(const GURL& a, const GURL& b) {
  return a.is_valid() && b.is_valid() && a.GetWithoutRef() == b.GetWithoutRef();
}

}  // namespace

SpeedreaderDistilledPageProducer::SpeedreaderDistilledPageProducer(
    const GURL& request_url,
    base::WeakPtr<SpeedreaderDelegate> speedreader_delegate)
    : request_url_(request_url),
      speedreader_delegate_(std::move(speedreader_delegate)) {}

SpeedreaderDistilledPageProducer::~SpeedreaderDistilledPageProducer() {
  if (!TakeContent().empty()) {
    // If throttle does not use the content (e.g., due to redirects), we should
    // render the received content.
    speedreader_delegate_->OnDistillComplete(DistillationResult::kFail);
  }
}

// static
std::unique_ptr<SpeedreaderDistilledPageProducer>
SpeedreaderDistilledPageProducer::MaybeCreate(
    const GURL& request_url,
    base::WeakPtr<SpeedreaderDelegate> speedreader_delegate) {
  if (!speedreader_delegate || !speedreader_delegate->IsPageContentPresent()) {
    return nullptr;
  }
  return base::WrapUnique(new SpeedreaderDistilledPageProducer(
      request_url, std::move(speedreader_delegate)));
}

bool SpeedreaderDistilledPageProducer::ShouldProcess(
    const GURL& response_url,
    network::mojom::URLResponseHead* response_head) {
  // The distilled content belongs to the page it was distilled from and must
  // never be sent as a body of any other URL, so re-check the URL of the
  // response, it may differ from the URL the request was started with.
  return speedreader_delegate_ &&
         IsSameUrlIgnoringRef(request_url_, response_url);
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
