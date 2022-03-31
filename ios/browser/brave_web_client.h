/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_BRAVE_WEB_CLIENT_H_
#define BRAVE_IOS_BROWSER_BRAVE_WEB_CLIENT_H_

#include <memory>
#include <string>
#include <vector>

#include "base/compiler_specific.h"
#import "ios/web/public/web_client.h"

class BraveWebMainParts;

class BraveWebClient : public web::WebClient {
 public:
  BraveWebClient();
  BraveWebClient(const BraveWebClient&) = delete;
  BraveWebClient& operator=(const BraveWebClient&) = delete;
  ~BraveWebClient() override;

  void SetUserAgent(const std::string& user_agent);

  // WebClient implementation.
  void AddAdditionalSchemes(Schemes* schemes) const override;
  bool IsAppSpecificURL(const GURL& url) const override;

  std::unique_ptr<web::WebMainParts> CreateWebMainParts() override;
  std::string GetUserAgent(web::UserAgentType type) const override;
  base::StringPiece GetDataResource(
      int resource_id,
      ui::ResourceScaleFactor scale_factor) const override;
  base::RefCountedMemory* GetDataResourceBytes(int resource_id) const override;

  void GetAdditionalWebUISchemes(
      std::vector<std::string>* additional_schemes) override;

 private:
  BraveWebMainParts* web_main_parts_;
  std::string user_agent_;
};

#endif  // BRAVE_IOS_BROWSER_BRAVE_WEB_CLIENT_H_
