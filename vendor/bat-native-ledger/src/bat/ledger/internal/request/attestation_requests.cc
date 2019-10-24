/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <string>

#include "base/strings/stringprintf.h"
#include "bat/ledger/internal/static_values.h"
#include "bat/ledger/internal/request/attestation_requests.h"
#include "bat/ledger/internal/request/request_util.h"

namespace braveledger_request_util {

std::string GetStartAttestationDesktopUrl() {
  return BuildUrl("/captchas", PREFIX_V1, ServerTypes::kPromotion);
}

std::string GetStartAttestationAndroidUrl() {
  return BuildUrl(
      "/attestations/safetynet",
      PREFIX_V2,
      ServerTypes::kPromotion);
}

std::string GetCaptchaUrl(const std::string captcha_id) {
  const std::string path = base::StringPrintf(
      "/captchas/%s.png",
      captcha_id.c_str());
  return BuildUrl(path, PREFIX_V1, ServerTypes::kPromotion);
}

std::string GetClaimAttestationDesktopUrl(const std::string captcha_id) {
  const std::string path = base::StringPrintf(
      "/captchas/%s",
      captcha_id.c_str());
  return BuildUrl(path, PREFIX_V1, ServerTypes::kPromotion);
}

}  // namespace braveledger_request_util
