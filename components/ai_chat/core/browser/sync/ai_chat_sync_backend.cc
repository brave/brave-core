/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ai_chat/core/browser/sync/ai_chat_sync_backend.h"

#include <utility>

#include "base/check.h"
#include "brave/components/ai_chat/core/browser/sync/ai_chat_sync_bridge.h"
#include "components/sync/model/data_type_local_change_processor.h"

namespace ai_chat {

AIChatSyncBackend::AIChatSyncBackend(
    scoped_refptr<base::SequencedTaskRunner> owning_task_runner)
    : base::RefCountedDeleteOnSequence<AIChatSyncBackend>(
          std::move(owning_task_runner)) {
  DETACH_FROM_SEQUENCE(sequence_checker_);
}

AIChatSyncBackend::~AIChatSyncBackend() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
}

void AIChatSyncBackend::SetBridge(std::unique_ptr<AIChatSyncBridge> bridge) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  CHECK(!bridge_) << "Sync bridge already installed";
  bridge_ = std::move(bridge);
}

void AIChatSyncBackend::SetDatabase(AIChatDatabase* database) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (bridge_) {
    bridge_->SetDatabase(database);
  }
}

void AIChatSyncBackend::ClearDatabase() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (bridge_) {
    bridge_->ClearDatabase();
  }
}

base::WeakPtr<syncer::DataTypeControllerDelegate>
AIChatSyncBackend::GetControllerDelegate() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (!bridge_) {
    return nullptr;
  }
  return bridge_->change_processor()->GetControllerDelegate();
}

void AIChatSyncBackend::Shutdown() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  bridge_.reset();
}

void AIChatSyncBackend::OnConversationAdded(const std::string& uuid) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (bridge_) {
    bridge_->OnConversationAdded(uuid);
  }
}

void AIChatSyncBackend::OnConversationModified(const std::string& uuid) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (bridge_) {
    bridge_->OnConversationModified(uuid);
  }
}

void AIChatSyncBackend::OnConversationDeleted(const std::string& uuid) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (bridge_) {
    bridge_->OnConversationDeleted(uuid);
  }
}

void AIChatSyncBackend::OnConversationEntryAdded(
    const std::string& conversation_uuid,
    const std::string& entry_uuid) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (bridge_) {
    bridge_->OnConversationEntryAdded(conversation_uuid, entry_uuid);
  }
}

void AIChatSyncBackend::OnConversationEntryModified(
    const std::string& conversation_uuid,
    const std::string& entry_uuid) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (bridge_) {
    bridge_->OnConversationEntryModified(conversation_uuid, entry_uuid);
  }
}

void AIChatSyncBackend::OnConversationEntryDeleted(
    const std::string& entry_uuid) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (bridge_) {
    bridge_->OnConversationEntryDeleted(entry_uuid);
  }
}

}  // namespace ai_chat
