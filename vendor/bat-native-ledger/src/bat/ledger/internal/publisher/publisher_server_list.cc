/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <algorithm>
#include <utility>
#include <vector>

#include "base/json/json_reader.h"
#include "base/time/time.h"
#include "bat/ledger/internal/bat_helper.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/internal/publisher/publisher_server_list.h"
#include "bat/ledger/internal/state_keys.h"
#include "bat/ledger/internal/request/request_util.h"
#include "bat/ledger/option_keys.h"
#include "brave_base/random.h"
#include "net/http/http_status_code.h"

using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;

namespace braveledger_publisher {

PublisherServerList::PublisherServerList(bat_ledger::LedgerImpl* ledger) :
    ledger_(ledger),
    server_list_timer_id_(0ull) {
}

PublisherServerList::~PublisherServerList() {
}

void PublisherServerList::OnTimer(uint32_t timer_id) {
  if (timer_id == server_list_timer_id_) {
    server_list_timer_id_ = 0;
    Download([](const ledger::Result _){});
  }
}

void PublisherServerList::Download(
    DownloadServerPublisherListCallback callback) {
  std::vector<std::string> headers;
  headers.push_back("Accept-Encoding: gzip");

  const std::string url = braveledger_request_util::BuildUrl(
      GET_PUBLISHERS_LIST,
      "",
      braveledger_request_util::ServerTypes::PUBLISHER_DISTRO);

  const ledger::LoadURLCallback download_callback = std::bind(
      &PublisherServerList::OnDownload,
      this,
      _1,
      _2,
      _3,
      callback);

  ledger_->LoadURL(
      url,
      headers,
      "",
      "",
      ledger::UrlMethod::GET,
      download_callback);
}

void PublisherServerList::OnDownload(
    int response_status_code,
    const std::string& response,
    const std::map<std::string, std::string>& headers,
    DownloadServerPublisherListCallback callback) {
  ledger_->LogResponse(
      __func__,
      response_status_code,
      "Publisher list",
      headers);

  if (response_status_code == net::HTTP_OK && !response.empty()) {
    const auto parse_callback =
      std::bind(&PublisherServerList::OnParsePublisherList, this, _1, callback);
    ParsePublisherList(response, parse_callback);
    return;
  }

  BLOG(ledger_, ledger::LogLevel::LOG_ERROR) << "Can't fetch publisher list";
  SetTimer(true);
  callback(ledger::Result::LEDGER_ERROR);
}

void PublisherServerList::OnParsePublisherList(
    const ledger::Result result,
    DownloadServerPublisherListCallback callback) {
  uint64_t new_time = 0ull;
  if (result == ledger::Result::LEDGER_OK) {
    ledger_->ContributeUnverifiedPublishers();

    base::Time now = base::Time::Now();
    new_time = static_cast<uint64_t>(now.ToDoubleT());
  }

  ledger_->SetUint64State(ledger::kStateServerPublisherListStamp, new_time);

  bool retry_after_error = result != ledger::Result::LEDGER_OK;
  SetTimer(retry_after_error);

  callback(result);
}

void PublisherServerList::SetTimer(bool retry_after_error) {
  auto start_timer_in = 0ull;

  if (server_list_timer_id_ != 0) {
    // timer in progress
    return;
  }

  uint64_t last_download =
      ledger_->GetUint64State(ledger::kStateServerPublisherListStamp);
  start_timer_in = GetTimerTime(retry_after_error, last_download);

  // Start downloading right away
  if (start_timer_in == 0ull) {
    OnTimer(server_list_timer_id_);
    return;
  }

  // start timer
  ledger_->SetTimer(start_timer_in, &server_list_timer_id_);
}

uint64_t PublisherServerList::GetTimerTime(
    bool retry_after_error,
    const uint64_t last_download) {
  auto start_timer_in = 0ull;
  if (retry_after_error) {
    start_timer_in = brave_base::random::Geometric(150);

    BLOG(ledger_, ledger::LogLevel::LOG_WARNING) <<
      "Failed to refresh server list, will try again in " <<
      start_timer_in <<
      " seconds.";

    return start_timer_in;
  }

  base::Time now = base::Time::Now();
  uint64_t now_seconds = static_cast<uint64_t>(now.ToDoubleT());

  // check if last_download doesn't exist or have erroneous value.
  // (start_timer_in == 0) is expected to call callback function immediately.

  // time since last successful download
  uint64_t  time_since_last_download =
      (last_download == 0ull || last_download > now_seconds)
      ? 0ull
      : now_seconds - last_download;

  uint64_t interval =
      ledger_->GetUint64Option(ledger::kOptionPublisherListRefreshInterval);

  if (now_seconds == last_download) {
    start_timer_in = interval;
  } else if (time_since_last_download > 0 &&
             time_since_last_download < interval) {
    start_timer_in = interval - time_since_last_download;
  } else {
    start_timer_in = 0ull;
  }

  return start_timer_in;
}

ledger::PublisherStatus PublisherServerList::ParsePublisherStatus(
    const std::string& status) {
  if (status == "publisher_verified") {
    return ledger::PublisherStatus::CONNECTED;
  }

  if (status == "wallet_connected") {
    return ledger::PublisherStatus::VERIFIED;
  }

  return ledger::PublisherStatus::NOT_VERIFIED;
}

void PublisherServerList::ParsePublisherList(
    const std::string& data,
    ParsePublisherListCallback callback) {
  ledger::ServerPublisherInfoList list;

  base::Optional<base::Value> value = base::JSONReader::Read(data);
  if (!value || !value->is_list()) {
    callback(ledger::Result::LEDGER_ERROR);
    return;
  }

  base::ListValue* publishers = nullptr;
  if (!value->GetAsList(&publishers)) {
    callback(ledger::Result::LEDGER_ERROR);
    return;
  }

  for (auto& item : *publishers) {
    base::ListValue* values = nullptr;
    if (!item.GetAsList(&values)) {
      continue;
    }

    auto publisher = ledger::ServerPublisherInfo::New();

    // Publisher key
    std::string publisher_key = "";
    if (!values->GetList()[0].is_string()) {
      continue;
    }
    publisher_key = values->GetList()[0].GetString();

    if (publisher_key.empty()) {
      continue;
    }

    publisher->publisher_key = publisher_key;

    // Status
    if (!values->GetList()[1].is_string()) {
      continue;
    }
    publisher->status = ParsePublisherStatus(values->GetList()[1].GetString());

    // Excluded
    if (!values->GetList()[2].is_bool()) {
      continue;
    }
    publisher->excluded = values->GetList()[2].GetBool();

    // Address
    if (!values->GetList()[3].is_string()) {
      continue;
    }
    publisher->address = values->GetList()[3].GetString();

    // Banner
    base::DictionaryValue* banner = nullptr;
    if (values->GetList()[4].GetAsDictionary(&banner)) {
      publisher->banner = ParsePublisherBanner(banner);
    }

    list.push_back(std::move(publisher));
  }

  if (list.size() == 0) {
    callback(ledger::Result::LEDGER_ERROR);
  }

  ledger_->ClearAndInsertServerPublisherList(std::move(list), callback);
}

ledger::PublisherBannerPtr PublisherServerList::ParsePublisherBanner(
    base::DictionaryValue* dictionary) {
  if (!dictionary->is_dict()) {
    return nullptr;
  }

  auto banner = ledger::PublisherBanner::New();
  bool empty = true;
  auto* title = dictionary->FindKey("title");
  if (title && title->is_string()) {
    banner->title = title->GetString();
    if (!banner->title.empty()) {
      empty = false;
    }
  }

  auto* description = dictionary->FindKey("description");
  if (description && description->is_string()) {
    banner->description = description->GetString();
    if (!banner->description.empty()) {
      empty = false;
    }
  }

  auto* background = dictionary->FindKey("backgroundUrl");
  if (background && background->is_string()) {
    banner->background = background->GetString();

    if (!banner->background.empty()) {
      banner->background = "chrome://rewards-image/" + banner->background;
      empty = false;
    }
  }

  auto* logo = dictionary->FindKey("logoUrl");
  if (logo && logo->is_string()) {
    banner->logo = logo->GetString();

    if (!banner->logo.empty()) {
      banner->logo = "chrome://rewards-image/" + banner->logo;
      empty = false;
    }
  }

  auto* amounts = dictionary->FindKey("donationAmounts");
  if (amounts && amounts->is_list()) {
    for (const auto& it : amounts->GetList()) {
      banner->amounts.push_back(it.GetInt());
    }

    if (banner->amounts.size() != 0) {
      empty = false;
    }
  }

  auto* links = dictionary->FindKey("socialLinks");
  if (links && links->is_dict()) {
    for (const auto& it : links->DictItems()) {
      banner->links.insert(std::make_pair(it.first, it.second.GetString()));
    }

    if (banner->links.size() != 0) {
      empty = false;
    }
  }

  if (empty) {
    return nullptr;
  }

  return banner;
}

}  // namespace braveledger_publisher
