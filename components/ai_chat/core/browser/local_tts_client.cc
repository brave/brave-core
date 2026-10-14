// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/browser/local_tts_client.h"

#include <optional>
#include <utility>

#include "base/base64.h"
#include "base/functional/bind.h"
#include "base/json/json_writer.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/time/time.h"
#include "base/values.h"
#include "brave/components/api_request_helper/api_request_helper.h"
#include "net/base/net_errors.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/simple_url_loader.h"
#include "url/gurl.h"

namespace ai_chat {
namespace {

constexpr net::NetworkTrafficAnnotationTag kTrafficAnnotation =
    net::DefineNetworkTrafficAnnotation("leo_local_tts", R"(
      semantics {
        sender: "Leo local text to speech"
        description: "Synthesizes Leo replies using a local Pocket TTS server."
        trigger: "A reply completes while the user has enabled voice mode."
        data: "The text of Leo's reply."
        destination: LOCAL
        internal { contacts { email: "support@brave.com" } }
        user_data { type: OTHER }
        last_reviewed: "2026-10-13"
      }
      policy {
        cookies_allowed: NO
        setting: "Only used while Leo voice mode is enabled."
        policy_exception_justification: "Local development integration."
      })");

// APIRequestHelper decodes JSON, so wrap the binary response for its decoder.
std::optional<std::string> ConvertAudio(const std::string& raw) {
  if (raw.empty() || raw.size() % sizeof(float) != 0) {
    return std::nullopt;
  }
  return base::WriteJson(
      base::DictValue().Set("audio", base::Base64Encode(raw)));
}

void OnSynthesized(LocalTTSClient::SynthesizeCallback callback,
                   api_request_helper::APIRequestResult result) {
  if (result.error_code() == net::ERR_INSUFFICIENT_RESOURCES) {
    std::move(callback).Run({}, 0, "tts-audio-too-large");
    return;
  }
  const auto* body = result.value_body().GetIfDict();
  const std::string* encoded = body ? body->FindString("audio") : nullptr;
  auto rate_header = result.headers().find("x-sample-rate");
  uint32_t sample_rate = 0;
  std::string audio;
  if (result.error_code() != net::OK || !result.Is2XXResponseCode() ||
      result.final_url() != GURL("http://127.0.0.1:3002/tts-stream") ||
      !encoded || rate_header == result.headers().end() ||
      !base::StringToUint(rate_header->second, &sample_rate) ||
      sample_rate < 8000 || sample_rate > 96000 ||
      !base::Base64Decode(*encoded, &audio) || audio.empty() ||
      audio.size() % sizeof(float) != 0) {
    std::move(callback).Run({}, 0, "Local TTS request failed");
    return;
  }
  std::move(callback).Run(std::vector<uint8_t>(audio.begin(), audio.end()),
                          sample_rate, "");
}

}  // namespace

LocalTTSClient::LocalTTSClient(
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory)
    : request_helper_(std::make_unique<api_request_helper::APIRequestHelper>(
          kTrafficAnnotation,
          std::move(url_loader_factory))) {}

LocalTTSClient::~LocalTTSClient() = default;

void LocalTTSClient::Synthesize(const std::string& text,
                                SynthesizeCallback callback) {
  std::string trimmed;
  base::TrimWhitespaceASCII(text, base::TRIM_ALL, &trimmed);
  if (trimmed.empty() || trimmed.size() > 32000) {
    std::move(callback).Run({}, 0, "Invalid TTS text");
    return;
  }
  api_request_helper::APIRequestOptions options;
  options.max_body_size =
      network::SimpleURLLoader::kMaxBoundedStringDownloadSize;
  options.timeout = base::Minutes(2);
  request_helper_->Request(
      "POST", GURL("http://127.0.0.1:3002/tts-stream"),
      base::WriteJson(base::DictValue().Set("text", trimmed)).value(),
      "application/json", base::BindOnce(&OnSynthesized, std::move(callback)),
      {}, options, base::BindOnce(&ConvertAudio));
}

void LocalTTSClient::Cancel() {
  request_helper_->CancelAll();
}

}  // namespace ai_chat
