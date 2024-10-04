/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/speedreader/speedreader_body_distiller.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/check.h"
#include "base/command_line.h"
#include "base/files/file_util.h"
#include "base/functional/bind.h"
#include "base/memory/weak_ptr.h"
#include "base/strings/string_util.h"
#include "brave/components/speedreader/speedreader_delegate.h"
#include "brave/components/speedreader/speedreader_rewriter_service.h"
#include "brave/components/speedreader/speedreader_service.h"
#include "brave/components/speedreader/speedreader_util.h"
#include "services/network/public/mojom/url_response_head.mojom.h"

namespace speedreader {

namespace {

void MaybeSaveDistilledDataForDebug(const GURL& url,
                                    const std::string& data,
                                    const std::string& stylesheet,
                                    const std::string& transformed) {
#if DCHECK_IS_ON()
  constexpr const char kCollectSwitch[] = "speedreader-collect-test-data";
  if (!base::CommandLine::ForCurrentProcess()->HasSwitch(kCollectSwitch)) {
    return;
  }
  const auto dir = base::CommandLine::ForCurrentProcess()->GetSwitchValuePath(
      kCollectSwitch);
  base::CreateDirectory(dir);
  base::WriteFile(dir.AppendASCII("page.url"), url.spec());
  base::WriteFile(dir.AppendASCII("original.html"), data);
  base::WriteFile(dir.AppendASCII("distilled.html"), transformed);
  base::WriteFile(dir.AppendASCII("result.html"), stylesheet + transformed);
#endif
}

}  // namespace

SpeedreaderBodyDistiller::SpeedreaderBodyDistiller(
    SpeedreaderRewriterService* rewriter_service,
    SpeedreaderService* speedreader_service,
    base::WeakPtr<SpeedreaderDelegate> speedreader_delegate)
    : rewriter_service_(rewriter_service),
      speedreader_service_(speedreader_service),
      speedreader_delegate_(std::move(speedreader_delegate)) {}

SpeedreaderBodyDistiller::~SpeedreaderBodyDistiller() = default;

// static
std::unique_ptr<SpeedreaderBodyDistiller> SpeedreaderBodyDistiller::MaybeCreate(
    SpeedreaderRewriterService* rewriter_service,
    SpeedreaderService* speedreader_service,
    base::WeakPtr<SpeedreaderDelegate> speedreader_delegate) {
  DCHECK(speedreader_delegate);
  if (!speedreader_delegate->IsPageDistillationAllowed()) {
    return nullptr;
  }
  return base::WrapUnique(new SpeedreaderBodyDistiller(
      rewriter_service, speedreader_service, std::move(speedreader_delegate)));
}

bool SpeedreaderBodyDistiller::OnRequest(network::ResourceRequest* request) {
  return true;
}

bool SpeedreaderBodyDistiller::ShouldProcess(
    const GURL& response_url,
    network::mojom::URLResponseHead* response_head,
    bool* defer) {
  if (!speedreader_delegate_ ||
      !speedreader_delegate_->IsPageDistillationAllowed()) {
    // The page was redirected to an ineligible URL. Skip.
    return false;
  }

  std::string mime_type;
  if (!response_head || !response_head->headers ||
      !response_head->headers->GetMimeType(&mime_type) ||
      base::CompareCaseInsensitiveASCII(mime_type, "text/html")) {
    // Skip all non-html documents.
    return false;
  }

  *defer = true;

  response_url_ = response_url;
  return true;
}

void SpeedreaderBodyDistiller::OnBeforeSending() {
  if (speedreader_delegate_) {
    speedreader_delegate_->OnDistillComplete(distillation_result_);
  }
}

void SpeedreaderBodyDistiller::OnComplete() {
  if (speedreader_delegate_) {
    speedreader_delegate_->OnDistilledDocumentSent();
  }
}

SpeedreaderBodyDistiller::Action SpeedreaderBodyDistiller::OnBodyUpdated(
    const std::string& body,
    bool is_complete) {
  return is_complete ? SpeedreaderBodyDistiller::Action::kComplete
                     : SpeedreaderBodyDistiller::Action::kContinue;
}

bool SpeedreaderBodyDistiller::IsTransformer() const {
  return true;
}

void SpeedreaderBodyDistiller::Transform(
    std::string body,
    base::OnceCallback<void(std::string)> on_complete) {
  if (body.empty()) {
    return std::move(on_complete).Run(std::move(body));
  }

  speedreader::DistillPage(
      response_url_, std::move(body), speedreader_service_, rewriter_service_,
      base::BindOnce(
          [](base::WeakPtr<SpeedreaderBodyDistiller> self, const GURL& url,
             const std::string& stylesheet,
             base::OnceCallback<void(std::string)> on_complete,
             speedreader::DistillationResult result, std::string original_data,
             std::string transformed) {
            if (!self) {
              return;
            }

            self->distillation_result_ = result;

            if (result == speedreader::DistillationResult::kSuccess) {
              MaybeSaveDistilledDataForDebug(url, original_data, stylesheet,
                                             transformed);
              std::move(on_complete).Run(stylesheet + std::move(transformed));
            } else {
              std::move(on_complete).Run(original_data);
            }
          },
          weak_factory_.GetWeakPtr(), response_url_,
          rewriter_service_->GetContentStylesheet(), std::move(on_complete)));
}

void SpeedreaderBodyDistiller::UpdateResponseHead(
    network::mojom::URLResponseHead* response_head) {}

}  // namespace speedreader
