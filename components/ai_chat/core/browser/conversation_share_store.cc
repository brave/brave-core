// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/browser/conversation_share_store.h"

#include <algorithm>
#include <functional>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "base/base64.h"
#include "base/check.h"
#include "base/check_deref.h"
#include "base/functional/bind.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/json/values_util.h"
#include "base/values.h"
#include "brave/components/ai_chat/core/common/features.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "brave/components/ai_chat/core/common/pref_names.h"
#include "components/os_crypt/async/browser/os_crypt_async.h"
#include "components/os_crypt/async/common/encryptor.h"
#include "components/prefs/pref_service.h"

namespace ai_chat {

namespace {

constexpr char kShareIdKey[] = "share_id";
constexpr char kDeletionIdKey[] = "deletion_id";
constexpr char kTitleKey[] = "title";
constexpr char kUrlKey[] = "url";
constexpr char kCreatedTimeKey[] = "created_time";

base::TimeDelta GetShareLifetime() {
  return base::Days(features::kAIChatConversationShareExpiryDays.Get());
}

}  // namespace

// static
std::optional<ConversationShareStore::ShareRecord>
ConversationShareStore::ShareRecordFromValue(const base::Value& value) {
  const base::DictValue* dict = value.GetIfDict();
  if (!dict) {
    return std::nullopt;
  }
  const std::string* share_id = dict->FindString(kShareIdKey);
  const std::string* deletion_id = dict->FindString(kDeletionIdKey);
  const std::string* url = dict->FindString(kUrlKey);
  const base::Value* created_time = dict->Find(kCreatedTimeKey);
  if (!share_id || !deletion_id || !url || !created_time) {
    return std::nullopt;
  }
  std::optional<base::Time> parsed_time = base::ValueToTime(*created_time);
  if (!parsed_time) {
    return std::nullopt;
  }

  ShareRecord record;
  record.share_id = *share_id;
  record.deletion_id = *deletion_id;
  const std::string* title = dict->FindString(kTitleKey);
  record.conversation_title = title ? *title : std::string();
  record.url = GURL(*url);
  record.created_time = *parsed_time;
  if (!record.url.is_valid()) {
    return std::nullopt;
  }
  return record;
}

ConversationShareStore::ShareRecord::ShareRecord() = default;
ConversationShareStore::ShareRecord::ShareRecord(const ShareRecord&) = default;
ConversationShareStore::ShareRecord&
ConversationShareStore::ShareRecord::operator=(const ShareRecord&) = default;
ConversationShareStore::ShareRecord::~ShareRecord() = default;

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
                                      const std::string& conversation_title,
                                      const GURL& url) {
  // Without a deletion id the share can never be deleted from this client, so
  // there is nothing the management UI could usefully do with the record, and
  // it would hold on to the decryption key for no reason.
  if (share_id.empty() || deletion_id.empty() || !url.is_valid()) {
    return;
  }

  ShareRecord record;
  record.share_id = share_id;
  record.deletion_id = deletion_id;
  record.conversation_title = conversation_title;
  record.url = url;
  record.created_time = base::Time::Now();

  RunWhenLoaded(base::BindOnce(&ConversationShareStore::AddShareInternal,
                               weak_ptr_factory_.GetWeakPtr(),
                               std::move(record)));
}

void ConversationShareStore::GetShares(GetSharesCallback callback) {
  RunWhenLoaded(base::BindOnce(&ConversationShareStore::GetSharesInternal,
                               weak_ptr_factory_.GetWeakPtr(),
                               std::move(callback)));
}

void ConversationShareStore::OnEncryptorReady(
    scoped_refptr<os_crypt_async::Encryptor> encryptor) {
  encryptor_ = std::move(encryptor);
  records_ = std::vector<ShareRecord>();

  // Read whatever is already stored. Anything that doesn't decrypt or parse is
  // dropped: it is unusable, and rewriting the pref discards it for good.
  const std::string stored =
      prefs_->GetString(prefs::kBraveAIChatConversationShares);
  std::optional<std::vector<uint8_t>> ciphertext =
      stored.empty() ? std::nullopt : base::Base64Decode(stored);
  std::optional<std::string> json =
      ciphertext && encryptor_->IsDecryptionAvailable()
          ? encryptor_->DecryptData(*ciphertext)
          : std::nullopt;
  std::optional<base::ListValue> parsed =
      json ? base::JSONReader::ReadList(*json, base::JSON_PARSE_RFC)
           : std::nullopt;

  // Whether the stored pref held anything that couldn't be read back, in which
  // case it is rewritten to discard it.
  bool discarded_unreadable = !stored.empty() && !parsed;

  if (parsed) {
    for (const base::Value& item : *parsed) {
      std::optional<ShareRecord> record = ShareRecordFromValue(item);
      if (!record) {
        discarded_unreadable = true;
        continue;
      }
      records_->push_back(std::move(*record));
    }
  }

  // The sharing server has already deleted anything past its lifetime, so
  // don't hold on to the keys for it.
  if (DropExpiredRecords() || discarded_unreadable) {
    WriteRecordsToPrefs();
  }
  ScheduleNextPurge();

  std::vector<base::OnceClosure> pending_tasks = std::move(pending_tasks_);
  pending_tasks_.clear();
  for (auto& task : pending_tasks) {
    std::move(task).Run();
  }
}

