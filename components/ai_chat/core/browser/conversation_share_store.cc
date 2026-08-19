// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/browser/conversation_share_store.h"

#include <algorithm>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "base/base64.h"
#include "base/check.h"
#include "base/check_deref.h"
#include "base/functional/bind.h"
#include "base/task/bind_post_task.h"
#include "brave/components/ai_chat/core/common/features.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "brave/components/ai_chat/core/common/pref_names.h"
#include "brave/components/ai_chat/core/proto/store.pb.h"
#include "components/os_crypt/async/browser/os_crypt_async.h"
#include "components/os_crypt/async/common/encryptor.h"
#include "components/prefs/pref_service.h"

namespace ai_chat {

namespace {

base::TimeDelta GetShareLifetime() {
  return base::Days(features::kAIChatConversationShareExpiryDays.Get());
}

base::Time CreatedTime(const store::ConversationShareProto& record) {
  return base::Time::FromDeltaSinceWindowsEpoch(
      base::Microseconds(record.created_time_windows_epoch_micros()));
}

// Whether a stored record has what it takes to list the share and delete it
// from the server. The title is not required: conversations can legitimately
// be untitled.
bool IsUsable(const store::ConversationShareProto& record) {
  return !record.share_id().empty() && !record.deletion_id().empty() &&
         !record.conversation_uuid().empty() &&
         record.created_time_windows_epoch_micros() > 0 &&
         GURL(record.url()).is_valid();
}

}  // namespace

ConversationShareStore::ConversationShareStore(
    PrefService* prefs,
    os_crypt_async::OSCryptAsync* os_crypt_async)
    : prefs_(prefs) {
  CHECK(prefs_);
  CHECK_DEREF(os_crypt_async)
      .GetInstance(base::BindOnce(&ConversationShareStore::OnEncryptorReady,
                                  weak_ptr_factory_.GetWeakPtr()));
}

ConversationShareStore::~ConversationShareStore() = default;

void ConversationShareStore::AddShare(const std::string& share_id,
                                      const std::string& deletion_id,
                                      const std::string& conversation_uuid,
                                      const std::string& conversation_title,
                                      const GURL& url) {
  // Without a deletion id the share can never be deleted from this client, so
  // there is nothing the management UI could usefully do with the record, and
  // it would hold on to the decryption key for no reason.
  if (share_id.empty() || deletion_id.empty() || !url.is_valid()) {
    return;
  }

  store::ConversationShareProto record;
  record.set_share_id(share_id);
  record.set_deletion_id(deletion_id);
  record.set_conversation_uuid(conversation_uuid);
  record.set_conversation_title(conversation_title);
  record.set_url(url.spec());
  record.set_created_time_windows_epoch_micros(
      base::Time::Now().ToDeltaSinceWindowsEpoch().InMicroseconds());

  RunWhenReady(base::BindOnce(
      [](store::ConversationShareProto record, ConversationShareStore& store) {
        // Nothing can be stored safely without a key to encrypt it with.
        if (!store.encryptor_->IsEncryptionAvailable()) {
          return;
        }
        std::unique_ptr<store::ConversationSharesProto> records =
            store.ReadRecords();
        if (!records) {
          return;
        }
        *records->add_shares() = std::move(record);
        store.WriteRecords(*records);
        store.ScheduleNextPurge(*records);
      },
      std::move(record)));
}

void ConversationShareStore::GetShares(GetSharesCallback callback) {
  // Whether the encryptor has arrived yet is an implementation detail, so the
  // callback is always run asynchronously.
  RunWhenReady(base::BindOnce(
      [](GetSharesCallback callback, ConversationShareStore& store) {
        std::unique_ptr<store::ConversationSharesProto> records =
            store.ReadRecords();
        if (!records) {
          // Nothing can be listed from records which can't be read.
          std::move(callback).Run({});
          return;
        }
        // The timer purges these from prefs at the moment they expire, but
        // never report one the server has already deleted.
        DropExpiredRecords(*records);
        // Most recently shared first.
        std::ranges::stable_sort(*records->mutable_shares(), std::greater<>(),
                                 &CreatedTime);

        std::vector<mojom::ConversationSharePtr> shares;
        shares.reserve(records->shares().size());
        for (const store::ConversationShareProto& record : records->shares()) {
          // The link is deliberately not included - it contains the
          // conversation's decryption key, which stays in the browser process.
          shares.push_back(mojom::ConversationShare::New(
              record.share_id(), record.conversation_uuid(),
              record.conversation_title(), CreatedTime(record)));
        }
        std::move(callback).Run(std::move(shares));
      },
      base::BindPostTaskToCurrentDefault(std::move(callback))));
}

void ConversationShareStore::GetDeletionId(const std::string& share_id,
                                           GetDeletionIdCallback callback) {
  RunWhenReady(base::BindOnce(
      [](std::string share_id, GetDeletionIdCallback callback,
         ConversationShareStore& store) {
        std::unique_ptr<store::ConversationSharesProto> records =
            store.ReadRecords();
        if (!records) {
          // Nothing can be found in records which can't be read.
          std::move(callback).Run(std::nullopt);
          return;
        }
        // The server has already deleted anything expired, so there is nothing
        // left to authorize a deletion for.
        DropExpiredRecords(*records);
        const store::ConversationShareProto* record =
            FindRecord(*records, share_id);
        std::move(callback).Run(
            record ? std::optional<std::string>(record->deletion_id())
                   : std::nullopt);
      },
      share_id, base::BindPostTaskToCurrentDefault(std::move(callback))));
}

void ConversationShareStore::GetShareUrl(const std::string& share_id,
                                         GetShareUrlCallback callback) {
  RunWhenReady(base::BindOnce(
      [](std::string share_id, GetShareUrlCallback callback,
         ConversationShareStore& store) {
        std::unique_ptr<store::ConversationSharesProto> records =
            store.ReadRecords();
        if (!records) {
          // Nothing can be found in records which can't be read.
          std::move(callback).Run(std::nullopt);
          return;
        }
        // There is nothing worth copying a link to once the server has deleted
        // it.
        DropExpiredRecords(*records);
        const store::ConversationShareProto* record =
            FindRecord(*records, share_id);
        std::move(callback).Run(record ? std::optional<GURL>(record->url())
                                       : std::nullopt);
      },
      share_id, base::BindPostTaskToCurrentDefault(std::move(callback))));
}

void ConversationShareStore::RemoveShare(const std::string& share_id) {
  RunWhenReady(base::BindOnce(
      [](std::string share_id, ConversationShareStore& store) {
        std::unique_ptr<store::ConversationSharesProto> records =
            store.ReadRecords();
        if (!records) {
          return;
        }
        auto& shares = *records->mutable_shares();
        auto removed = std::ranges::remove_if(
            shares, [&share_id](const store::ConversationShareProto& record) {
              return record.share_id() == share_id;
            });
        if (!removed.empty()) {
          shares.erase(removed.begin(), shares.end());
          store.WriteRecords(*records);
          store.ScheduleNextPurge(*records);
        }
      },
      share_id));
}

void ConversationShareStore::OnEncryptorReady(
    scoped_refptr<os_crypt_async::Encryptor> encryptor) {
  encryptor_ = std::move(encryptor);

  // The sharing server has already deleted anything past its lifetime, so
  // don't hold on to the keys for it.
  PurgeExpiredRecords();

  std::vector<PendingTask> pending_tasks = std::move(pending_tasks_);
  pending_tasks_.clear();
  for (auto& task : pending_tasks) {
    std::move(task).Run(*this);
  }
}

void ConversationShareStore::RunWhenReady(PendingTask task) {
  if (encryptor_) {
    std::move(task).Run(*this);
    return;
  }
  pending_tasks_.push_back(std::move(task));
}

std::unique_ptr<store::ConversationSharesProto>
ConversationShareStore::ReadRecords() {
  CHECK(encryptor_);
  auto records = std::make_unique<store::ConversationSharesProto>();
  const std::string stored =
      prefs_->GetString(prefs::kBraveAIChatConversationShares);
  if (stored.empty()) {
    return records;
  }

  std::optional<std::vector<uint8_t>> ciphertext = base::Base64Decode(stored);
  const bool decryption_available = encryptor_->IsDecryptionAvailable();
  os_crypt_async::Encryptor::DecryptFlags flags;
  std::optional<std::string> serialized =
      ciphertext && decryption_available
          ? encryptor_->DecryptData(*ciphertext, &flags)
          : std::nullopt;
  if (!serialized) {
    // A key can be missing for this session alone, e.g. a locked keyring, which
    // says nothing about whether what is stored is still good.
    if (ciphertext &&
        (!decryption_available || flags.temporarily_unavailable)) {
      return nullptr;
    }
    // Anything else will never be readable, so discard it rather than let it
    // stop shares from being recorded from now on.
    prefs_->ClearPref(prefs::kBraveAIChatConversationShares);
    return records;
  }

  // Whether anything stored couldn't be understood, in which case it is
  // rewritten without it.
  bool discarded_unreadable = false;
  if (records->ParseFromString(*serialized)) {
    auto& shares = *records->mutable_shares();
    auto unusable = std::ranges::remove_if(shares, std::not_fn(&IsUsable));
    if (!unusable.empty()) {
      shares.erase(unusable.begin(), shares.end());
      discarded_unreadable = true;
    }
  } else {
    // A failed parse can still have left fields behind.
    records->Clear();
    discarded_unreadable = true;
  }
  if (discarded_unreadable) {
    WriteRecords(*records);
  }
  return records;
}

// static
const store::ConversationShareProto* ConversationShareStore::FindRecord(
    const store::ConversationSharesProto& records,
    const std::string& share_id) {
  auto it = std::ranges::find(records.shares(), share_id,
                              &store::ConversationShareProto::share_id);
  return it == records.shares().end() ? nullptr : &*it;
}

// static
bool ConversationShareStore::DropExpiredRecords(
    store::ConversationSharesProto& records) {
  const base::Time expired_before = base::Time::Now() - GetShareLifetime();
  auto& shares = *records.mutable_shares();
  auto expired = std::ranges::remove_if(
      shares, [expired_before](const store::ConversationShareProto& record) {
        return CreatedTime(record) <= expired_before;
      });
  if (expired.empty()) {
    return false;
  }
  shares.erase(expired.begin(), shares.end());
  return true;
}

void ConversationShareStore::PurgeExpiredRecords() {
  std::unique_ptr<store::ConversationSharesProto> records = ReadRecords();
  if (!records) {
    // Nothing can be dropped from records which can't be read, and there is no
    // expiry to arm the timer for.
    expiry_timer_.Stop();
    return;
  }
  if (DropExpiredRecords(*records)) {
    WriteRecords(*records);
  }
  ScheduleNextPurge(*records);
}

void ConversationShareStore::ScheduleNextPurge(
    const store::ConversationSharesProto& records) {
  // Leaving no pending task when there is nothing to expire matters beyond
  // efficiency: a perpetually armed timer makes RunLoop::Run() advance mock
  // time forever in tests which never share a conversation.
  if (records.shares().empty()) {
    expiry_timer_.Stop();
    return;
  }

  auto oldest =
      std::ranges::min_element(records.shares(), std::less<>(), &CreatedTime);
  const base::TimeDelta delay =
      CreatedTime(*oldest) + GetShareLifetime() - base::Time::Now();
  expiry_timer_.Start(
      FROM_HERE, std::max(delay, base::TimeDelta()),
      base::BindOnce(&ConversationShareStore::PurgeExpiredRecords,
                     base::Unretained(this)));
}

void ConversationShareStore::WriteRecords(
    const store::ConversationSharesProto& records) {
  CHECK(encryptor_);
  if (records.shares().empty()) {
    prefs_->ClearPref(prefs::kBraveAIChatConversationShares);
    return;
  }

  std::string serialized;
  std::optional<std::vector<uint8_t>> ciphertext;
  if (records.SerializeToString(&serialized)) {
    ciphertext = encryptor_->EncryptString(serialized);
  }
  // Leave what is stored alone rather than fall back to writing the decryption
  // keys in the clear.
  if (!ciphertext) {
    return;
  }
  prefs_->SetString(prefs::kBraveAIChatConversationShares,
                    base::Base64Encode(*ciphertext));
}

}  // namespace ai_chat
