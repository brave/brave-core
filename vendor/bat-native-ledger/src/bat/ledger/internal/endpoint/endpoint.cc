/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/endpoint/endpoint.h"

#include <tuple>
#include <utility>

#include "bat/ledger/internal/ledger_impl.h"
#include "net/http/http_status_code.h"

namespace ledger::endpoint {
namespace {

void Send(absl::optional<std::tuple<LedgerImpl*,
                                    type::UrlRequestPtr,
                                    client::LoadURLCallback,
                                    uint32_t>> params) {
  if (params) {
    auto [ledger, request, callback, retry_count_on_rate_limiting] =
        std::move(*params);

    auto request_clone = request->Clone();
    ledger->LoadURL(
        std::move(request),
        base::BindOnce(
            [](LedgerImpl* ledger, type::UrlRequestPtr request,
               client::LoadURLCallback callback,
               uint32_t retry_count_on_rate_limiting,
               const type::UrlResponse& response) -> decltype(params) {
              // TODO(sszaloki):
              // https://github.com/brave/brave-browser/issues/17442
              if (response.status_code == net::HTTP_TOO_MANY_REQUESTS &&
                  retry_count_on_rate_limiting) {
                ledger::LogUrlResponse(__func__, response);

                return std::tuple(ledger, std::move(request),
                                  std::move(callback),
                                  --retry_count_on_rate_limiting);
              } else {
                std::move(callback).Run(response);
                return absl::nullopt;
              }
            },
            ledger, std::move(request_clone), std::move(callback),
            retry_count_on_rate_limiting)
            .Then(base::BindOnce(&Send)));
  }
}

}  // namespace

Endpoint::~Endpoint() = default;

base::expected<type::UrlRequestPtr, std::string> Endpoint::Request() const {
  std::string errors;

  const auto url = Url();
  if (!url.has_value()) {
    errors += '\n' + url.error().ToString();
  }

  auto headers = Headers();
  if (!headers.has_value()) {
    errors += '\n' + headers.error().ToString();
  }

  const auto content = Content();
  if (!content.has_value()) {
    errors += '\n' + content.error().ToString();
  }

  if (!errors.empty()) {
    return base::unexpected((std::ostringstream{}
                             << "Endpoint: " << Method()
                             << (url.has_value() ? " " + *url : "")
                             << "\nErrors:" << errors)
                                .str());
  }

  return type::UrlRequest::New(*url, Method(), std::move(*headers), *content,
                               ContentType(), SkipLog(), LoadFlags());
}

void Endpoint::Send(type::UrlRequestPtr request,
                    client::LoadURLCallback callback) const {
  endpoint::Send(std::tuple(ledger_, std::move(request), std::move(callback),
                            RetryCountOnRateLimiting()));
}

Endpoint::Endpoint(LedgerImpl* ledger) : ledger_((DCHECK(ledger), ledger)) {}

type::UrlMethod Endpoint::Method() const {
  return type::UrlMethod::POST;
}

base::expected<std::vector<std::string>, base::Location> Endpoint::Headers()
    const {
  return std::vector<std::string>{};
}

base::expected<std::string, base::Location> Endpoint::Content() const {
  return "";
}

std::string Endpoint::ContentType() const {
  return "application/json; charset=utf-8";
}

bool Endpoint::SkipLog() const {
  return false;
}

uint32_t Endpoint::LoadFlags() const {
  return 0;
}

uint32_t Endpoint::RetryCountOnRateLimiting() const {
  return 0;
}

};  // namespace ledger::endpoint
