/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_SYNC_AI_CHAT_SYNC_BACKEND_H_
#define BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_SYNC_AI_CHAT_SYNC_BACKEND_H_

#include <memory>
#include <string>

#include "base/memory/ref_counted_delete_on_sequence.h"
#include "base/memory/weak_ptr.h"
#include "base/sequence_checker.h"
#include "base/task/sequenced_task_runner.h"

namespace syncer {
class DataTypeControllerDelegate;
}  // namespace syncer

namespace ai_chat {

class AIChatDatabase;
class AIChatSyncBridge;

// Thread-safe, sequence-affine holder for the AIChatSyncBridge. It lets
// AIChatService hand out a ProxyDataTypeControllerDelegate (bound to this
// backend) before the bridge itself has been created on the database sequence.
// The bridge is owned, set, accessed, and destroyed only on the owning
// sequence; the backend is shared with the UI thread, which only ever copies
// it into cross-sequence PostTasks. Outbound local-change notifications are
// routed through here rather than via a WeakPtr to the bridge, so the posted
// closure keeps the backend alive across the hop and the bridge is only ever
// touched on its own sequence.
//
// The backend is created once and lives for the whole AIChatService lifetime;
// it is never swapped out. This keeps the ProxyDataTypeControllerDelegate that
// the sync engine holds pointing at the same object, so it always resolves to
// the current bridge. The database, by contrast, comes and goes with the
// on-disk storage pref and is attached/detached via SetDatabase()/
// ClearDatabase().
class AIChatSyncBackend
    : public base::RefCountedDeleteOnSequence<AIChatSyncBackend> {
 public:
  explicit AIChatSyncBackend(
      scoped_refptr<base::SequencedTaskRunner> owning_task_runner);

  AIChatSyncBackend(const AIChatSyncBackend&) = delete;
  AIChatSyncBackend& operator=(const AIChatSyncBackend&) = delete;

  // Called on the owning sequence to install the bridge once it has been
  // constructed (without a database yet). Must be called at most once.
  void SetBridge(std::unique_ptr<AIChatSyncBridge> bridge);

  // Attaches/detaches the database on the owning sequence, forwarding to the
  // bridge. No-ops if the bridge has been released by Shutdown().
  void SetDatabase(AIChatDatabase* database);
  void ClearDatabase();

  // Returns the bridge's change-processor controller delegate, or a null
  // weak_ptr if the bridge has not been installed yet (or has been released by
  // Shutdown()). Called by ProxyDataTypeControllerDelegate on the owning
  // sequence.
  base::WeakPtr<syncer::DataTypeControllerDelegate> GetControllerDelegate();

  // Drops the bridge on the owning sequence (only at AIChatService shutdown) so
  // its raw_ptr to the AIChatDatabase is released before the database is
  // destroyed. The backend itself may still be kept alive by proxy delegates
  // the sync engine holds; GetControllerDelegate() returns null after this
  // point.
  void Shutdown();

  // Outbound local-change notifications, forwarded to the bridge on the owning
  // sequence. Each call is a no-op once Shutdown() has released the bridge.
  void OnConversationAdded(const std::string& uuid);
  void OnConversationModified(const std::string& uuid);
  void OnConversationDeleted(const std::string& uuid);
  void OnConversationEntryAdded(const std::string& conversation_uuid,
                                const std::string& entry_uuid);
  void OnConversationEntryModified(const std::string& conversation_uuid,
                                   const std::string& entry_uuid);
  void OnConversationEntryDeleted(const std::string& entry_uuid);

 private:
  friend class base::RefCountedDeleteOnSequence<AIChatSyncBackend>;
  friend class base::DeleteHelper<AIChatSyncBackend>;
  ~AIChatSyncBackend();

  std::unique_ptr<AIChatSyncBridge> bridge_;
  SEQUENCE_CHECKER(sequence_checker_);
};

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_SYNC_AI_CHAT_SYNC_BACKEND_H_
