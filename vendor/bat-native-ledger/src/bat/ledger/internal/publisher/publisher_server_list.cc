/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <algorithm>
#include <utility>

#include "base/json/json_reader.h"
#include "bat/ledger/internal/common/time_util.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/internal/publisher/publisher_server_list.h"
#include "bat/ledger/internal/state_keys.h"
#include "bat/ledger/internal/request/request_util.h"
#include "bat/ledger/internal/static_values.h"
#include "bat/ledger/option_keys.h"
#include "brave_base/random.h"
#include "net/http/http_status_code.h"

#if defined(OS_IOS)
#include <dispatch/dispatch.h>
#endif

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
#if defined(OS_IOS)
    std::string data = response;  // Make sure the data is copied into block
    dispatch_queue_global_t global_queue =
      dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0);
    dispatch_async(global_queue, ^{
      this->ParsePublisherList(data, parse_callback);
    });
#else
    ParsePublisherList(response, parse_callback);
#endif
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
    new_time = braveledger_time_util::GetCurrentTimeStamp();
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

  uint64_t now_seconds = braveledger_time_util::GetCurrentTimeStamp();

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
  base::Optional<base::Value> value = base::JSONReader::Read(data);
  if (!value || !value->is_list()) {
    BLOG(ledger_, ledger::LogLevel::LOG_ERROR) << "Data is not correct";
    callback(ledger::Result::LEDGER_ERROR);
    return;
  }

  for (auto& item : value->GetList()) {
    if (!item.is_list()) {
      continue;
    }

    const auto& list = item.GetList();

    if (list.size() != 5) {
      continue;
    }

    if (!list[0].is_string() || list[0].GetString().empty()  // Publisher key
        || !list[1].is_string()                              // Status
        || !list[2].is_bool()                                // Excluded
        || !list[3].is_string()) {                           // Address
      continue;
    }

    // Banner
    ledger::PublisherBannerPtr banner = nullptr;
    if (list[4].is_dict() && !list[4].DictEmpty()) {
      banner = ParsePublisherBanner(&list[4], list[0].GetString());
    }

    list_.push_back(ledger::ServerPublisherInfo::New(
        list[0].GetString(),
        ParsePublisherStatus(list[1].GetString()),
        list[2].GetBool(),
        list[3].GetString(),
        std::move(banner)));
  }

  if (list_.empty()) {
    callback(ledger::Result::LEDGER_ERROR);
    return;
  }

  auto clear_callback = std::bind(&PublisherServerList::SaveParsedData,
      this,
      _1,
      callback);

  ledger_->ClearServerPublisherList(clear_callback);
}

ledger::PublisherBannerPtr PublisherServerList::ParsePublisherBanner(
    base::Value* dictionary,
    const std::string& publisher_key) {
  DCHECK(dictionary);
  if (!dictionary->is_dict()) {
    return nullptr;
  }

  auto banner = ledger::PublisherBanner::New();
  const auto* title = dictionary->FindStringKey("title");
  if (title) {
    banner->title = *title;
  }

  const auto* description = dictionary->FindStringKey("description");
  if (description) {
    banner->description = *description;
  }

  const auto* background = dictionary->FindStringKey("backgroundUrl");
  if (background && !background->empty()) {
    banner->background = "chrome://rewards-image/" + *background;
  }

  const auto* logo = dictionary->FindStringKey("logoUrl");
  if (logo && !logo->empty()) {
    banner->logo = "chrome://rewards-image/" + *logo;
  }

  const auto* amounts = dictionary->FindListKey("donationAmounts");
  if (amounts) {
    for (const auto& it : amounts->GetList()) {
      if (it.is_int()) {
        banner->amounts.push_back(it.GetInt());
      }
    }
  }

  const auto* links = dictionary->FindDictKey("socialLinks");
  if (links) {
    for (const auto& it : links->DictItems()) {
      if (it.second.is_string()) {
        banner->links.insert(std::make_pair(it.first, it.second.GetString()));
      }
    }
  }

  banner->publisher_key = publisher_key;

  return banner;
}

void PublisherServerList::SaveParsedData(
    const ledger::Result result,
    ParsePublisherListCallback callback) {
  if (result != ledger::Result::LEDGER_OK) {
    callback(result);
    return;
  }

  if (!list_.empty()) {
    SavePublishers(callback);
    return;
  }

  callback(ledger::Result::LEDGER_OK);
}

void PublisherServerList::SavePublishers(
    ParsePublisherListCallback callback) {
  if (list_.empty()) {
    callback(ledger::Result::LEDGER_OK);
    return;
  }

  const int max_insert_records_ = 70000;

  int32_t interval = max_insert_records_;
  const auto list_size = list_.size();
  if (list_size < max_insert_records_) {
    interval = list_size;
  }

  ledger::ServerPublisherInfoList save_list;
  ledger::ServerPublisherInfoList new_list;
  int i = 0;
  for (auto& item : list_) {
    if (i <= interval) {
      save_list.push_back(std::move(item));
    } else {
      new_list.push_back(std::move(item));
    }
  }

  list_ = std::move(new_list);

  auto save_callback = std::bind(&PublisherServerList::SaveParsedData,
      this,
      _1,
      callback);

  ledger_->InsertServerPublisherList(std::move(save_list), save_callback);
}

void PublisherServerList::ClearTimer() {
  server_list_timer_id_ = 0;
}

}  // namespace braveledger_publisher
