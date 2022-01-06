/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/core/environment_config.h"

namespace ledger {

const char* EnvironmentConfig::auto_contribute_sku() const {
  switch (env()) {
    case Environment::kDevelopment:
      return "AgEJYnJhdmUuY29tAiNicmF2ZSB1c2VyLXdhbGxldC12b3RlIHNrdSB0b2tlbiB2M"
             "QACFHNrdT11c2VyLXdhbGxldC12b3RlAAIKcHJpY2U9MC4yNQACDGN1cnJlbmN5PU"
             "JBVAACDGRlc2NyaXB0aW9uPQACGmNyZWRlbnRpYWxfdHlwZT1zaW5nbGUtdXNlAAA"
             "GINiB9dUmpqLyeSEdZ23E4dPXwIBOUNJCFN9d5toIME2M";
    case Environment::kStaging:
      return "AgEJYnJhdmUuY29tAiNicmF2ZSB1c2VyLXdhbGxldC12b3RlIHNrdSB0b2tlbiB2M"
             "QACFHNrdT11c2VyLXdhbGxldC12b3RlAAIKcHJpY2U9MC4yNQACDGN1cnJlbmN5PU"
             "JBVAACDGRlc2NyaXB0aW9uPQACGmNyZWRlbnRpYWxfdHlwZT1zaW5nbGUtdXNlAAA"
             "GIOH4Li+rduCtFOfV8Lfa2o8h4SQjN5CuIwxmeQFjOk4W";
    case Environment::kProduction:
      return "AgEJYnJhdmUuY29tAiNicmF2ZSB1c2VyLXdhbGxldC12b3RlIHNrdSB0b2tlbiB2M"
             "QACFHNrdT11c2VyLXdhbGxldC12b3RlAAIKcHJpY2U9MC4yNQACDGN1cnJlbmN5PU"
             "JBVAACDGRlc2NyaXB0aW9uPQACGmNyZWRlbnRpYWxfdHlwZT1zaW5nbGUtdXNlAAA"
             "GIOaNAUCBMKm0IaLqxefhvxOtAKB0OfoiPn0NPVfI602J";
  }
}

const char* EnvironmentConfig::uphold_token_order_address() const {
  switch (env()) {
    case Environment::kDevelopment:
      return "9094c3f2-b3ae-438f-bd59-92aaad92de5c";
    case Environment::kStaging:
      return "6654ecb0-6079-4f6c-ba58-791cc890a561";
    case Environment::kProduction:
      return "5d4be2ad-1c65-4802-bea1-e0f3a3a487cb";
  }
}

const char* EnvironmentConfig::gemini_token_order_address() const {
  switch (env()) {
    case Environment::kDevelopment:
      return "60e5e863-8c3d-4341-8b54-23e2695a490c";
    case Environment::kStaging:
      return "622b9018-f26a-44bf-9a45-3bf3bf3c95e9";
    case Environment::kProduction:
      return "6116adaf-92e6-42fa-bee8-6f749b8eb44e";
  }
}

const char* EnvironmentConfig::payment_service_host() const {
  switch (env()) {
    case Environment::kDevelopment:
      return "payment.rewards.brave.software";
    case Environment::kStaging:
      return "payment.rewards.bravesoftware.com";
    case Environment::kProduction:
      return "payment.rewards.brave.com";
  }
}

}  // namespace ledger
