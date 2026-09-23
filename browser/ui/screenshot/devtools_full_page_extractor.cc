// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/screenshot/devtools_full_page_extractor.h"

#include <utility>

#include "base/base64.h"
#include "base/functional/bind.h"
#include "base/types/expected.h"
#include "base/values.h"
#include "components/devtools/simple_devtools_protocol_client/simple_devtools_protocol_client.h"
#include "content/public/browser/web_contents.h"

namespace screenshot {

namespace {

constexpr char kCommandCaptureScreenshot[] = "Page.captureScreenshot";
constexpr char kParamFormat[] = "format";
constexpr char kParamCaptureBeyondViewport[] = "captureBeyondViewport";
constexpr char kResponseResult[] = "result";
constexpr char kResponseData[] = "data";

}  // namespace

using simple_devtools_protocol_client::SimpleDevToolsProtocolClient;

base::expected<std::vector<uint8_t>, std::string> ParseCdpScreenshotResponse(
    const base::DictValue& response) {
  const base::DictValue* result = response.FindDict(kResponseResult);
  if (!result) {
    return base::unexpected(std::string("No result in CDP response"));
  }

  const std::string* data = result->FindString(kResponseData);
  if (!data || data->empty()) {
    return base::unexpected(std::string("Missing data in CDP response"));
  }

  std::optional<std::vector<uint8_t>> decoded = base::Base64Decode(*data);
  if (!decoded) {
    return base::unexpected(std::string("Base64 decode failed"));
  }

  return base::ok(std::move(*decoded));
}

DevToolsFullPageExtractor::DevToolsFullPageExtractor() = default;
DevToolsFullPageExtractor::~DevToolsFullPageExtractor() = default;

void DevToolsFullPageExtractor::CaptureFullPage(
    content::WebContents* web_contents,
    ResultCallback done) {
  cdp_client_ = std::make_unique<SimpleDevToolsProtocolClient>();
  cdp_client_->AttachToWebContents(web_contents);

  base::DictValue params;
  params.Set(kParamFormat, "png");
  params.Set(kParamCaptureBeyondViewport, true);

  cdp_client_->SendCommand(
      kCommandCaptureScreenshot, std::move(params),
      base::BindOnce(&DevToolsFullPageExtractor::OnCaptureResponse,
                     weak_factory_.GetWeakPtr(), std::move(done)));
}

void DevToolsFullPageExtractor::OnCaptureResponse(ResultCallback done,
                                                  base::DictValue response) {
  cdp_client_->DetachClient();
  cdp_client_.reset();
  std::move(done).Run(ParseCdpScreenshotResponse(response));
}

}  // namespace screenshot
