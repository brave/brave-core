// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/brave_screenshots/screenshots_tab_feature.h"

#include <memory>
#include <string>
#include <utility>

#include "base/base64.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "brave/browser/brave_screenshots/screenshots_utils.h"
#include "chrome/browser/image_editor/screenshot_flow.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/public/tab_features.h"
#include "chrome/browser/ui/tabs/public/tab_interface.h"
#include "content/public/browser/devtools_agent_host.h"
#include "content/public/browser/devtools_agent_host_client.h"
#include "content/public/browser/web_contents.h"

using content::DevToolsAgentHost;
using content::DevToolsAgentHostClient;
using content::WebContents;

using image_editor::ScreenshotCaptureCallback;
using image_editor::ScreenshotCaptureResult;
using image_editor::ScreenshotFlow;

namespace {
class DevToolsAgentHostClientImpl : public DevToolsAgentHostClient {
 public:
  explicit DevToolsAgentHostClientImpl(base::WeakPtr<WebContents> web_contents,
                                       ScreenshotCaptureCallback callback)
      : callback_(std::move(callback)), web_contents_(web_contents) {}

  void DispatchProtocolMessage(DevToolsAgentHost* host,
                               base::span<const uint8_t> message) override {
    if (!web_contents_) {
      return;
    }

    std::string response(message.begin(), message.end());
    std::optional<base::Value> value = base::JSONReader::Read(response);

    if (!value || !value->is_dict()) {
      return;
    }

    std::string decoded_png;
    const std::string* encoded_png =
        value->GetDict().FindStringByDottedPath("result.data");

    if (!encoded_png || !base::Base64Decode(*encoded_png, &decoded_png)) {
      return;
    }

    ScreenshotCaptureResult result;
    result.image = gfx::Image::CreateFrom1xPNGBytes(
        base::as_bytes(base::make_span(decoded_png)));

    if (callback_) {
      std::move(callback_).Run(result);
    }
  }

  void AgentHostClosed(DevToolsAgentHost* host) override {
    web_contents_ = nullptr;
  }

 private:
  ScreenshotCaptureCallback callback_;
  base::WeakPtr<WebContents> web_contents_ = nullptr;
};

}  // namespace

namespace brave_screenshots {

BraveScreenshotsTabFeature::BraveScreenshotsTabFeature(
    WebContents* web_contents)
    : ScreenshotFlow(web_contents) {
  weak_this_ = weak_factory_.GetWeakPtr();
}

BraveScreenshotsTabFeature::~BraveScreenshotsTabFeature() {
  if (devtools_host_) {
    devtools_host_->DetachAllClients();
    devtools_client_.reset();
  }
}

void BraveScreenshotsTabFeature::StartScreenshot(Browser* browser,
                                                 ScreenshotType type) {
  if (!browser) {
    return;
  }

  browser_ = browser->AsWeakPtr();

  ScreenshotCaptureCallback callback = base::BindOnce(
      &BraveScreenshotsTabFeature::OnCaptureComplete, weak_this_);

  switch (type) {
    case ScreenshotType::kSelection:
      ScreenshotFlow::Start(std::move(callback));
      break;
    case ScreenshotType::kViewport:
      ScreenshotFlow::StartFullscreenCapture(std::move(callback));
      break;
    case ScreenshotType::kFullPage: {
      if (!InitDevToolsHelper(std::move(callback))) {
        // Need to plan what to do if this fails. Ideally we would know when the
        // browser is launched whether or not this feature is available. This
        // would enable us to dim it in the context menu, or not include it at
        // all.
        return;
      }

      SendCaptureFullscreenCommand();
      break;
    }
    default:
      NOTREACHED();
  }
}

bool BraveScreenshotsTabFeature::InitDevToolsHelper(
    ScreenshotCaptureCallback callback) {
  devtools_host_ = DevToolsAgentHost::GetOrCreateFor(web_contents());
  devtools_client_ = std::make_unique<DevToolsAgentHostClientImpl>(
      web_contents()->GetWeakPtr(), std::move(callback));

  if (devtools_host_->AttachClient(devtools_client_.get())) {
    return true;
  }

  devtools_host_ = nullptr;
  devtools_client_.reset();
  return false;
}

void BraveScreenshotsTabFeature::SendCaptureFullscreenCommand() {
  // Construct a JSON command to capture the full page screenshot
  // https://chromedevtools.github.io/devtools-protocol/tot/Page/#method-captureScreenshot
  std::string json_command;
  auto command = base::Value::Dict()
                     .Set("id", 1)
                     .Set("method", "Page.captureScreenshot")
                     .Set("params", base::Value::Dict().Set(
                                        "captureBeyondViewport", true));

  base::JSONWriter::Write(command, &json_command);

  devtools_host_->DispatchProtocolMessage(
      devtools_client_.get(), base::as_bytes(base::make_span(json_command)));
}

void BraveScreenshotsTabFeature::OnCaptureComplete(
    const ScreenshotCaptureResult& result) {
  utils::CopyImageToClipboard(result);
  utils::DisplayScreenshotBubble(result, browser_);
}

}  // namespace brave_screenshots
