/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_origin/origin_activation_limit_impl.h"

#include <optional>
#include <utility>

#include "base/functional/bind.h"
#include "base/logging.h"
#include "base/strings/escape.h"
#include "base/strings/strcat.h"
#include "base/values.h"
#include "brave/brave_domains/service_domains.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "url/gurl.h"

namespace brave_origin {

namespace {

// Hostname part of the payment service. Matches the host the SKU SDK derives
// from its environment in components/skus/browser/rs/lib/src/sdk/mod.rs, so
// both talk to the same service.
constexpr char kPaymentServiceHostnamePart[] = "payment.rewards";

// Subpaths under an order's credential batches collection.
constexpr char kExtendWithReceiptPath[] = "extend-with-receipt";
constexpr char kExtendWithReceiptCheckPath[] = "extend-with-receipt/check";

// Response fields of the check endpoint.
constexpr char kAtLimitKey[] = "at_limit";
constexpr char kCanExtendKey[] = "can_extend";

// Application error code in a failed response, which shares the shape of every
// other payment service error (see APIError in
// components/skus/browser/rs/lib/src/models.rs).
constexpr char kErrorCodeKey[] = "errorCode";

constexpr char kContentType[] = "application/json";

const net::NetworkTrafficAnnotationTag& GetNetworkTrafficAnnotationTag() {
  static const net::NetworkTrafficAnnotationTag tag =
      net::DefineNetworkTrafficAnnotation("brave_origin_activation_limit", R"(
      semantics {
        sender: "Brave Origin"
        description:
          "Asks the Brave payment service whether a Brave Origin subscription "
          "has exhausted its device activation budget, and raises that budget "
          "at the subscriber's request. Without this a subscriber who changes "
          "or reinstalls on enough devices is locked out of a subscription "
          "they still pay for."
        trigger:
          "Fetching credentials for an Origin order failed, or the subscriber "
          "asked to raise the activation limit."
        data:
          "The order id and the store receipt for the subscription."
        destination: OTHER
        destination_other: "Brave payment service"
      }
      policy {
        cookies_allowed: NO
        setting:
          "Only sent for users with a Brave Origin subscription purchased "
          "through an app store."
      })");
  return tag;
}

// Builds a URL under the order's credential batches collection on the payment
// service, which is the same host the SKU SDK talks to.
GURL GetBatchesUrl(const std::string& order_id, const std::string& subpath) {
  // STAGING for unofficial builds; official builds always resolve to prod.
  // Matches BraveOriginService's own domain resolution.
  return GURL(base::StrCat({"https://",
                            brave_domains::GetServicesDomain(
                                kPaymentServiceHostnamePart,
                                brave_domains::ServicesEnvironment::STAGING),
                            "/v1/orders/", base::EscapePath(order_id),
                            "/credentials/batches/", subpath}));
}

}  // namespace

OriginActivationLimitImpl::OriginActivationLimitImpl(
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory)
    : api_request_helper_(GetNetworkTrafficAnnotationTag(),
                          std::move(url_loader_factory)) {}

OriginActivationLimitImpl::~OriginActivationLimitImpl() = default;

void OriginActivationLimitImpl::CanExtend(const std::string& order_id,
                                          const std::string& receipt_payload,
                                          CanExtendCallback callback) {
  GURL url = GetBatchesUrl(order_id, kExtendWithReceiptCheckPath);
  if (receipt_payload.empty() || !url.is_valid()) {
    VLOG(1) << "Origin activation limit: no receipt or invalid check URL";
    std::move(callback).Run(nullptr);
    return;
  }

  api_request_helper_.Request(
      "POST", url, receipt_payload, kContentType,
      base::BindOnce(&OriginActivationLimitImpl::OnCanExtend,
                     weak_factory_.GetWeakPtr(), std::move(callback)));
}

void OriginActivationLimitImpl::OnCanExtend(
    CanExtendCallback callback,
    api_request_helper::APIRequestResult result) {
  if (!result.Is2XXResponseCode()) {
    VLOG(1) << "Origin activation limit: check failed, response code "
            << result.response_code();
    std::move(callback).Run(nullptr);
    return;
  }

  const base::DictValue* response = result.value_body().GetIfDict();
  if (!response) {
    VLOG(1) << "Origin activation limit: check returned a non-dictionary";
    std::move(callback).Run(nullptr);
    return;
  }

  // Both fields are required. Defaulting a missing one would either hide a
  // real limit or offer an extension the service never promised.
  std::optional<bool> at_limit = response->FindBool(kAtLimitKey);
  std::optional<bool> can_extend = response->FindBool(kCanExtendKey);
  if (!at_limit.has_value() || !can_extend.has_value()) {
    VLOG(1) << "Origin activation limit: check response missing at_limit or "
               "can_extend";
    std::move(callback).Run(nullptr);
    return;
  }

  std::move(callback).Run(
      mojom::ExtendEligibility::New(*at_limit, *can_extend));
}

void OriginActivationLimitImpl::Extend(const std::string& order_id,
                                       const std::string& receipt_payload,
                                       ExtendCallback callback) {
  GURL url = GetBatchesUrl(order_id, kExtendWithReceiptPath);
  if (receipt_payload.empty() || !url.is_valid()) {
    VLOG(1) << "Origin activation limit: no receipt or invalid extend URL";
    std::move(callback).Run(/*succeeded=*/false, /*error_code=*/std::string());
    return;
  }

  api_request_helper_.Request(
      "POST", url, receipt_payload, kContentType,
      base::BindOnce(&OriginActivationLimitImpl::OnExtend,
                     weak_factory_.GetWeakPtr(), std::move(callback)));
}

void OriginActivationLimitImpl::OnExtend(
    ExtendCallback callback,
    api_request_helper::APIRequestResult result) {
  if (result.Is2XXResponseCode()) {
    std::move(callback).Run(/*succeeded=*/true, /*error_code=*/std::string());
    return;
  }

  // Surface the application error code so callers can tell a refusal worth
  // showing the subscriber from one that only belongs in the log. It is absent
  // when the request never reached the service.
  std::string error_code;
  if (const base::DictValue* response = result.value_body().GetIfDict()) {
    if (const std::string* value = response->FindString(kErrorCodeKey)) {
      error_code = *value;
    }
  }
  VLOG(1) << "Origin activation limit: extend failed, response code "
          << result.response_code() << " error code \"" << error_code << "\"";
  std::move(callback).Run(/*succeeded=*/false, error_code);
}

}  // namespace brave_origin
