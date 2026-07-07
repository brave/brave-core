// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_SCREENSHOT_DEVTOOLS_FULL_PAGE_EXTRACTOR_H_
#define BRAVE_BROWSER_UI_SCREENSHOT_DEVTOOLS_FULL_PAGE_EXTRACTOR_H_

#include <memory>
#include <string>
#include <vector>

#include "base/functional/callback.h"
#include "base/memory/weak_ptr.h"
#include "base/types/expected.h"
#include "base/values.h"

namespace content {
class WebContents;
}

namespace simple_devtools_protocol_client {
class SimpleDevToolsProtocolClient;
}

namespace screenshot {

// Parses a CDP Page.captureScreenshot response and returns the decoded PNG
// bytes, or an error string. Exposed for unit testing.
base::expected<std::vector<uint8_t>, std::string> ParseCdpScreenshotResponse(
    const base::DictValue& response);

// Captures the full scrollable content of a page (beyond the visible viewport)
// by issuing a CDP(Chrome DevTools Protocol) Page.captureScreenshot command
// with captureBeyondViewport. The result is a PNG-encoded image as raw bytes.
class DevToolsFullPageExtractor {
 public:
  using ResultCallback = base::OnceCallback<void(
      base::expected<std::vector<uint8_t>, std::string>)>;

  DevToolsFullPageExtractor();
  ~DevToolsFullPageExtractor();
  DevToolsFullPageExtractor(const DevToolsFullPageExtractor&) = delete;
  DevToolsFullPageExtractor& operator=(const DevToolsFullPageExtractor&) =
      delete;

  void CaptureFullPage(content::WebContents* web_contents, ResultCallback done);

 private:
  void OnCaptureResponse(ResultCallback done, base::DictValue response);

  std::unique_ptr<simple_devtools_protocol_client::SimpleDevToolsProtocolClient>
      cdp_client_;

  base::WeakPtrFactory<DevToolsFullPageExtractor> weak_factory_{this};
};

}  // namespace screenshot

#endif  // BRAVE_BROWSER_UI_SCREENSHOT_DEVTOOLS_FULL_PAGE_EXTRACTOR_H_
