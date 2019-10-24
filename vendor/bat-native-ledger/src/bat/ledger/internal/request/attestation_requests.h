/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_COMMON_ATTESTATION_REQUESTS_H_
#define BRAVELEDGER_COMMON_ATTESTATION_REQUESTS_H_

#include <string>

namespace braveledger_request_util {

std::string GetStartAttestationDesktopUrl();

std::string GetStartAttestationAndroidUrl();

std::string GetCaptchaUrl(const std::string captcha_id);

std::string GetClaimAttestationDesktopUrl(const std::string captcha_id);

}  // namespace braveledger_request_util

#endif  // BRAVELEDGER_COMMON_ATTESTATION_REQUESTS_H_
