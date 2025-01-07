/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/web_discovery/browser/background_credential_helper.h"

#include <utility>

#include "brave/components/web_discovery/browser/background_credential_helper_impl.h"

namespace web_discovery {

GenerateJoinRequestResult::GenerateJoinRequestResult(
    std::string join_request_b64,
    std::vector<uint8_t> join_gsk,
    std::string signature)
    : join_request_b64(std::move(join_request_b64)),
      join_gsk(std::move(join_gsk)),
      signature(std::move(signature)) {}

GenerateJoinRequestResult::~GenerateJoinRequestResult() = default;

GenerateJoinRequestResult::GenerateJoinRequestResult(
    const GenerateJoinRequestResult&) = default;
GenerateJoinRequestResult& GenerateJoinRequestResult::operator=(
    const GenerateJoinRequestResult&) = default;

std::unique_ptr<BackgroundCredentialHelper>
BackgroundCredentialHelper::Create() {
  return std::make_unique<BackgroundCredentialHelperImpl>();
}

}  // namespace web_discovery
