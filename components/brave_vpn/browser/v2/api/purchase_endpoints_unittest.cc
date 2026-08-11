/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/browser/v2/api/purchase_endpoints.h"

#include "base/values.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_vpn::v2::endpoints {
namespace {
constexpr char kTestProductType[] = "test-product-type";
constexpr char kTestProductId[] = "test-product-id";
constexpr char kTestValidationMethod[] = "test-validation-method";
constexpr char kTestPurchaseToken[] = "test-purchase-token";
constexpr char kTestBundleId[] = "test-bundle-id";
constexpr char kTestSkusCredential[] = "test-skus-credential";
}  // namespace

TEST(PurchaseEndpointsTest, GetSubscriberCredentialRequestBodyToValue) {
  const GetSubscriberCredentialRequestBody body{
      .product_type = kTestProductType,
      .product_id = kTestProductId,
      .validation_method = kTestValidationMethod,
      .purchase_token = kTestPurchaseToken,
      .bundle_id = kTestBundleId};
  EXPECT_EQ(body.ToValue(),
            base::DictValue()
                .Set(kProductTypeKey, kTestProductType)
                .Set(kProductIdKey, kTestProductId)
                .Set(kValidationMethodKey, kTestValidationMethod)
                .Set(kPurchaseTokenKey, kTestPurchaseToken)
                .Set(kBundleIdKey, kTestBundleId));
}

TEST(PurchaseEndpointsTest, GetSubscriberCredentialV12RequestBodyToValue) {
  const GetSubscriberCredentialV12RequestBody body{.skus_credential =
                                                       kTestSkusCredential};
  EXPECT_EQ(body.ToValue(),
            base::DictValue()
                .Set(kValidationMethodKey, kValidationMethodDefaultValue)
                .Set(kSkusCredentialKey, kTestSkusCredential));
}

TEST(PurchaseEndpointsTest, VerifyPurchaseTokenRequestBodyToValue) {
  const VerifyPurchaseTokenRequestBody body{
      .purchase_token = kTestPurchaseToken,
      .product_id = kTestProductId,
      .product_type = kTestProductType,
      .bundle_id = kTestBundleId};
  EXPECT_EQ(body.ToValue(), base::DictValue()
                                .Set(kPurchaseTokenKey, kTestPurchaseToken)
                                .Set(kProductIdKey, kTestProductId)
                                .Set(kProductTypeKey, kTestProductType)
                                .Set(kBundleIdKey, kTestBundleId));
}

}  // namespace brave_vpn::v2::endpoints
