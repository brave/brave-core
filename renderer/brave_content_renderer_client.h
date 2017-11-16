// Copyright (c) 2017 The Brave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BRAVE_RENDERER_BRAVE_CONTENT_RENDERER_CLIENT_H_
#define BRAVE_RENDERER_BRAVE_CONTENT_RENDERER_CLIENT_H_

#include "chrome/renderer/chrome_content_renderer_client.h"

class BraveContentRendererClient : public ChromeContentRendererClient {
 public:
  BraveContentRendererClient();
  ~BraveContentRendererClient() override;

 private:
  DISALLOW_COPY_AND_ASSIGN(BraveContentRendererClient);
};

#endif  // BRAVE_RENDERER_BRAVE_CONTENT_RENDERER_CLIENT_H_
