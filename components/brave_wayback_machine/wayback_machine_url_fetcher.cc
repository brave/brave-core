/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wayback_machine/wayback_machine_url_fetcher.h"

#include <algorithm>
#include <string>
#include <utility>

#include "base/functional/bind.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "brave/components/brave_wayback_machine/brave_wayback_machine_utils.h"
#include "brave/components/brave_wayback_machine/url_constants.h"
#include "net/base/load_flags.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/mojom/fetch_api.mojom-shared.h"
#include "url/gurl.h"
#include "url/url_constants.h"

namespace {

constexpr int kMaxBodySize = 1024 * 1024;
constexpr char kArchiveSnapshotsClosestKey[] = "archived_snapshots.closest";
constexpr char kUrlKey[] = "url";
constexpr char kTimestampKey[] = "timestamp";

const net::NetworkTrafficAnnotationTag& GetNetworkTrafficAnnotationTag() {
  static const net::NetworkTrafficAnnotationTag network_traffic_annotation_tag =
      net::DefineNetworkTrafficAnnotation("wayback_machine_url_fetcher", R"(
        semantics {
          sender:
            "Brave Wayback Machine"
          description:
            "Download wayback url"
          trigger:
            "When user gets 404 page"
          data: "current tab's url"
          destination: WEBSITE
        }
        policy {
          cookies_allowed: NO
          policy_exception_justification:
            "Not implemented."
        })");
  return network_traffic_annotation_tag;
}

}  // namespace

WaybackMachineURLFetcher::WaybackMachineURLFetcher(
    Client* client,
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory)
    : client_(client),
      api_request_helper_(new api_request_helper::APIRequestHelper(
          GetNetworkTrafficAnnotationTag(),
          url_loader_factory)) {}

WaybackMachineURLFetcher::~WaybackMachineURLFetcher() = default;

void WaybackMachineURLFetcher::Fetch(const GURL& url) {
  Cancel();
  const GURL wayback_fetch_url(std::string(kWaybackQueryURL) +
                               GetSanitizedInputURL(url).spec());
  api_request_helper_->Request(
      "GET", FixupWaybackQueryURL(wayback_fetch_url), std::string(),
      "application/json",
      base::BindOnce(&WaybackMachineURLFetcher::OnWaybackURLFetched,
                     base::Unretained(this)),
      {},
      {.auto_retry_on_network_change = true, .max_body_size = kMaxBodySize});
}

void WaybackMachineURLFetcher::Cancel() {
  api_request_helper_->CancelAll();
}

void WaybackMachineURLFetcher::OnWaybackURLFetched(
    api_request_helper::APIRequestResult api_request_result) {
  auto notify_not_found = [&]() {
    client_->OnWaybackURLFetched(GURL::EmptyGURL(), base::Time());
  };

  auto& value_body = api_request_result.value_body();
  if (!value_body.is_dict()) {
    notify_not_found();
    return;
  }

  const base::DictValue* closest =
      value_body.GetDict().FindDictByDottedPath(kArchiveSnapshotsClosestKey);
  const std::string* url_string =
      closest ? closest->FindString(kUrlKey) : nullptr;

  // Response doesn't have wayback url.
  if (!url_string) {
    notify_not_found();
    return;
  }

  GURL wayback_url = GetSanitizedWaybackURL(GURL(*url_string));
  if (wayback_url.is_empty()) {
    notify_not_found();
    return;
  }

  const std::string* timestamp = closest->FindString(kTimestampKey);
  client_->OnWaybackURLFetched(
      wayback_url,
      timestamp ? ParseSnapshotTimestamp(*timestamp) : base::Time());
}

// static
base::Time WaybackMachineURLFetcher::ParseSnapshotTimestamp(
    std::string_view timestamp) {
  if (timestamp.size() != 14 ||
      !std::ranges::all_of(timestamp, base::IsAsciiDigit<char>)) {
    return base::Time();
  }

  auto field = [&](size_t pos, size_t len) {
    int value = 0;
    base::StringToInt(timestamp.substr(pos, len), &value);
    return value;
  };

  base::Time::Exploded exploded = {
      .year = field(0, 4),
      .month = field(4, 2),
      .day_of_month = field(6, 2),
      .hour = field(8, 2),
      .minute = field(10, 2),
      .second = field(12, 2),
  };

  base::Time time;
  if (!base::Time::FromUTCExploded(exploded, &time)) {
    return base::Time();
  }
  return time;
}

GURL WaybackMachineURLFetcher::GetSanitizedWaybackURL(const GURL& url) const {
  if (!url.is_valid()) {
    return GURL::EmptyGURL();
  }

  if (!url.SchemeIsHTTPOrHTTPS()) {
    return GURL::EmptyGURL();
  }

  if (url.host() != kWaybackHost) {
    return GURL::EmptyGURL();
  }

  // Upgrade to https.
  if (url.SchemeIs(url::kHttpScheme)) {
    GURL::Replacements replacements;
    replacements.SetSchemeStr(url::kHttpsScheme);
    return url.ReplaceComponents(replacements);
  }

  return url;
}

GURL WaybackMachineURLFetcher::GetSanitizedInputURL(const GURL& url) const {
  GURL::Replacements replacements;
  replacements.ClearRef();
  replacements.ClearUsername();
  replacements.ClearPassword();
  return url.ReplaceComponents(replacements);
}
