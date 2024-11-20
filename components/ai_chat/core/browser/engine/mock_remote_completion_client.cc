/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ai_chat/core/browser/engine/mock_remote_completion_client.h"

#include <string_view>

#include "base/memory/scoped_refptr.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"

namespace ai_chat {

MockRemoteCompletionClient::MockRemoteCompletionClient(
    const std::string& model_name)
    : RemoteCompletionClient(model_name, {}, nullptr, nullptr) {}
MockRemoteCompletionClient::~MockRemoteCompletionClient() = default;

}  // namespace ai_chat
