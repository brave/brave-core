/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_TALK_BROWSER_BRAVE_TALK_FRAME_HOST_H_
#define BRAVE_COMPONENTS_BRAVE_TALK_BROWSER_BRAVE_TALK_FRAME_HOST_H_

#include <string>

#include "base/memory/raw_ptr.h"
#include "brave/components/brave_talk/common/brave_talk_frame.mojom.h"
#include "content/public/browser/web_contents.h"

namespace content {
class BrowserContext;
class WebContents;
}  // namespace content

namespace brave_talk {

class BraveTalkFrameHost final
    : public brave_talk::mojom::BraveTalkFrame {
 public:
  BraveTalkFrameHost(const BraveTalkFrameHost&) = delete;
  BraveTalkFrameHost& operator=(const BraveTalkFrameHost&) = delete;

  explicit BraveTalkFrameHost(content::WebContents* contents,
                                  const std::string& host);
  ~BraveTalkFrameHost() override;

  // brave_talk::mojom::BraveTalkAdvertise:
  void BeginAdvertiseShareDisplayMedia(
      BeginAdvertiseShareDisplayMediaCallback callback) override;

 private:
  base::raw_ptr<content::WebContents> contents_;
  const std::string host_;
};

}  // namespace brave_talk

#endif  // BRAVE_COMPONENTS_BRAVE_TALK_BROWSER_BRAVE_TALK_FRAME_HOST_H_
