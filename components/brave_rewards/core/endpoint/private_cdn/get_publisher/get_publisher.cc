/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */
#include "brave/components/brave_rewards/core/endpoint/private_cdn/get_publisher/get_publisher.h"

#include <string_view>
#include <utility>

#include "base/strings/strcat.h"
#include "base/strings/string_util.h"
#include "brave/components/brave_private_cdn/private_cdn_helper.h"
#include "brave/components/brave_rewards/core/common/brotli_util.h"
#include "brave/components/brave_rewards/core/common/environment_config.h"
#include "brave/components/brave_rewards/core/common/time_util.h"
#include "brave/components/brave_rewards/core/common/url_loader.h"
#include "brave/components/brave_rewards/core/publisher/protos/channel_response.pb.h"
#include "brave/components/brave_rewards/core/rewards_engine.h"
#include "net/base/load_flags.h"
#include "net/http/http_status_code.h"

// Due to privacy concerns, the request length must be consistent
// for all publisher lookups. Do not add URL parameters or headers
// whose size will vary depending on the publisher key.

namespace brave_rewards::internal {

namespace {

mojom::PublisherBannerPtr GetPublisherBannerFromMessage(
    const publishers_pb::SiteBannerDetails& banner_details) {
  auto banner = mojom::PublisherBanner::New();

  banner->title = banner_details.title();
  banner->description = banner_details.description();

  if (!banner_details.background_url().empty()) {
    banner->background =
        "chrome://rewards-image/" + banner_details.background_url();
  }

  if (!banner_details.logo_url().empty()) {
    banner->logo = "chrome://rewards-image/" + banner_details.logo_url();
  }

  if (banner_details.has_social_links()) {
    auto& links = banner_details.social_links();
    if (!links.youtube().empty()) {
      banner->links.insert(std::make_pair("youtube", links.youtube()));
    }
    if (!links.twitter().empty()) {
      banner->links.insert(std::make_pair("twitter", links.twitter()));
    }
    if (!links.twitch().empty()) {
      banner->links.insert(std::make_pair("twitch", links.twitch()));
    }
  }

  if (!banner_details.web3_url().empty()) {
    banner->web3_url = banner_details.web3_url();
  }

  return banner;
}

void GetPublisherStatusFromMessage(
    const publishers_pb::ChannelResponse& response,
    mojom::ServerPublisherInfo* info) {
  DCHECK(info);
  info->status = mojom::PublisherStatus::NOT_VERIFIED;
  for (const auto& wallet : response.wallets()) {
    if (wallet.has_uphold_wallet()) {
      auto& uphold = wallet.uphold_wallet();
      if (uphold.wallet_state() == publishers_pb::UPHOLD_ACCOUNT_KYC &&
          !uphold.address().empty()) {
        info->status = mojom::PublisherStatus::UPHOLD_VERIFIED;
        info->address = uphold.address();
        return;
      }
    }
    if (wallet.has_bitflyer_wallet()) {
      auto& bitflyer = wallet.bitflyer_wallet();
      if (bitflyer.wallet_state() == publishers_pb::BITFLYER_ACCOUNT_KYC &&
          !bitflyer.address().empty()) {
        info->status = mojom::PublisherStatus::BITFLYER_VERIFIED;
        info->address = bitflyer.address();
        return;
      }
    }
    if (wallet.has_gemini_wallet()) {
      auto& gemini = wallet.gemini_wallet();
      if (gemini.wallet_state() == publishers_pb::GEMINI_ACCOUNT_KYC &&
          !gemini.address().empty()) {
        info->status = mojom::PublisherStatus::GEMINI_VERIFIED;
        info->address = gemini.address();
        return;
      }
    }
  }
  if (!response.site_banner_details().web3_url().empty()) {
    info->status = mojom::PublisherStatus::WEB3_ENABLED;
    return;
  }
}

void GetServerInfoForEmptyResponse(const std::string& publisher_key,
                                   mojom::ServerPublisherInfo* info) {
  DCHECK(info);
  info->publisher_key = publisher_key;
  info->status = mojom::PublisherStatus::NOT_VERIFIED;
  info->updated_at = util::GetCurrentTimeStamp();
}

mojom::Result ServerPublisherInfoFromMessage(
    const publishers_pb::ChannelResponseList& message,
    const std::string& expected_key,
    mojom::ServerPublisherInfo* info) {
  DCHECK(info);

  if (expected_key.empty()) {
    return mojom::Result::FAILED;
  }

  for (const auto& entry : message.channel_responses()) {
    if (entry.channel_identifier() != expected_key) {
      continue;
    }

    info->publisher_key = entry.channel_identifier();
    info->updated_at = util::GetCurrentTimeStamp();
    GetPublisherStatusFromMessage(entry, info);

    if (entry.has_site_banner_details()) {
      info->banner = GetPublisherBannerFromMessage(entry.site_banner_details());
    }
    return mojom::Result::OK;
  }

  return mojom::Result::FAILED;
}

bool DecompressMessage(std::string_view payload, std::string* output) {
  constexpr size_t buffer_size = 32 * 1024;
  return util::DecodeBrotliStringWithBuffer(payload, buffer_size, output);
}

}  // namespace

namespace endpoint::private_cdn {

GetPublisher::GetPublisher(RewardsEngine& engine) : engine_(engine) {}

GetPublisher::~GetPublisher() = default;

std::string GetPublisher::GetUrl(const std::string& hash_prefix) {
  return engine_->Get<EnvironmentConfig>()
      .brave_pcdn_url()
      .Resolve(base::StrCat(
          {"/publishers/prefixes/", base::ToLowerASCII(hash_prefix)}))
      .spec();
}

mojom::Result GetPublisher::CheckStatusCode(const int status_code) {
  if (status_code == net::HTTP_NOT_FOUND) {
    return mojom::Result::NOT_FOUND;
  }

  if (status_code != net::HTTP_OK) {
    engine_->LogError(FROM_HERE) << "Unexpected HTTP status: " << status_code;
    return mojom::Result::FAILED;
  }

  return mojom::Result::OK;
}

mojom::Result GetPublisher::ParseBody(const std::string& body,
                                      const std::string& publisher_key,
                                      mojom::ServerPublisherInfo* info) {
  DCHECK(info);

  if (body.empty()) {
    engine_->LogError(FROM_HERE) << "Publisher data empty";
    return mojom::Result::FAILED;
  }

  std::string_view body_payload(body.data(), body.size());
  if (!brave::PrivateCdnHelper::GetInstance()->RemovePadding(&body_payload)) {
    engine_->LogError(FROM_HERE)
        << "Publisher data response has invalid padding";
    return mojom::Result::FAILED;
  }

  std::string message_string;
  if (!DecompressMessage(body_payload, &message_string)) {
    engine_->Log(FROM_HERE) << "Error decompressing publisher data response. "
                               "Attempting to parse as uncompressed message.";
    message_string.assign(body_payload.data(), body_payload.size());
  }

  publishers_pb::ChannelResponseList message;
  if (!message.ParseFromString(message_string)) {
    engine_->LogError(FROM_HERE)
        << "Error parsing publisher data protobuf message";
    return mojom::Result::FAILED;
  }

  auto result = ServerPublisherInfoFromMessage(message, publisher_key, info);
  if (result != mojom::Result::OK) {
    GetServerInfoForEmptyResponse(publisher_key, info);
  }

  return mojom::Result::OK;
}

void GetPublisher::Request(const std::string& publisher_key,
                           const std::string& hash_prefix,
                           GetPublisherCallback callback) {
  auto request = mojom::UrlRequest::New();
  request->url = GetUrl(hash_prefix);
  request->load_flags = net::LOAD_BYPASS_CACHE | net::LOAD_DISABLE_CACHE;

  engine_->Get<URLLoader>().Load(
      std::move(request), URLLoader::LogLevel::kDetailed,
      base::BindOnce(&GetPublisher::OnRequest, base::Unretained(this),
                     publisher_key, std::move(callback)));
}

void GetPublisher::OnRequest(const std::string& publisher_key,
                             GetPublisherCallback callback,
                             mojom::UrlResponsePtr response) {
  DCHECK(response);
  auto result = CheckStatusCode(response->status_code);

  auto info = mojom::ServerPublisherInfo::New();
  if (result == mojom::Result::NOT_FOUND) {
    GetServerInfoForEmptyResponse(publisher_key, info.get());
    std::move(callback).Run(mojom::Result::OK, std::move(info));
    return;
  }

  if (result != mojom::Result::OK) {
    std::move(callback).Run(mojom::Result::FAILED, nullptr);
    return;
  }

  result = ParseBody(response->body, publisher_key, info.get());

  if (result != mojom::Result::OK) {
    std::move(callback).Run(result, nullptr);
    return;
  }

  std::move(callback).Run(mojom::Result::OK, std::move(info));
}

}  // namespace endpoint::private_cdn

}  // namespace brave_rewards::internal
