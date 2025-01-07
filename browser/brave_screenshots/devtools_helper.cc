// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/brave_screenshots/devtools_helper.h"

#include <memory>
#include <string>
#include <utility>

#include "base/base64.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "content/public/browser/devtools_agent_host.h"
#include "content/public/browser/devtools_agent_host_client.h"
#include "content/public/browser/web_contents.h"
#include "ui/gfx/image/image.h"

namespace brave_screenshots {

class DevToolsHelper::DevToolsClientImpl
    : public content::DevToolsAgentHostClient {
 public:
  DevToolsClientImpl(base::WeakPtr<content::WebContents> web_contents,
                     image_editor::ScreenshotCaptureCallback callback)
      : web_contents_(web_contents), callback_(std::move(callback)) {}

  ~DevToolsClientImpl() override = default;

  void DispatchProtocolMessage(content::DevToolsAgentHost* host,
                               base::span<const uint8_t> message) override {
    if (!web_contents_) {
      return;
    }

    std::string response_str(message.begin(), message.end());
    std::optional<base::Value> value = base::JSONReader::Read(response_str);
    if (!value || !value->is_dict()) {
      return;
    }

    const std::string* encoded_png =
        value->GetDict().FindStringByDottedPath("result.data");
    if (!encoded_png) {
      return;
    }

    std::string decoded_png;
    if (!base::Base64Decode(*encoded_png, &decoded_png)) {
      return;
    }

    image_editor::ScreenshotCaptureResult result;
    result.image = gfx::Image::CreateFrom1xPNGBytes(
        base::as_bytes(base::make_span(decoded_png)));

    if (callback_) {
      std::move(callback_).Run(result);
    }
  }

  void AgentHostClosed(content::DevToolsAgentHost* host) override {
    web_contents_ = nullptr;
  }

 private:
  base::WeakPtr<content::WebContents> web_contents_;
  image_editor::ScreenshotCaptureCallback callback_;
};

DevToolsHelper::DevToolsHelper(base::WeakPtr<content::WebContents> web_contents,
                               image_editor::ScreenshotCaptureCallback callback)
    : devtools_host_(
          content::DevToolsAgentHost::GetOrCreateFor(web_contents.get())),
      devtools_client_(
          std::make_unique<DevToolsClientImpl>(web_contents,
                                               std::move(callback))) {}

DevToolsHelper::~DevToolsHelper() {
  if (devtools_host_ && devtools_client_) {
    devtools_host_->DetachClient(devtools_client_.get());
  }
}

bool DevToolsHelper::Attach() {
  if (!devtools_host_ || !devtools_client_) {
    return false;
  }

  if (!devtools_host_->AttachClient(devtools_client_.get())) {
    devtools_host_ = nullptr;
    devtools_client_.reset();
    return false;
  }

  return true;
}

void DevToolsHelper::SendCaptureFullscreenCommand() {
  if (!devtools_host_ || !devtools_client_) {
    return;
  }

  // Construct a JSON command to capture the full page screenshot
  // https://chromedevtools.github.io/devtools-protocol/tot/Page/#method-captureScreenshot
  std::string json_command;
  auto command = base::Value::Dict()
                     .Set("id", 1)  // Arbitrary id ('id' is required)
                     .Set("method", "Page.captureScreenshot")
                     .Set("params", base::Value::Dict().Set(
                                        "captureBeyondViewport", true));

  base::JSONWriter::Write(command, &json_command);

  devtools_host_->DispatchProtocolMessage(
      devtools_client_.get(), base::as_bytes(base::make_span(json_command)));
}

}  // namespace brave_screenshots
