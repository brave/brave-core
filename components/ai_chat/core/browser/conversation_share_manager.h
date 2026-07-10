// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_CONVERSATION_SHARE_MANAGER_H_
#define BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_CONVERSATION_SHARE_MANAGER_H_

#include <memory>
#include <optional>
#include <string>

#include "base/functional/callback.h"
#include "base/memory/scoped_refptr.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/api_request_helper/api_request_helper.h"
#include "url/gurl.h"

namespace network {
class SharedURLLoaderFactory;
}  // namespace network

namespace ai_chat {

// Uploads client-encrypted conversation contents to the Brave sharing server
// and builds the shareable viewer URL from the returned share id. The key used
// to encrypt the contents never reaches this class or the server; the UI keeps
// it and appends it to the returned URL as a fragment.
class ConversationShareManager {
 public:
  // std::nullopt indicates the share failed (network error, unexpected
  // response, or an invalid resulting URL).
  using ShareConversationCallback =
      base::OnceCallback<void(const std::optional<GURL>&)>;

  explicit ConversationShareManager(
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory);
  ConversationShareManager(const ConversationShareManager&) = delete;
  ConversationShareManager& operator=(const ConversationShareManager&) = delete;
  virtual ~ConversationShareManager();

  // |encrypted_contents| is the base64-encoded (IV + AES-GCM ciphertext) blob
  // produced by the UI.
  virtual void ShareConversation(const std::string& encrypted_contents,
                                 ShareConversationCallback callback);

 protected:
  void SetAPIRequestHelperForTesting(
      std::unique_ptr<api_request_helper::APIRequestHelper> api_helper) {
    api_request_helper_ = std::move(api_helper);
  }
  api_request_helper::APIRequestHelper* GetAPIRequestHelperForTesting() {
    return api_request_helper_.get();
  }

 private:
  void OnShareCompleted(ShareConversationCallback callback,
                        api_request_helper::APIRequestResult result);

  std::unique_ptr<api_request_helper::APIRequestHelper> api_request_helper_;

  base::WeakPtrFactory<ConversationShareManager> weak_ptr_factory_{this};
};

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_CONVERSATION_SHARE_MANAGER_H_
