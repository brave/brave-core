/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_DE_AMP_BROWSER_DE_AMP_BODY_HANDLER_H_
#define BRAVE_COMPONENTS_DE_AMP_BROWSER_DE_AMP_BODY_HANDLER_H_

#include <memory>
#include <string>

#include "base/values.h"
#include "brave/components/body_sniffer/body_sniffer_url_loader.h"
#include "content/public/browser/web_contents.h"
#include "services/network/public/cpp/resource_request.h"
#include "url/gurl.h"

namespace de_amp {

// Handler for AMP HTML detection.
// If AMP page, cancel request and initiate new one to non-AMP canonical link.
class DeAmpBodyHandler : public body_sniffer::BodyHandler {
 public:
  ~DeAmpBodyHandler() override;

  static std::unique_ptr<DeAmpBodyHandler> Create(
      const network::ResourceRequest& request,
      const content::WebContents::Getter& wc_getter);

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
  DeAmpBodyHandler(const network::ResourceRequest& request,
                   const content::WebContents::Getter& wc_getter);
  bool MaybeRedirectToCanonicalLink(const std::string& body);
  bool OpenCanonicalURL(const GURL& new_url);

  network::ResourceRequest request_;
  content::WebContents::Getter wc_getter_;
  GURL response_url_;

  base::Value navigation_chain_;
  size_t bytes_analyzed_ = 0;

  enum class State {
    kCheckForAmp,
    kFindForCanonicalUrl,
  } state_ = State::kCheckForAmp;
};

}  // namespace de_amp

#endif  // BRAVE_COMPONENTS_DE_AMP_BROWSER_DE_AMP_BODY_HANDLER_H_