void ConversationShareStore::RunWhenLoaded(base::OnceClosure task) {
  if (records_) {
    std::move(task).Run();
    return;
  }
  pending_tasks_.push_back(std::move(task));
}

void ConversationShareStore::AddShareInternal(ShareRecord record) {
  CHECK(records_);
  // Nothing can be stored safely without a key to encrypt it with.
  if (!encryptor_->IsEncryptionAvailable()) {
    return;
  }
  records_->push_back(std::move(record));
  WriteRecordsToPrefs();
  ScheduleNextPurge();
}

void ConversationShareStore::GetSharesInternal(GetSharesCallback callback) {
  CHECK(records_);
  std::vector<const ShareRecord*> ordered;
  ordered.reserve(records_->size());
  for (const ShareRecord& record : *records_) {
    ordered.push_back(&record);
  }
  // Most recently shared first.
  std::ranges::stable_sort(ordered, std::greater<>(),
                           &ShareRecord::created_time);

  std::vector<mojom::ConversationSharePtr> shares;
  shares.reserve(ordered.size());
  for (const ShareRecord* record : ordered) {
    // The link is deliberately not included - it contains the conversation's
    // decryption key, which stays in the browser process.
    shares.push_back(mojom::ConversationShare::New(
        record->share_id, record->conversation_title, record->created_time));
  }
  std::move(callback).Run(std::move(shares));
}

bool ConversationShareStore::DropExpiredRecords() {
  CHECK(records_);
  const base::Time expired_before = base::Time::Now() - GetShareLifetime();
  return std::erase_if(*records_, [expired_before](const ShareRecord& record) {
           return record.created_time <= expired_before;
         }) > 0;
}

void ConversationShareStore::PurgeExpiredRecords() {
  if (DropExpiredRecords()) {
    WriteRecordsToPrefs();
  }
  ScheduleNextPurge();
}

void ConversationShareStore::ScheduleNextPurge() {
  CHECK(records_);
  // Leaving no pending task when there is nothing to expire matters beyond
  // efficiency: a perpetually armed timer makes RunLoop::Run() advance mock
  // time forever in tests which never share a conversation.
  if (records_->empty()) {
    expiry_timer_.Stop();
    return;
  }

  auto oldest = std::ranges::min_element(*records_, std::less<>(),
                                         &ShareRecord::created_time);
  const base::TimeDelta delay =
      oldest->created_time + GetShareLifetime() - base::Time::Now();
  expiry_timer_.Start(
      FROM_HERE, std::max(delay, base::TimeDelta()),
      base::BindOnce(&ConversationShareStore::PurgeExpiredRecords,
                     base::Unretained(this)));
}

void ConversationShareStore::WriteRecordsToPrefs() {
  CHECK(records_);
  if (records_->empty()) {
    prefs_->ClearPref(prefs::kBraveAIChatConversationShares);
    return;
  }

  base::ListValue list;
  for (const ShareRecord& record : *records_) {
    base::DictValue dict;
    dict.Set(kShareIdKey, record.share_id);
    dict.Set(kDeletionIdKey, record.deletion_id);
    dict.Set(kTitleKey, record.conversation_title);
    dict.Set(kUrlKey, record.url.spec());
    dict.Set(kCreatedTimeKey, base::TimeToValue(record.created_time));
    list.Append(base::Value(std::move(dict)));
  }

  std::string json;
  std::optional<std::vector<uint8_t>> ciphertext;
  if (base::JSONWriter::Write(list, &json)) {
    ciphertext = encryptor_->EncryptString(json);
  }
  if (!ciphertext) {
    // Never fall back to writing the decryption keys in the clear.
    prefs_->ClearPref(prefs::kBraveAIChatConversationShares);
    return;
  }
  prefs_->SetString(prefs::kBraveAIChatConversationShares,
                    base::Base64Encode(*ciphertext));
}

}  // namespace ai_chat
