/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ACCOUNT_FLOWS_MAKE_REQUEST_H_
#define BRAVE_COMPONENTS_BRAVE_ACCOUNT_FLOWS_MAKE_REQUEST_H_

#include "net/traffic_annotation/network_traffic_annotation.h"

namespace brave_account::internal {

inline constexpr net::NetworkTrafficAnnotationTag kTrafficAnnotation =
    net::DefineNetworkTrafficAnnotation("brave_account_endpoints",
                                        R"(
  semantics {
    sender: "Brave Account client"
    description:
      "Implements the creation or sign-in process for a Brave Account."
    trigger:
      "User attempts to create or sign in to a Brave Account from settings."
    user_data: {
      type: EMAIL
    }
    data:
      "Blinded cryptographic message for secure password setup "
      "and account email address."
      "Verification token for account activation and "
      "serialized cryptographic record for account finalization."
    destination: OTHER
    destination_other: "Brave Account service"
  }
  policy {
    cookies_allowed: NO
    policy_exception_justification:
      "These requests are essential for Brave Account creation and sign-in "
      "and cannot be disabled by policy."
  }
)");

template <typename Request>
auto MakeRequest() {
  Request request;
  request.network_traffic_annotation_tag =
      net::MutableNetworkTrafficAnnotationTag(kTrafficAnnotation);
  return request;
}

}  // namespace brave_account::internal

#endif  // BRAVE_COMPONENTS_BRAVE_ACCOUNT_FLOWS_MAKE_REQUEST_H_
