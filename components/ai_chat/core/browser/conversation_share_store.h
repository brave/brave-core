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
// Prefs are the only copy: the list is read back for each operation rather than
// cached, so the decryption keys it holds are only in memory for as long as an
// operation takes. The list is small (the server expires shares, see below) and
// every write needs the whole list anyway, since it is stored as one blob.
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
    ShareRecord(ShareRecord&&) noexcept;
    ShareRecord& operator=(ShareRecord&&) noexcept;
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

  // A deferred store operation. It is handed the store when it runs, so that a
  // queued task never holds a pointer to one; see RunWhenReady().
  using PendingTask = base::OnceCallback<void(ConversationShareStore&)>;

  void OnEncryptorReady(scoped_refptr<os_crypt_async::Encryptor> encryptor);

  // Runs |task| once the encryptor has arrived, which may be immediately.
  // Queued tasks are owned by this store and only ever run from one of its
  // methods, so |task| cannot outlive the store it is handed.
  void RunWhenReady(PendingTask task);

  // What is stored in prefs, or std::nullopt when it couldn't be decrypted this
  // session but may well decrypt in a later one, e.g. a locked keyring: nothing
  // may be written over records in that state. Data which will never be
  // readable is discarded here instead, so that one bad write can't stop shares
  // from being recorded for good.
  std::optional<std::vector<ShareRecord>> ReadRecords();

  void WriteRecords(const std::vector<ShareRecord>& records);

  // Removes records the sharing server will have deleted by now. Returns
  // whether anything was removed.
  static bool DropExpiredRecords(std::vector<ShareRecord>& records);

  // Drops expired records from prefs and re-arms |expiry_timer_|. This is what
  // the timer runs.
  void PurgeExpiredRecords();

  // Arms |expiry_timer_| for when the oldest of |records| expires, or cancels
  // it when there are none. Must be called after any change to what is stored.
  void ScheduleNextPurge(const std::vector<ShareRecord>& records);

  raw_ptr<PrefService> prefs_;

  // Null until the encryptor has arrived.
  scoped_refptr<os_crypt_async::Encryptor> encryptor_;

  // Operations which arrived before the encryptor did.
  std::vector<PendingTask> pending_tasks_;

  base::OneShotTimer expiry_timer_;

  base::WeakPtrFactory<ConversationShareStore> weak_ptr_factory_{this};
};

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_CONVERSATION_SHARE_STORE_H_
