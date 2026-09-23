/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/p3a/network_annotations.h"

#include <string_view>

#include "base/check_op.h"
#include "brave/components/p3a/uploader.h"

namespace p3a {

net::NetworkTrafficAnnotationTag GetRandomnessRequestAnnotation() {
  return net::DefineNetworkTrafficAnnotation("p3a_star_randomness", R"(
    semantics {
      sender: "Brave Privacy-Preserving Product Analytics STAR Randomness Request"
      description:
        "Requests randomness for a single analytics metric."
        "The randomness data is used to create a key for encrypting analytics data "
        "using the STAR protocol, to protect user anonymity."
        "See https://arxiv.org/abs/2109.10074 for more information."
      trigger:
        "Requests are automatically sent at intervals "
        "while Brave is running."
      data: "Anonymous usage data."
      destination: WEBSITE
    }
    policy {
      cookies_allowed: NO
      setting:
        "Users can enable or disable it in brave://settings/privacy"
       policy_exception_justification:
         "Not implemented."
    })");
}

net::NetworkTrafficAnnotationTag GetRandomnessServerInfoAnnotation() {
  return net::DefineNetworkTrafficAnnotation("p3a_star_server_info", R"(
    semantics {
      sender: "Brave Privacy-Preserving Product Analytics STAR Randomness Server Info"
      description:
        "Requests randomness server info which includes the current epoch, "
        "and time of the next epoch."
        "The randomness data is used to create a key for encrypting analytics data "
        "using the STAR protocol, to protect user anonymity."
        "See https://arxiv.org/abs/2109.10074 for more information."
      trigger:
        "Requests are automatically sent at intervals "
        "while Brave is running."
      data: "Request for randomness server info."
      destination: WEBSITE
    }
    policy {
      cookies_allowed: NO
      setting:
        "Users can enable or disable it in brave://settings/privacy"
       policy_exception_justification:
         "Not implemented."
    })");
}

net::NetworkTrafficAnnotationTag GetP3AUploadAnnotation() {
  return net::DefineNetworkTrafficAnnotation("p3a", R"(
      semantics {
        sender: "Brave Privacy-Preserving Product Analytics Uploader"
        description:
          "Report of anonymized usage statistics. For more info, see "
          "https://brave.com/P3A"
        trigger:
          "Reports are automatically generated on startup and at intervals "
          "while Brave is running."
        data:
          "A base64 encoded encrypted payload with anonymized usage data."
          "Encryption is performed using STAR to protect user anonymity."
          "See https://arxiv.org/abs/2109.10074 for more information."
        destination: WEBSITE
      }
      policy {
        cookies_allowed: NO
        setting:
          "Users can enable or disable it in brave://settings/privacy"
          policy_exception_justification:
            "Not implemented."
      })");
}

}  // namespace p3a
