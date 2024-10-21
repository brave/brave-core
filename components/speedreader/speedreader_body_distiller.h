/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_SPEEDREADER_SPEEDREADER_BODY_DISTILLER_H_
#define BRAVE_COMPONENTS_SPEEDREADER_SPEEDREADER_BODY_DISTILLER_H_

#include <memory>
#include <string>

#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/body_sniffer/body_sniffer_url_loader.h"
#include "url/gurl.h"

namespace speedreader {

enum class DistillationResult : int;

class SpeedreaderRewriterService;
class SpeedreaderService;
class SpeedreaderDelegate;

class SpeedreaderBodyDistiller : public body_sniffer::BodyHandler {
 public:
  ~SpeedreaderBodyDistiller() override;

  static std::unique_ptr<SpeedreaderBodyDistiller> MaybeCreate(
      SpeedreaderRewriterService* rewriter_service,
      SpeedreaderService* speedreader_service,
      base::WeakPtr<SpeedreaderDelegate> speedreader_delegate);

  bool OnRequest(network::ResourceRequest* request) override;
  bool ShouldProcess(const GURL& response_url,
                     network::mojom::URLResponseHead* response_head,
                     bool* defer) override;
  void OnBeforeSending() override;
  void OnComplete() override;
  Action OnBodyUpdated(const std::string& body, bool is_complete) override;

  bool IsTransformer() const override;
  void Transform(std::string body,
                 base::OnceCallback<void(std::string)> on_complete) override;
  void UpdateResponseHead(
      network::mojom::URLResponseHead* response_head) override;

 private:
  SpeedreaderBodyDistiller(
      SpeedreaderRewriterService* rewriter_service,
      SpeedreaderService* speedreader_service,
      base::WeakPtr<SpeedreaderDelegate> speedreader_delegate);

  GURL response_url_;

  // Not Owned
  raw_ptr<SpeedreaderRewriterService> rewriter_service_ = nullptr;
  raw_ptr<SpeedreaderService> speedreader_service_ = nullptr;
  base::WeakPtr<SpeedreaderDelegate> speedreader_delegate_;

  DistillationResult distillation_result_;

  base::WeakPtrFactory<SpeedreaderBodyDistiller> weak_factory_{this};
};

}  // namespace speedreader

#endif  // BRAVE_COMPONENTS_SPEEDREADER_SPEEDREADER_BODY_DISTILLER_H_
