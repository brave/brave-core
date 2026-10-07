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

// Result of a successful upload to the sharing server.
struct ConversationShareResult {
  // The viewer URL, without the decryption key fragment.
  GURL viewer_url;
  // Identifies the share within the viewer URL.
  std::string share_id;
  // Capability token required to delete the share. Empty if the server didn't
  // provide one, in which case the share cannot be deleted by this client.
  std::string deletion_id;
};

// Uploads client-encrypted conversation contents to the Brave sharing server
// and builds the shareable viewer URL from the returned share id. The key used
// to encrypt the contents never reaches this class or the server; the UI keeps
// it and appends it to the returned URL as a fragment.
class ConversationShareManager {
 public:
  // std::nullopt indicates the share failed (network error, unexpected
  // response, or an invalid resulting URL).
  using ShareConversationCallback =
      base::OnceCallback<void(const std::optional<ConversationShareResult>&)>;
  // Whether the server accepted the deletion.
  using DeleteShareCallback = base::OnceCallback<void(bool)>;

  explicit ConversationShareManager(
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory);
  ConversationShareManager(const ConversationShareManager&) = delete;
  ConversationShareManager& operator=(const ConversationShareManager&) = delete;
  virtual ~ConversationShareManager();

  // |encrypted_contents| is the base64-encoded (IV + AES-GCM ciphertext) blob
  // produced by the UI.
  virtual void ShareConversation(const std::string& encrypted_contents,
                                 ShareConversationCallback callback);

  // Asks the server to delete a previously uploaded share. |deletion_id| is the
  // capability token the server returned when the share was created - the share
  // id alone does not authorize deletion.
  virtual void DeleteShare(const std::string& deletion_id,
                           DeleteShareCallback callback);

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
  void OnDeleteShareCompleted(DeleteShareCallback callback,
                              api_request_helper::APIRequestResult result);

  std::unique_ptr<api_request_helper::APIRequestHelper> api_request_helper_;

  base::WeakPtrFactory<ConversationShareManager> weak_ptr_factory_{this};
};

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_CONVERSATION_SHARE_MANAGER_H_
