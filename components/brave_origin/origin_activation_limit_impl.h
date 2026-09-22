/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ORIGIN_ORIGIN_ACTIVATION_LIMIT_IMPL_H_
#define BRAVE_COMPONENTS_BRAVE_ORIGIN_ORIGIN_ACTIVATION_LIMIT_IMPL_H_

#include <string>

#include "base/memory/scoped_refptr.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/api_request_helper/api_request_helper.h"
#include "brave/components/brave_origin/mojom/brave_origin_settings.mojom.h"

namespace network {
class SharedURLLoaderFactory;
}  // namespace network

namespace brave_origin {

// Talks to the payment service about an order's device activation budget.
//
// A subscription can only be activated on a limited number of devices. Once
// that budget is spent, fetching order credentials fails even though the
// subscription is still paid for, locking the subscriber out of what they
// bought. `CanExtend` asks whether that limit is the reason a fetch failed,
// and `Extend` raises the budget once the subscriber asks for it.
//
// Deliberately not part of the SKU SDK: the SDK is shared with desktop, while
// these two endpoints are mobile-only.
class OriginActivationLimitImpl : public mojom::OriginActivationLimit {
 public:
  explicit OriginActivationLimitImpl(
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory);
  ~OriginActivationLimitImpl() override;

  OriginActivationLimitImpl(const OriginActivationLimitImpl&) = delete;
  OriginActivationLimitImpl& operator=(const OriginActivationLimitImpl&) =
      delete;

  // mojom::OriginActivationLimit:
  // `receipt_payload` is the whole request body, already serialized:
  // {type, raw_receipt, package, subscription_id}.
  void CanExtend(const std::string& order_id,
                 const std::string& receipt_payload,
                 CanExtendCallback callback) override;
  void Extend(const std::string& order_id,
              const std::string& receipt_payload,
              ExtendCallback callback) override;

 private:
  void OnCanExtend(CanExtendCallback callback,
                   api_request_helper::APIRequestResult result);
  void OnExtend(ExtendCallback callback,
                api_request_helper::APIRequestResult result);

  api_request_helper::APIRequestHelper api_request_helper_;
  base::WeakPtrFactory<OriginActivationLimitImpl> weak_factory_{this};
};

}  // namespace brave_origin

#endif  // BRAVE_COMPONENTS_BRAVE_ORIGIN_ORIGIN_ACTIVATION_LIMIT_IMPL_H_
