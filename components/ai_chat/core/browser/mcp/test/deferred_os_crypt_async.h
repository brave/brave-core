/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_MCP_TEST_DEFERRED_OS_CRYPT_ASYNC_H_
#define BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_MCP_TEST_DEFERRED_OS_CRYPT_ASYNC_H_

#include <vector>

#include "base/memory/scoped_refptr.h"
#include "components/os_crypt/async/browser/os_crypt_async.h"

namespace os_crypt_async {
class Encryptor;
}  // namespace os_crypt_async

namespace ai_chat {

// Hands out `encryptor` only when a test says so, so that tests can control
// when it becomes available, and share it between instances.
class DeferredOSCryptAsync : public os_crypt_async::OSCryptAsync {
 public:
  explicit DeferredOSCryptAsync(
      scoped_refptr<os_crypt_async::Encryptor> encryptor);
  ~DeferredOSCryptAsync() override;

  // Hands out the encryptor to everyone waiting, and from now on to anyone
  // who asks.
  void Complete();

  // os_crypt_async::OSCryptAsync:
  void GetInstance(InitCallback callback) override;

 private:
  scoped_refptr<os_crypt_async::Encryptor> encryptor_;
  bool complete_ = false;
  std::vector<InitCallback> callbacks_;
};

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_MCP_TEST_DEFERRED_OS_CRYPT_ASYNC_H_
