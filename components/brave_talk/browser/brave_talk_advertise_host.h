/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_TALK_BROWSER_BRAVE_TALK_ADVERTISE_HOST_H_
#define BRAVE_COMPONENTS_BRAVE_TALK_BROWSER_BRAVE_TALK_ADVERTISE_HOST_H_

#include <string>

#include "base/memory/raw_ptr.h"
#include "brave/components/brave_talk/common/brave_talk_advertise.mojom.h"
#include "content/public/browser/web_contents.h"

namespace content {
class BrowserContext;
class WebContents;
}  // namespace content

namespace brave_talk {

class BraveTalkAdvertiseHost final
    : public brave_talk::mojom::BraveTalkAdvertise {
 public:
  BraveTalkAdvertiseHost(const BraveTalkAdvertiseHost&) = delete;
  BraveTalkAdvertiseHost& operator=(const BraveTalkAdvertiseHost&) = delete;

  explicit BraveTalkAdvertiseHost(content::WebContents* contents,
                                  const std::string& host);
  ~BraveTalkAdvertiseHost() override;

  // brave_talk::mojom::BraveTalkAdvertise:
  void BeginAdvertiseShareDisplayMedia(
      BeginAdvertiseShareDisplayMediaCallback callback) override;

 private:
  base::raw_ptr<content::WebContents> contents_;
  const std::string host_;
};

}  // namespace brave_talk

#endif  // BRAVE_COMPONENTS_BRAVE_TALK_BROWSER_BRAVE_TALK_ADVERTISE_HOST_H_
