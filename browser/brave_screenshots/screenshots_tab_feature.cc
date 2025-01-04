// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/brave_screenshots/screenshots_tab_feature.h"

#include <memory>
#include <string>

#include "base/base64.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "brave/app/brave_command_ids.h"
#include "brave/browser/brave_screenshots/screenshots_utils.h"
#include "chrome/browser/image_editor/screenshot_flow.h"
#include "chrome/browser/ui/tabs/public/tab_features.h"
#include "chrome/browser/ui/tabs/public/tab_interface.h"
#include "content/public/browser/devtools_agent_host.h"
#include "content/public/browser/devtools_agent_host_client.h"
#include "content/public/browser/web_contents.h"

namespace {

class DevToolsAgentHostClientImpl : public content::DevToolsAgentHostClient {
 public:
  explicit DevToolsAgentHostClientImpl(
      base::WeakPtr<content::WebContents> web_contents)
      : web_contents_(web_contents) {}

  void DispatchProtocolMessage(content::DevToolsAgentHost* host,
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

    image_editor::ScreenshotCaptureResult result;
    result.image = gfx::Image::CreateFrom1xPNGBytes(
        base::as_bytes(base::make_span(decoded_png)));

    brave_screenshots::utils::NotifyUserOfScreenshot(result, web_contents_);
  }

  void AgentHostClosed(content::DevToolsAgentHost* host) override {
    web_contents_ = nullptr;
  }

 private:
  base::WeakPtr<content::WebContents> web_contents_ = nullptr;
};

}  // namespace

namespace brave_screenshots {

void TakeScreenshot(base::WeakPtr<content::WebContents> web_contents,
                    int command_id) {
  if (!web_contents) {
    return;
  }

  tabs::TabInterface* tab =
      tabs::TabInterface::GetFromContents(web_contents.get());

  if (!tab) {
    return;
  }

  tabs::TabFeatures* features = tab->GetTabFeatures();

  if (!features) {
    return;
  }

  auto* helper = features->brave_screenshots_tab_feature();

  if (!helper) {
    return;
  }

  switch (command_id) {
    case IDC_BRAVE_SCREENSHOTS_START_SELECTION_TO_CLIPBOARD:
      helper->Start();
      break;
    case IDC_BRAVE_SCREENSHOTS_START_VIEWPORT_TO_CLIPBOARD:
      helper->StartFullscreenCapture();
      break;
    case IDC_BRAVE_SCREENSHOTS_START_FULLPAGE_TO_CLIPBOARD:
      helper->StartScreenshotFullPageToClipboard();
      break;
    default:
      NOTREACHED();
  }
}

BraveScreenshotsTabFeature::BraveScreenshotsTabFeature(
    content::WebContents* web_contents)
    : image_editor::ScreenshotFlow(web_contents) {
  weak_this_ = weak_factory_.GetWeakPtr();
}

BraveScreenshotsTabFeature::~BraveScreenshotsTabFeature() {
  if (devtools_agent_host_) {
    devtools_agent_host_->DetachClient(devtools_agent_host_client_.get());
    devtools_agent_host_client_.reset();
  }
}

void BraveScreenshotsTabFeature::Start() {
  image_editor::ScreenshotFlow::Start(base::BindOnce(
      &BraveScreenshotsTabFeature::OnCaptureComplete, weak_this_));
}

void BraveScreenshotsTabFeature::StartFullscreenCapture() {
  image_editor::ScreenshotFlow::StartFullscreenCapture(base::BindOnce(
      &BraveScreenshotsTabFeature::OnCaptureComplete, weak_this_));
}

void BraveScreenshotsTabFeature::StartScreenshotFullPageToClipboard() {
  if (!InitializeDevToolsAgentHost()) {
    // Need to plan what to do if this fails. Ideally we would know when the
    // browser is launched whether or not this feature is available. This would
    // enable us to dim it in the context menu, or not include it at all.
    return;
  }

  SendCaptureFullscreenCommand();
}

bool BraveScreenshotsTabFeature::InitializeDevToolsAgentHost() {
  if (devtools_agent_host_) {
    return true;
  }

  devtools_agent_host_ =
      content::DevToolsAgentHost::GetOrCreateFor(web_contents());

  devtools_agent_host_client_ = std::make_unique<DevToolsAgentHostClientImpl>(
      web_contents()->GetWeakPtr());

  if (!devtools_agent_host_ ||
      !devtools_agent_host_->AttachClient(devtools_agent_host_client_.get())) {
    // This may not succeed (e.g. if restricted by a policy).
    devtools_agent_host_client_.reset();
    return false;
  }

  return true;
}

void BraveScreenshotsTabFeature::SendCaptureFullscreenCommand() {
  if (!devtools_agent_host_ || !devtools_agent_host_client_) {
    return;
  }

  // Construct a JSON command to capture the full page screenshot
  // https://chromedevtools.github.io/devtools-protocol/tot/Page/#method-captureScreenshot
  std::string json_command;
  auto command = base::Value::Dict()
                     .Set("id", 1)
                     .Set("method", "Page.captureScreenshot")
                     .Set("params", base::Value::Dict().Set(
                                        "captureBeyondViewport", true));

  base::JSONWriter::Write(command, &json_command);

  devtools_agent_host_->DispatchProtocolMessage(
      devtools_agent_host_client_.get(),
      base::as_bytes(base::make_span(json_command)));
}

void BraveScreenshotsTabFeature::OnCaptureComplete(
    const image_editor::ScreenshotCaptureResult& result) {
  brave_screenshots::utils::NotifyUserOfScreenshot(
      result, web_contents()->GetWeakPtr());
}

}  // namespace brave_screenshots
