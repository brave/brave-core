/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include "bat/ledger/internal/endpoint/promotion/get_drain/get_drain.h"

#include <utility>

#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/strings/stringprintf.h"
#include "bat/ledger/internal/common/request_util.h"
#include "bat/ledger/internal/endpoint/promotion/promotions_util.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "net/http/http_status_code.h"

using std::placeholders::_1;

namespace ledger {
namespace endpoint {
namespace promotion {

GetDrain::GetDrain(LedgerImpl* ledger) : ledger_(ledger) {
  DCHECK(ledger_);
}

GetDrain::~GetDrain() = default;

std::string GetDrain::GetUrl(const std::string& drain_id) {
  return GetServerUrl("/v1/promotions/drain/" + drain_id);
}

type::Result GetDrain::CheckStatusCode(const int status_code) {
  if (status_code == net::HTTP_BAD_REQUEST) {
    BLOG(0, "Invalid request");
    return type::Result::LEDGER_ERROR;
  }

  if (status_code == net::HTTP_NOT_FOUND) {
    BLOG(0, "Drain ID URL param not found");
    return type::Result::LEDGER_ERROR;
  }

  if (status_code != net::HTTP_OK) {
    BLOG(0, "Unexpected HTTP status: " << status_code);
    return type::Result::LEDGER_ERROR;
  }

  return type::Result::LEDGER_OK;
}

void GetDrain::Request(const std::string& drain_id, GetDrainCallback callback) {
  auto url_callback = std::bind(&GetDrain::OnRequest, this, _1, callback);

  auto request = type::UrlRequest::New();
  request->url = GetUrl(drain_id);
  request->method = type::UrlMethod::GET;
  ledger_->LoadURL(std::move(request), url_callback);
}

void GetDrain::OnRequest(const type::UrlResponse& response,
                         GetDrainCallback callback) {
  ledger::LogUrlResponse(__func__, response);
  auto result = CheckStatusCode(response.status_code);
  if (result != type::Result::LEDGER_OK) {
    callback(type::Result::LEDGER_ERROR, type::DrainStatus::INVALID);
    return;
  }

  absl::optional<base::Value> value = base::JSONReader::Read(response.body);
  if (!value || !value->is_dict()) {
    BLOG(0, "Invalid JSON");
    callback(type::Result::LEDGER_ERROR, type::DrainStatus::INVALID);
    return;
  }

  base::DictionaryValue* dictionary = nullptr;
  if (!value->GetAsDictionary(&dictionary)) {
    BLOG(0, "Invalid JSON");
    callback(type::Result::LEDGER_ERROR, type::DrainStatus::INVALID);
    return;
  }

  auto* drain_id = dictionary->FindStringKey("drainId");
  if (!drain_id) {
    BLOG(0, "Missing key drain id");
    callback(type::Result::LEDGER_ERROR, type::DrainStatus::INVALID);
    return;
  }

  auto* status = dictionary->FindStringKey("status");
  if (!status) {
    BLOG(0, "Missing key status");
    callback(type::Result::LEDGER_ERROR, type::DrainStatus::INVALID);
    return;
  }

  auto drain_status = type::DrainStatus::INVALID;
  if (*status == "complete") {
    drain_status = type::DrainStatus::COMPLETE;
  } else if (*status == "pending") {
    drain_status = type::DrainStatus::PENDING;
  } else if (*status == "delayed") {
    drain_status = type::DrainStatus::DELAYED;
  } else if (*status == "in-progress") {
    drain_status = type::DrainStatus::IN_PROGRESS;
  } else {
    BLOG(0, "Invalid drain status.");
    callback(type::Result::LEDGER_ERROR, drain_status);
    return;
  }

  callback(result, drain_status);
}

}  // namespace promotion
}  // namespace endpoint
}  // namespace ledger
