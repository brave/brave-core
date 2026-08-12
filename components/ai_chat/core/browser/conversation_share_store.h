// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_CONVERSATION_SHARE_STORE_H_
#define BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_CONVERSATION_SHARE_STORE_H_

#include <optional>
#include <string>
#include <vector>

#include "base/functional/callback.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/scoped_refptr.h"
#include "base/memory/weak_ptr.h"
#include "base/time/time.h"
#include "base/timer/timer.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom-forward.h"
#include "url/gurl.h"

class PrefService;

namespace base {
class Value;
}  // namespace base

namespace os_crypt_async {
class Encryptor;
class OSCryptAsync;
}  // namespace os_crypt_async

namespace ai_chat {

// Remembers which conversations the user has shared, so that the share
// management UI can list them, re-copy their links and delete them from the
// sharing server.
//
// Records hold the full shareable link, which contains the conversation's
// decryption key, so the list is encrypted with OSCrypt before it is written to
// prefs. If OSCrypt has no key available then nothing is recorded - the share
// itself still works, it just can't be managed later.
//
// The sharing server deletes a share once it is
// features::kAIChatConversationShareExpiryDays old, so records at least that
// old are dropped when this store is created, and each remaining record is
// dropped as it reaches that age. No timer is pending when there is nothing
// left to expire.
class ConversationShareStore {
 public:
  using GetSharesCallback =
      base::OnceCallback<void(std::vector<mojom::ConversationSharePtr>)>;

  ConversationShareStore(PrefService* prefs,
                         os_crypt_async::OSCryptAsync* os_crypt_async);
  ConversationShareStore(const ConversationShareStore&) = delete;
  ConversationShareStore& operator=(const ConversationShareStore&) = delete;
  virtual ~ConversationShareStore();

  // |url| is the full shareable link, including the decryption key fragment.
  virtual void AddShare(const std::string& share_id,
                        const std::string& deletion_id,
                        const std::string& conversation_uuid,
                        const std::string& conversation_title,
                        const GURL& url);

  // Unexpired shares, most recently shared first.
  virtual void GetShares(GetSharesCallback callback);

 private:
  struct ShareRecord {
    ShareRecord();
    ShareRecord(const ShareRecord&);
    ShareRecord& operator=(const ShareRecord&);
    ~ShareRecord();

    std::string share_id;
    std::string deletion_id;
    std::string conversation_uuid;
    std::string conversation_title;
    GURL url;
    base::Time created_time;
  };

  // std::nullopt if |value| isn't a record this version can use.
  static std::optional<ShareRecord> ShareRecordFromValue(
      const base::Value& value);

  void OnEncryptorReady(scoped_refptr<os_crypt_async::Encryptor> encryptor);

  // Runs |task| once the encryptor has arrived and |records_| has been read
  // from prefs, which may be immediately.
  void RunWhenLoaded(base::OnceClosure task);

  void AddShareInternal(ShareRecord record);
  void GetSharesInternal(GetSharesCallback callback);

  // Drops records the sharing server will have deleted by now. Returns whether
  // anything was dropped.
  bool DropExpiredRecords();
  void PurgeExpiredRecords();

  // Arms |expiry_timer_| for when the oldest remaining record expires, or
  // cancels it when no records remain. Must be called after any change to
  // |records_|.
  void ScheduleNextPurge();

  void WriteRecordsToPrefs();

  raw_ptr<PrefService> prefs_;

  scoped_refptr<os_crypt_async::Encryptor> encryptor_;

  // std::nullopt until the encryptor has arrived and prefs have been read.
  std::optional<std::vector<ShareRecord>> records_;

  // Operations which arrived before the encryptor did.
  std::vector<base::OnceClosure> pending_tasks_;

  base::OneShotTimer expiry_timer_;

  base::WeakPtrFactory<ConversationShareStore> weak_ptr_factory_{this};
};

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_CONVERSATION_SHARE_STORE_H_
