/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_ADS_TABS_HTML_CONTENT_BODY_HANDLER_H_
#define BRAVE_BROWSER_BRAVE_ADS_TABS_HTML_CONTENT_BODY_HANDLER_H_

#include <memory>
#include <string>

#include "brave/components/body_sniffer/body_sniffer_url_loader.h"
#include "content/public/browser/web_contents.h"
#include "services/network/public/cpp/resource_request.h"
#include "url/gurl.h"

class Profile;

namespace brave_ads {

class AdsService;

class HtmlContentBodyHandler : public body_sniffer::BodyHandler {
 public:
  ~HtmlContentBodyHandler() override;

  static std::unique_ptr<HtmlContentBodyHandler> MaybeCreate(
      AdsService* const ads_service,
      const content::WebContents::Getter& web_contents_getter);

  // body_sniffer::BodyHandler:
  bool OnRequest(network::ResourceRequest* request) override;
  bool ShouldProcess(const GURL& response_url,
                     network::mojom::URLResponseHead* response_head,
                     bool* defer) override;
  void OnComplete() override;
  Action OnBodyUpdated(const std::string& body, bool is_complete) override;
  bool IsTransformer() const override;
  void Transform(std::string body,
                 base::OnceCallback<void(std::string)> on_complete) override;
  void UpdateResponseHead(
      network::mojom::URLResponseHead* response_head) override {}

 private:
  HtmlContentBodyHandler(AdsService* const ads_service,
                         content::WebContents::Getter web_contents_getter);

  content::NavigationEntry* GetPendingNavigationEntry();

  void MaybeNotifyTabHtmlContentDidChange();

  const raw_ptr<AdsService> ads_service_;
  const content::WebContents::Getter web_contents_getter_;
  std::string html_;
};

}  // namespace brave_ads

#endif  // BRAVE_BROWSER_BRAVE_ADS_TABS_HTML_CONTENT_BODY_HANDLER_H_
