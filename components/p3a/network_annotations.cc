/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/p3a/network_annotations.h"

#include <string_view>

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

net::NetworkTrafficAnnotationTag GetP3AUploadAnnotation(
    std::string_view upload_type,
    bool is_constellation) {
  if (is_constellation) {
    if (upload_type == kP3ACreativeUploadType ||
        upload_type == kP3AUploadType) {
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
    DCHECK_EQ(upload_type, kP2AUploadType);
    return net::DefineNetworkTrafficAnnotation("p2a", R"(
        semantics {
          sender: "Brave Privacy-Preserving Ad Analytics Uploader"
          description:
            "Report of anonymized usage statistics. For more info, see "
            "https://github.com/brave/brave-browser/wiki/"
            "Randomized-Response-for-Private-Advertising-Analytics"
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
            "Users can enable or disable it by enabling or disabling Brave "
            "rewards or ads in brave://rewards."
           policy_exception_justification:
             "Not implemented."
        })");
  } else {
    if (upload_type == kP3ACreativeUploadType ||
        upload_type == kP3AUploadType) {
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
              "A json document with anonymized usage data."
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
    DCHECK_EQ(upload_type, kP2AUploadType);
    return net::DefineNetworkTrafficAnnotation("p2a", R"(
        semantics {
          sender: "Brave Privacy-Preserving Ad Analytics Uploader"
          description:
            "Report of anonymized usage statistics. For more info, see "
            "https://github.com/brave/brave-browser/wiki/"
            "Randomized-Response-for-Private-Advertising-Analytics"
          trigger:
            "Reports are automatically generated on startup and at intervals "
            "while Brave is running."
          data:
            "A json document with anonymized usage data."
          destination: WEBSITE
        }
        policy {
          cookies_allowed: NO
          setting:
            "Users can enable or disable it by enabling or disabling Brave "
            "rewards or ads in brave://rewards."
           policy_exception_justification:
             "Not implemented."
        })");
  }
}

}  // namespace p3a
