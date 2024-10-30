// Copyright 2018 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_PUBLIC_PLATFORM_WEB_BRAVE_DEVTOOLS_H_
#define THIRD_PARTY_BLINK_PUBLIC_PLATFORM_WEB_BRAVE_DEVTOOLS_H_

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

#endif  // THIRD_PARTY_BLINK_PUBLIC_PLATFORM_WEB_BRAVE_DEVTOOLS_H_
