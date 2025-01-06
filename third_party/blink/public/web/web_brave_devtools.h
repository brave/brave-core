// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_THIRD_PARTY_BLINK_PUBLIC_WEB_WEB_BRAVE_DEVTOOLS_H_
#define BRAVE_THIRD_PARTY_BLINK_PUBLIC_WEB_WEB_BRAVE_DEVTOOLS_H_

#include "base/memory/raw_ptr.h"
#include "base/values.h"
#include "third_party/blink/public/platform/web_common.h"

namespace blink {

class WebString;
class WebLocalFrame;

class BLINK_EXPORT WebBraveDevtoolsClient {
 public:
  explicit WebBraveDevtoolsClient(WebLocalFrame* local_frame);
  virtual ~WebBraveDevtoolsClient();

 protected:
  bool IsBraveDevtoolsEnabled();

  void SendBraveDevtoolsCommand(const WebString& command,
                                const base::Value::Dict& params);

  virtual void HandleBraveDevtoolsMessage(const WebString& message,
                                          const base::Value::Dict& params) = 0;

 private:
  friend class WebBraveDevtoolsSink;

  void BraveDevtoolsEnabled(bool enabled);

  raw_ptr<WebLocalFrame> local_frame_ = nullptr;
  bool brave_devtools_enabled_ = false;
};

}  // namespace blink

#endif  // BRAVE_THIRD_PARTY_BLINK_PUBLIC_WEB_WEB_BRAVE_DEVTOOLS_H_
