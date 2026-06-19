/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_SYNC_AI_CHAT_SYNC_BACKEND_H_
#define BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_SYNC_AI_CHAT_SYNC_BACKEND_H_

#include <memory>

#include "base/memory/ref_counted_delete_on_sequence.h"
#include "base/memory/weak_ptr.h"
#include "base/sequence_checker.h"
#include "base/task/sequenced_task_runner.h"

namespace syncer {
class DataTypeControllerDelegate;
}  // namespace syncer

namespace ai_chat {

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
class AIChatSyncBackend
    : public base::RefCountedDeleteOnSequence<AIChatSyncBackend> {
 public:
  explicit AIChatSyncBackend(
      scoped_refptr<base::SequencedTaskRunner> owning_task_runner);

  AIChatSyncBackend(const AIChatSyncBackend&) = delete;
  AIChatSyncBackend& operator=(const AIChatSyncBackend&) = delete;

  // Called on the owning sequence to install the bridge once it has been
  // constructed. Must be called at most once.
  void SetBridge(std::unique_ptr<AIChatSyncBridge> bridge);

  // Returns the bridge's change-processor controller delegate, or a null
  // weak_ptr if the bridge has not been installed yet (or has been released by
  // Shutdown()). Called by ProxyDataTypeControllerDelegate on the owning
  // sequence.
  base::WeakPtr<syncer::DataTypeControllerDelegate> GetControllerDelegate();

  // Drops the bridge on the owning sequence so its raw_ptr to the
  // AIChatDatabase is released before the database is destroyed. The backend
  // itself may still be kept alive by proxy delegates the sync engine holds;
  // GetControllerDelegate() returns null after this point.
  void Shutdown();

 private:
  friend class base::RefCountedDeleteOnSequence<AIChatSyncBackend>;
  friend class base::DeleteHelper<AIChatSyncBackend>;
  ~AIChatSyncBackend();

  std::unique_ptr<AIChatSyncBridge> bridge_;
  SEQUENCE_CHECKER(sequence_checker_);
};

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_SYNC_AI_CHAT_SYNC_BACKEND_H_
