/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include "bat/ledger/internal/endpoint/private_cdn/get_publisher/get_publisher.h"

#include <utility>

#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "bat/ledger/internal/common/brotli_helpers.h"
#include "bat/ledger/internal/common/time_util.h"
#include "bat/ledger/internal/endpoint/private_cdn/private_cdn_util.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/internal/publisher/protos/channel_response.pb.h"
#include "brave/components/brave_private_cdn/private_cdn_helper.h"
#include "net/http/http_status_code.h"

using std::placeholders::_1;

// GET /publishers/prefixes/{prefix}
//
// Success code:
// HTTP_OK (200)
//
// Error codes:
// HTTP_NOT_FOUND (404)
//
// Response body:
// See https://github.com/brave/brave-core/blob/master/vendor/bat-native-ledger/src/bat/ledger/internal/publisher/protos/channel_response.proto


// Due to privacy concerns, the request length must be consistent
// for all publisher lookups. Do not add URL parameters or headers
// whose size will vary depending on the publisher key.

using brave::PrivateCdnHelper;

namespace {

ledger::PublisherBannerPtr GetPublisherBannerFromMessage(
    const publishers_pb::SiteBannerDetails& banner_details) {
  auto banner = ledger::PublisherBanner::New();

  banner->title = banner_details.title();
  banner->description = banner_details.description();

  if (!banner_details.background_url().empty()) {
    banner->background =
        "chrome://rewards-image/" + banner_details.background_url();
  }

  if (!banner_details.logo_url().empty()) {
    banner->logo = "chrome://rewards-image/" + banner_details.logo_url();
  }

  for (const auto& amount : banner_details.donation_amounts()) {
    banner->amounts.push_back(amount);
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

  return banner;
}

ledger::PublisherStatus GetPublisherStatusFromMessage(
    const publishers_pb::ChannelResponse& response) {
  auto status = ledger::PublisherStatus::CONNECTED;
  for (const auto& wallet : response.wallets()) {
    if (wallet.has_uphold_wallet()) {
      switch (wallet.uphold_wallet().wallet_state()) {
        case publishers_pb::UPHOLD_ACCOUNT_KYC:
          return ledger::PublisherStatus::VERIFIED;
        default: {}
      }
    }
  }
  return status;
}

std::string GetPublisherAddressFromMessage(
    const publishers_pb::ChannelResponse& response) {
  for (const auto& wallet : response.wallets()) {
    if (wallet.has_uphold_wallet()) {
      return wallet.uphold_wallet().address();
    }
  }
  return "";
}

void GetServerInfoForEmptyResponse(
    const std::string& publisher_key,
    ledger::ServerPublisherInfo* info) {
  DCHECK(info);

  BLOG(1, "Server did not return an entry for publisher " << publisher_key);
  info->publisher_key = publisher_key;
  info->status = ledger::PublisherStatus::NOT_VERIFIED;
  info->updated_at = braveledger_time_util::GetCurrentTimeStamp();
}

ledger::Result ServerPublisherInfoFromMessage(
    const publishers_pb::ChannelResponseList& message,
    const std::string& expected_key,
    ledger::ServerPublisherInfo* info) {
  DCHECK(info);

  if (expected_key.empty()) {
    return ledger::Result::LEDGER_ERROR;
  }

  for (const auto& entry : message.channel_responses()) {
    if (entry.channel_identifier() != expected_key) {
      continue;
    }

    info->publisher_key = entry.channel_identifier();
    info->status = GetPublisherStatusFromMessage(entry);
    info->address = GetPublisherAddressFromMessage(entry);
    info->updated_at = braveledger_time_util::GetCurrentTimeStamp();

    if (entry.has_site_banner_details()) {
      info->banner =
          GetPublisherBannerFromMessage(entry.site_banner_details());
    }
    return ledger::Result::LEDGER_OK;;
  }

  return ledger::Result::LEDGER_ERROR;
}

bool DecompressMessage(base::StringPiece payload, std::string* output) {
  constexpr size_t buffer_size = 32 * 1024;
  return braveledger_helpers::DecodeBrotliStringWithBuffer(
      payload,
      buffer_size,
      output);
}

}  // namespace


namespace ledger {
namespace endpoint {
namespace private_cdn {

GetPublisher::GetPublisher(bat_ledger::LedgerImpl* ledger):
    ledger_(ledger) {
  DCHECK(ledger_);
}

GetPublisher::~GetPublisher() = default;

std::string GetPublisher::GetUrl(const std::string& hash_prefix) {
  const std::string prefix = base::ToLowerASCII(hash_prefix);
  const std::string path =
      base::StringPrintf("/publishers/prefixes/%s", prefix.c_str());
  return GetServerUrl(path);
}

ledger::Result GetPublisher::CheckStatusCode(const int status_code) {
  if (status_code == net::HTTP_NOT_FOUND) {
    return ledger::Result::NOT_FOUND;
  }

  if (status_code != net::HTTP_OK) {
    return ledger::Result::LEDGER_ERROR;
  }

  return ledger::Result::LEDGER_OK;
}

ledger::Result GetPublisher::ParseBody(
    const std::string& body,
    const std::string& publisher_key,
    ledger::ServerPublisherInfo* info) {
  DCHECK(info);

  if (body.empty()) {
    BLOG(0, "Publisher data empty");
    return ledger::Result::LEDGER_ERROR;
  }

  base::StringPiece body_payload(body.data(), body.size());
  if (!PrivateCdnHelper::GetInstance()->RemovePadding(&body_payload)) {
    BLOG(0, "Publisher data response has invalid padding");
    return ledger::Result::LEDGER_ERROR;
  }

  std::string message_string;
  if (!DecompressMessage(body_payload, &message_string)) {
    BLOG(1,
         "Error decompressing publisher data response. "
         "Attempting to parse as uncompressed message.");
    message_string.assign(body_payload.data(), body_payload.size());
  }

  publishers_pb::ChannelResponseList message;
  if (!message.ParseFromString(message_string)) {
    BLOG(0, "Error parsing publisher data protobuf message");
    return ledger::Result::LEDGER_ERROR;
  }

  auto result = ServerPublisherInfoFromMessage(message, publisher_key, info);
  if (result != ledger::Result::LEDGER_OK) {
    GetServerInfoForEmptyResponse(publisher_key, info);
  }

  return ledger::Result::LEDGER_OK;
}

void GetPublisher::Request(
    const std::string& publisher_key,
    const std::string& hash_prefix,
    GetPublisherCallback callback) {
  auto url_callback = std::bind(&GetPublisher::OnRequest,
      this,
      _1,
      publisher_key,
      callback);
  ledger_->LoadURL(
      GetUrl(hash_prefix),
      {},
      "",
      "",
      ledger::UrlMethod::GET,
      url_callback);
}

void GetPublisher::OnRequest(
    const ledger::UrlResponse& response,
    const std::string& publisher_key,
    GetPublisherCallback callback) {
  ledger::LogUrlResponse(__func__, response);
  auto result = CheckStatusCode(response.status_code);

  auto info = ledger::ServerPublisherInfo::New();
  if (result == ledger::Result::NOT_FOUND) {
    GetServerInfoForEmptyResponse(publisher_key, info.get());
    callback(ledger::Result::LEDGER_OK, std::move(info));
    return;
  }

  if (result != ledger::Result::LEDGER_OK) {
    callback(ledger::Result::LEDGER_ERROR, nullptr);
    return;
  }

  result = ParseBody(response.body, publisher_key, info.get());

  if (result != ledger::Result::LEDGER_OK) {
    callback(result, nullptr);
    return;
  }

  callback(ledger::Result::LEDGER_OK, std::move(info));
}

}  // namespace private_cdn
}  // namespace endpoint
}  // namespace ledger
