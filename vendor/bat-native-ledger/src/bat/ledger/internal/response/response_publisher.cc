/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/response/response_publisher.h"

#include <utility>

#include "base/json/json_reader.h"
#include "bat/ledger/internal/common/brotli_helpers.h"
#include "bat/ledger/internal/common/time_util.h"
#include "bat/ledger/internal/logging/logging.h"
#include "bat/ledger/internal/publisher/protos/channel_response.pb.h"
#include "brave/components/brave_private_cdn/private_cdn_helper.h"
#include "net/http/http_status_code.h"

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

  for (auto& amount : banner_details.donation_amounts()) {
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

ledger::ServerPublisherInfoPtr GetServerInfoForEmptyResponse(
    const std::string& publisher_key) {
  // The server has indicated that a publisher record does not exist
  // for this publisher key, perhaps as a result of a false positive
  // when searching the publisher prefix list. Create a "non-verified"
  // record that can be cached in the database so that we don't repeatedly
  // attempt to fetch from the server for this publisher.
  BLOG(1, "Server did not return an entry for publisher " << publisher_key);
  auto server_info = ledger::ServerPublisherInfo::New();
  server_info->publisher_key = publisher_key;
  server_info->status = ledger::PublisherStatus::NOT_VERIFIED;
  server_info->updated_at = braveledger_time_util::GetCurrentTimeStamp();
  return server_info;
}

ledger::ServerPublisherInfoPtr ServerPublisherInfoFromMessage(
    const publishers_pb::ChannelResponseList& message,
    const std::string& expected_key) {
  if (expected_key.empty()) {
    return nullptr;
  }

  for (const auto& entry : message.channel_responses()) {
    if (entry.channel_identifier() != expected_key) {
      continue;
    }

    auto server_info = ledger::ServerPublisherInfo::New();
    server_info->publisher_key = entry.channel_identifier();
    server_info->status = GetPublisherStatusFromMessage(entry);
    server_info->address = GetPublisherAddressFromMessage(entry);
    server_info->updated_at = braveledger_time_util::GetCurrentTimeStamp();

    if (entry.has_site_banner_details()) {
      server_info->banner =
          GetPublisherBannerFromMessage(entry.site_banner_details());
    }

    return server_info;
  }

  return nullptr;
}

bool DecompressMessage(base::StringPiece payload, std::string* output) {
  constexpr size_t buffer_size = 32 * 1024;
  return braveledger_helpers::DecodeBrotliStringWithBuffer(
      payload,
      buffer_size,
      output);
}

}  // namespace

namespace braveledger_response_util {

// Request Url:
// GET /publishers/prefixes/{prefix}
//
// Success:
// OK (200)
//
// Response Format:
// See https://github.com/brave/brave-core/blob/master/vendor/bat-native-ledger/src/bat/ledger/internal/publisher/protos/channel_response.proto

ledger::ServerPublisherInfoPtr ParsePublisherInfo(
    const std::string& publisher_key,
    int response_status_code,
    const std::string& response) {
  // Not Found (404)
  if (response_status_code == net::HTTP_NOT_FOUND) {
    return GetServerInfoForEmptyResponse(publisher_key);
  }

  if (response_status_code != net::HTTP_OK || response.empty()) {
    BLOG(0, "Server returned an invalid response from publisher data URL");
    return nullptr;
  }

  base::StringPiece response_payload(response.data(), response.size());
  if (!PrivateCdnHelper::GetInstance()->RemovePadding(&response_payload)) {
    BLOG(0, "Publisher data response has invalid padding");
    return nullptr;
  }

  std::string message_string;
  if (!DecompressMessage(response_payload, &message_string)) {
    BLOG(1,
         "Error decompressing publisher data response. "
         "Attempting to parse as uncompressed message.");
    message_string.assign(response_payload.data(), response_payload.size());
  }

  publishers_pb::ChannelResponseList message;
  if (!message.ParseFromString(message_string)) {
    BLOG(0, "Error parsing publisher data protobuf message");
    return nullptr;
  }

  auto server_info = ServerPublisherInfoFromMessage(message, publisher_key);
  if (!server_info) {
    return GetServerInfoForEmptyResponse(publisher_key);
  }

  return server_info;
}

}  // namespace braveledger_response_util
