// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/screenshots/brave_screenshots_utils.h"

#include <string>
#include <utility>

#include "base/base64.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "chrome/browser/image_editor/screenshot_flow.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/sharing_hub/screenshot/screenshot_captured_bubble_controller.h"
#include "content/public/browser/devtools_agent_host.h"
#include "content/public/browser/devtools_agent_host_client.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "ui/gfx/codec/png_codec.h"

namespace {

class FullPageScreenshotDevToolsClient
    : public content::DevToolsAgentHostClient {
 public:
  FullPageScreenshotDevToolsClient(
      content::WebContents* web_contents,
      base::OnceCallback<void(const SkBitmap&)> callback)
      : command_id_(1),
        web_contents_(web_contents),
        callback_(std::move(callback)) {
    agent_host_ = content::DevToolsAgentHost::GetOrCreateFor(web_contents_);
    agent_host_->AttachClient(this);
    SendCaptureScreenshotCommand();
  }

  ~FullPageScreenshotDevToolsClient() override {
    if (agent_host_.get() && agent_host_->IsAttached()) {
      agent_host_->DetachClient(this);
    }
  }

  void DispatchProtocolMessage(content::DevToolsAgentHost* agent_host,
                               base::span<const uint8_t> message) override {
    std::string str_message(message.begin(), message.end());

    std::optional<base::Value> value = base::JSONReader::Read(str_message);
    if (!value || !value->is_dict()) {
      return;
    }

    base::Value::Dict& dict = value->GetDict();

    if (dict.FindInt("id").value_or(0) == command_id_) {
      SkBitmap bitmap;
      ExtractBitmap(bitmap, dict);
      std::move(callback_).Run(bitmap);
      delete this;
    }
  }

  SkBitmap DecodeBase64PNG(const std::string& base64_png) {
    std::string png_data;
    if (!base::Base64Decode(base64_png, &png_data)) {
      return SkBitmap();
    }

    return gfx::PNGCodec::Decode(base::as_bytes(base::make_span(png_data)));
  }

  void AgentHostClosed(content::DevToolsAgentHost* agent_host) override {
    if (callback_) {
      SkBitmap empty;
      std::move(callback_).Run(empty);
    }
    delete this;
  }

  void ExtractBitmap(SkBitmap& bitmap, const base::Value::Dict& dict) {
    const std::string* data = dict.FindStringByDottedPath("result.data");
    bitmap = data ? DecodeBase64PNG(*data) : SkBitmap();
  }

 private:
  void SendCaptureScreenshotCommand() {
    auto command = base::Value::Dict()
                       .Set("id", command_id_)
                       .Set("method", "Page.captureScreenshot")
                       .Set("params", base::Value::Dict().Set(
                                          "captureBeyondViewport", true));

    std::string json_command;
    base::JSONWriter::Write(command, &json_command);
    agent_host_->DispatchProtocolMessage(
        this, base::as_bytes(base::make_span(json_command)));
  }

  int command_id_ = 0;
  raw_ptr<content::WebContents> web_contents_;
  scoped_refptr<content::DevToolsAgentHost> agent_host_;
  base::OnceCallback<void(const SkBitmap&)> callback_;
};

}  // namespace

namespace brave_utils {

void ScreenshotSelectionToClipboard(base::WeakPtr<Browser> browser) {
  if (!browser) {
    return;
  }

  sharing_hub::ScreenshotCapturedBubbleController::Get(
      browser->tab_strip_model()->GetActiveWebContents())
      ->Capture(browser.get());
}

void ScreenshotViewportToClipboard(
    base::WeakPtr<content::WebContents> web_contents) {
  content::WebContents* wc = web_contents.get();
  if (!wc) {
    return;
  }

  image_editor::ScreenshotFlow screenshot_flow(wc);

  base::OnceCallback<void(const image_editor::ScreenshotCaptureResult&)>
      callback = base::BindOnce(
          [](content::WebContents* web_contents,
             const image_editor::ScreenshotCaptureResult& image) {
            if (image.image.IsEmpty() || !web_contents) {
              return;
            }
            sharing_hub::ScreenshotCapturedBubbleController::Get(web_contents)
                ->ShowBubble(image);
          },
          wc);

  screenshot_flow.StartFullscreenCapture(std::move(callback));
}

void ScreenshotFullPageToClipboard(
    base::WeakPtr<content::WebContents> web_contents) {
  content::WebContents* wc = web_contents.get();
  if (!wc) {
    return;
  }

  new FullPageScreenshotDevToolsClient(
      wc,
      base::BindOnce(
          [](content::WebContents* web_contents, const SkBitmap& image) {
            if (image.empty() || !web_contents) {
              return;
            }
            image_editor::ScreenshotCaptureResult result;
            result.image = gfx::Image::CreateFrom1xBitmap(image);
            sharing_hub::ScreenshotCapturedBubbleController::Get(web_contents)
                ->ShowBubble(result);
          },
          wc));
}

}  // namespace brave_utils
