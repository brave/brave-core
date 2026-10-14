// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_LOCAL_TTS_CLIENT_H_
#define BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_LOCAL_TTS_CLIENT_H_

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "base/functional/callback.h"
#include "base/memory/scoped_refptr.h"

namespace network {
class SharedURLLoaderFactory;
}
namespace api_request_helper {
class APIRequestHelper;
}

namespace ai_chat {

// Pocket TTS running on loopback; responses contain mono float32 PCM.
class LocalTTSClient {
 public:
  using SynthesizeCallback = base::OnceCallback<
      void(const std::vector<uint8_t>&, uint32_t, const std::string&)>;

  explicit LocalTTSClient(
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory);
  ~LocalTTSClient();

  void Synthesize(const std::string& text, SynthesizeCallback callback);
  void Cancel();

 private:
  std::unique_ptr<api_request_helper::APIRequestHelper> request_helper_;
};

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_LOCAL_TTS_CLIENT_H_
