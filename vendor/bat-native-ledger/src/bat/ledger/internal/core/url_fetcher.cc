/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/core/url_fetcher.h"

#include <array>
#include <memory>
#include <string>
#include <utility>

#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/strings/string_piece.h"
#include "base/strings/string_util.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/ledger_client.h"

namespace ledger {

namespace {

constexpr std::array<const char*, 4> kRequestHeadersForLogging = {
    "digest", "signature", "accept", "content-type"};

constexpr size_t kMaxResponseBodySizeForLogging = 1024;

base::StringPiece GetResponseBodyForLogging(const std::string& body) {
  base::StringPiece result(body);
  return result.substr(0, kMaxResponseBodySizeForLogging);
}

bool CanLogRequestHeader(const std::string& header) {
  for (const char* name : kRequestHeadersForLogging) {
    if (base::StartsWith(header, name, base::CompareCase::INSENSITIVE_ASCII)) {
      return true;
    }
  }
  return false;
}

void LogURLRequest(BATLedgerContext* context,
                   const mojom::UrlRequest& request) {
  auto stream = context->LogVerbose(FROM_HERE);

  stream << "\n[ REQUEST ]";
  stream << "\n> URL: " << request.url;
  stream << "\n> Method: " << request.method;

  if (!request.content.empty()) {
    stream << "\n> Content: " << request.content;
  }

  if (!request.content_type.empty()) {
    stream << "\n> Content-Type: " << request.content_type;
  }

  for (const std::string& header : request.headers) {
    if (CanLogRequestHeader(header)) {
      stream << "\n> Header " << header;
    }
  }
}

void LogURLResponse(BATLedgerContext* context,
                    const mojom::UrlResponse& response) {
  std::string result;
  if (!response.error.empty()) {
    result = "Error (" + response.error + ")";
  } else if (response.status_code >= 200 && response.status_code < 300) {
    result = "Success";
  } else {
    result = "Failure";
  }

  auto stream = context->LogVerbose(FROM_HERE);

  stream << "\n[ RESPONSE ]";
  stream << "\n> URL: " << response.url;
  stream << "\n> Result: " << result;
  stream << "\n> HTTP Status: " << response.status_code;

  if (!response.body.empty()) {
    stream << "\n> Body:\n" << GetResponseBodyForLogging(response.body);
  }
}

}  // namespace

void URLRequest::SetBody(const std::string& content,
                         const std::string& content_type) {
  req_.content = content;
  req_.content_type = content_type;
}

void URLRequest::SetBody(const base::Value& value) {
  std::string json;
  bool ok = base::JSONWriter::Write(value, &json);
  DCHECK(ok);

  req_.content = json;
  req_.content_type = "application/json; charset=utf-8";
}

void URLRequest::AddHeader(const std::string& name, const std::string& value) {
  req_.headers.push_back(name + "=" + value);
}

URLRequest::URLRequest(mojom::UrlMethod method, const std::string& url) {
  req_.url = url;
  req_.method = method;
}

URLResponse::URLResponse(mojom::UrlResponsePtr response)
    : resp_(std::move(response)) {
  DCHECK(resp_);
}

URLResponse::~URLResponse() = default;

URLResponse::URLResponse(URLResponse&& other) = default;

URLResponse& URLResponse::operator=(URLResponse&& other) = default;

bool URLResponse::Succeeded() const {
  return resp_->status_code >= 200 && resp_->status_code < 300;
}

base::Value URLResponse::ReadBodyAsJSON() const {
  auto value = base::JSONReader::Read(resp_->body);
  if (!value) {
    value = base::Value(base::Value::Type::DICTIONARY);
  }
  return std::move(*value);
}

std::string URLResponse::ReadBodyAsText() const {
  return resp_->body;
}

Future<URLResponse> URLFetcher::Fetch(const URLRequest& request) {
  return FetchImpl(request);
}

Future<URLResponse> URLFetcher::FetchImpl(const URLRequest& request) {
  LogURLRequest(&context(), request.req());

  auto promise = std::make_shared<Promise<URLResponse>>();
  auto future = promise->GetFuture();
  auto weak_context = context().GetWeakPtr();

  context().GetLedgerClient()->LoadURL(
      request.req().Clone(),
      [weak_context, promise](const mojom::UrlResponse& response) {
        if (weak_context) {
          LogURLResponse(weak_context.get(), response);
        }
        promise->SetValue(URLResponse(response.Clone()));
      });

  return future;
}

}  // namespace ledger
