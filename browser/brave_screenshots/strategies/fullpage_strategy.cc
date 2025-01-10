// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/brave_screenshots/strategies/fullpage_strategy.h"

#include <string>
#include <utility>

#include "base/base64.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/logging.h"
#include "base/values.h"
#include "chrome/browser/image_editor/screenshot_flow.h"
#include "content/public/browser/devtools_agent_host.h"
#include "content/public/browser/devtools_agent_host_client.h"
#include "content/public/browser/web_contents.h"
#include "ui/gfx/image/image.h"

namespace {
constexpr int kMaxDimensions = 16384;  // 16k limit for GPU textures
}  // namespace

namespace brave_screenshots {

FullPageStrategy::FullPageStrategy() = default;

FullPageStrategy::~FullPageStrategy() {
  DVLOG(2) << __func__;
  // If anything is still attached, tear it down
  if (devtools_host_ && devtools_host_->IsAttached()) {
    devtools_host_->DetachClient(this);
  }
}

void FullPageStrategy::Capture(
    content::WebContents* web_contents,
    image_editor::ScreenshotCaptureCallback callback) {
  // Store the WebContents and callback
  web_contents_ = web_contents->GetWeakPtr();
  callback_ = std::move(callback);

  // Attach to the DevToolsAgentHost
  devtools_host_ = content::DevToolsAgentHost::GetOrCreateFor(web_contents);
  devtools_host_->AttachClient(this);

  // Step 1: Request layout metrics
  RequestPageLayoutMetrics();
}

void FullPageStrategy::RequestPageLayoutMetrics() {
  // https://chromedevtools.github.io/devtools-protocol/tot/Page/#method-getLayoutMetrics
  base::Value::Dict params;  // No parameters needed
  SendDevToolsCommand("Page.getLayoutMetrics", std::move(params), next_id_++);
}

// Response arrives in DispatchProtocolMessage
void FullPageStrategy::OnLayoutMetricsReceived(int width, int height) {
  // Maybe clip dimensions
  if (width > kMaxDimensions) {
    LOG(WARNING) << "Clipping screenshot width to " << kMaxDimensions;
    width = kMaxDimensions;
    screenshot_was_clipped_ = true;
  }

  if (height > kMaxDimensions) {
    LOG(WARNING) << "Clipping screenshot height to " << kMaxDimensions;
    height = kMaxDimensions;
    screenshot_was_clipped_ = true;
  }

  // Check for invalid dimensions
  if (width <= 0 || height <= 0) {
    LOG(ERROR) << "Invalid dimensions from Page.getLayoutMetrics";
    RunCallback({});
    return;
  }

  // Step 2: Having received the layout metrics, request the screenshot
  RequestFullPageScreenshot(width, height);
}

bool FullPageStrategy::DidClipScreenshot() const {
  return screenshot_was_clipped_;
}

void FullPageStrategy::RequestFullPageScreenshot(int width, int height) {
  DVLOG(2) << "Requesting full page screenshot with dimensions: " << width
           << "x" << height;
  // https://chromedevtools.github.io/devtools-protocol/tot/Page/#method-captureScreenshot
  base::Value::Dict clip;
  clip.Set("x", 0);
  clip.Set("y", 0);
  clip.Set("width", width);
  clip.Set("height", height);
  clip.Set("scale", 1);

  base::Value::Dict params;
  params.Set("captureBeyondViewport", true);
  params.Set("clip", std::move(clip));

  SendDevToolsCommand("Page.captureScreenshot", std::move(params), next_id_++);
}

// Actually sends the JSON command
void FullPageStrategy::SendDevToolsCommand(const std::string& command,
                                           base::Value::Dict params,
                                           int command_id) {
  base::Value::Dict message;
  message.Set("id", command_id);
  message.Set("method", command);
  message.Set("params", std::move(params));

  std::string json;
  base::JSONWriter::Write(message, &json);
  devtools_host_->DispatchProtocolMessage(
      this, base::as_bytes(base::make_span(json)));
}

// DevToolsAgentHostClient overrides
void FullPageStrategy::DispatchProtocolMessage(
    content::DevToolsAgentHost* host,
    base::span<const uint8_t> message) {
  // Convert the incoming message to a string
  std::string message_str(message.begin(), message.end());

  // Determine whether a response to "getLayoutMetrics" or "captureScreenshot"
  auto parsed = base::JSONReader::Read(message_str);
  if (!parsed || !parsed->is_dict()) {
    LOG(ERROR) << "Invalid JSON response from DevTools protocol";
    RunCallback({});
    return;
  }

  // Is this a content-size response?
  // https://chromedevtools.github.io/devtools-protocol/tot/Page/#method-getLayoutMetrics
  const base::Value::Dict* css_content_size =
      parsed->GetDict().FindDictByDottedPath("result.cssContentSize");
  if (css_content_size) {
    DVLOG(2) << "Layout metrics received";

    int width = css_content_size->FindInt("width").value_or(0);
    int height = css_content_size->FindInt("height").value_or(0);

    OnLayoutMetricsReceived(width, height);
    return;
  }

  // Is this a screenshot response?
  // https://chromedevtools.github.io/devtools-protocol/tot/Page/#method-captureScreenshot
  const std::string* encoded_png =
      parsed->GetDict().FindStringByDottedPath("result.data");
  if (encoded_png) {
    DVLOG(2) << "Screenshot captured";

    std::string decoded_png;
    image_editor::ScreenshotCaptureResult result;

    DVLOG(2) << "Decoding PNG";
    base::Base64Decode(*encoded_png, &decoded_png);

    DVLOG(2) << "Creating image from PNG";
    result.image = gfx::Image::CreateFrom1xPNGBytes(
        base::as_bytes(base::make_span(decoded_png)));

    RunCallback(result);
    return;
  }

  // If we get here, it's an unknown response
  DLOG(WARNING) << "Unknown/Unhandled DevTools response: " << message_str;

  RunCallback({});
}

void FullPageStrategy::AgentHostClosed(content::DevToolsAgentHost* host) {
  DVLOG(2) << __func__;
  RunCallback({});
}

void FullPageStrategy::RunCallback(
    const image_editor::ScreenshotCaptureResult& result) {
  DVLOG(2) << __func__;
  // Run the callback, if it exists
  if (callback_) {
    std::move(callback_).Run(result);
  }

  // Detach from the DevToolsAgentHost
  if (devtools_host_ && devtools_host_->IsAttached()) {
    devtools_host_->DetachClient(this);
  }
}

}  // namespace brave_screenshots
