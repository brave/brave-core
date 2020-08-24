/* Copyright 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMMON_BRAVE_CONTENT_CLIENT_H_
#define BRAVE_COMMON_BRAVE_CONTENT_CLIENT_H_

#include "chrome/common/chrome_content_client.h"

class BraveContentClient : public ChromeContentClient {
 public:
  BraveContentClient();
  ~BraveContentClient() override;

 private:
  // ChromeContentClient overrides:
  base::RefCountedMemory* GetDataResourceBytes(int resource_id) override;
  void AddAdditionalSchemes(Schemes* schemes) override;
};

#endif  // BRAVE_COMMON_BRAVE_CONTENT_CLIENT_H_
