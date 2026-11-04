/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ai_chat/core/browser/mcp/test/deferred_os_crypt_async.h"

#include <memory>
#include <utility>
#include <vector>

#include "components/os_crypt/async/common/encryptor.h"

namespace ai_chat {

DeferredOSCryptAsync::DeferredOSCryptAsync(
    scoped_refptr<os_crypt_async::Encryptor> encryptor)
    : OSCryptAsync(std::vector<
                   std::pair<Precedence,
                             std::unique_ptr<os_crypt_async::KeyProvider>>>()),
      encryptor_(std::move(encryptor)) {}

DeferredOSCryptAsync::~DeferredOSCryptAsync() = default;

void DeferredOSCryptAsync::Complete() {
  complete_ = true;
  for (InitCallback& callback : std::exchange(callbacks_, {})) {
    std::move(callback).Run(encryptor_);
  }
}

void DeferredOSCryptAsync::GetInstance(InitCallback callback) {
  if (complete_) {
    std::move(callback).Run(encryptor_);
    return;
  }
  callbacks_.push_back(std::move(callback));
}

}  // namespace ai_chat
