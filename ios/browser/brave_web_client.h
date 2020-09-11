/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_BRAVE_WEB_CLIENT_H_
#define BRAVE_IOS_BROWSER_BRAVE_WEB_CLIENT_H_

#include <string>

#include "base/compiler_specific.h"
#include "base/macros.h"
#import "ios/web/public/web_client.h"

class BraveWebMainParts;

class BraveWebClient : public web::WebClient {
 public:
  BraveWebClient();
  ~BraveWebClient() override;

  void SetUserAgent(const std::string& user_agent);

  // WebClient implementation.
  std::unique_ptr<web::WebMainParts> CreateWebMainParts() override;
  std::string GetUserAgent(web::UserAgentType type) const override;
  base::StringPiece GetDataResource(
      int resource_id,
      ui::ScaleFactor scale_factor) const override;
  base::RefCountedMemory* GetDataResourceBytes(int resource_id) const override;

 private:
  BraveWebMainParts* web_main_parts_;
  std::string user_agent_;

  DISALLOW_COPY_AND_ASSIGN(BraveWebClient);
};

#endif  // BRAVE_IOS_BROWSER_BRAVE_WEB_CLIENT_H_
