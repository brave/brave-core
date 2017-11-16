// Copyright (c) 2017 The Brave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BRAVE_UTILITY_BRAVE_CONTENT_UTILITY_CLIENT_H_
#define BRAVE_UTILITY_BRAVE_CONTENT_UTILITY_CLIENT_H_

#include "chrome/utility/chrome_content_utility_client.h"

class BraveContentUtilityClient : public ChromeContentUtilityClient {
 public:
  BraveContentUtilityClient();
  ~BraveContentUtilityClient() override;

 private:
  DISALLOW_COPY_AND_ASSIGN(BraveContentUtilityClient);
};

#endif  // BRAVE_UTILITY_BRAVE_CONTENT_UTILITY_CLIENT_H_
