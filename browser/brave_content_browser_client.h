// Copyright (c) 2017 The Brave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BRAVE_BROWSER_BRAVE_CONTENT_BROWSER_CLIENT_H_
#define BRAVE_BROWSER_BRAVE_CONTENT_BROWSER_CLIENT_H_

#include "chrome/browser/chrome_content_browser_client.h"

class BraveContentBrowserClient : public ChromeContentBrowserClient {
 public:
  BraveContentBrowserClient();
  ~BraveContentBrowserClient() override;

 private:
  DISALLOW_COPY_AND_ASSIGN(BraveContentBrowserClient);
};

#endif  // BRAVE_BROWSER_BRAVE_CONTENT_BROWSER_CLIENT_H_
