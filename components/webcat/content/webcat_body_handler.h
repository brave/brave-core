/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_WEBCAT_CONTENT_WEBCAT_BODY_HANDLER_H_
#define BRAVE_COMPONENTS_WEBCAT_CONTENT_WEBCAT_BODY_HANDLER_H_

#include <map>
#include <optional>
#include <string>

#include "base/memory/raw_ptr.h"
#include "brave/components/body_sniffer/body_sniffer_url_loader.h"
#include "brave/components/webcat/core/bundle_parser.h"
#include "brave/components/webcat/core/csp_validator.h"
#include "brave/components/webcat/core/manifest_verifier.h"
#include "url/gurl.h"

namespace network {
struct ResourceRequest;
}

namespace webcat {

class WebcatBodyHandler : public body_sniffer::BodyHandler {
  friend class WebcatBodyHandlerTest;

 public:
  explicit WebcatBodyHandler(const Manifest& manifest);
  ~WebcatBodyHandler() override;

  WebcatBodyHandler(const WebcatBodyHandler&) = delete;
  WebcatBodyHandler& operator=(const WebcatBodyHandler&) = delete;

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
  std::string GetExpectedHash(const std::string& path) const;
  std::string NormalizePath(const std::string& path) const;

  Manifest manifest_;
  GURL response_url_;
  bool should_process_ = false;
  bool is_main_frame_ = false;
  std::string accumulated_body_;
};

}  // namespace webcat

#endif  // BRAVE_COMPONENTS_WEBCAT_CONTENT_WEBCAT_BODY_HANDLER_H_