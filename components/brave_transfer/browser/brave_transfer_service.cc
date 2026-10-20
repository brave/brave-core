// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_transfer/browser/brave_transfer_service.h"

#include <algorithm>
#include <utility>

#include "base/base64.h"
#include "base/check_op.h"
#include "base/containers/span.h"
#include "base/functional/bind.h"
#include "base/logging.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/strings/string_view_util.h"
#include "base/task/sequenced_task_runner.h"
#include "base/task/thread_pool.h"
#include "base/uuid.h"
#include "base/values.h"
#include "brave/components/brave_transfer/browser/byte_stream.h"
#include "brave/components/brave_transfer/browser/connector.h"
#include "brave/components/brave_transfer/core/filename_sanitizer.h"
#include "brave/components/brave_transfer/core/frame_codec.h"
#include "brave/components/brave_transfer/core/pref_names.h"
#include "components/os_crypt/async/browser/os_crypt_async.h"
#include "components/os_crypt/async/common/encryptor.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"
#include "url/gurl.h"

namespace brave_transfer {

namespace {

constexpr char kKeyField[] = "key";
constexpr char kNameField[] = "name";
constexpr char kPolicyField[] = "policy";
constexpr char kLastSeenField[] = "last_seen";
constexpr char kRemovedField[] = "removed";

constexpr base::TimeDelta kProgressInterval = base::Milliseconds(200);
constexpr uint32_t kMaxInboxPage = 500;

std::string NewId() {
  return base::Uuid::GenerateRandomV4().AsLowercaseString();
}

bool IsFinal(mojom::TransferState state) {
  return state != mojom::TransferState::kWaitingForResponse &&
         state != mojom::TransferState::kInProgress;
}

bool IsTextual(proto::ItemKind kind) {
  return kind == proto::ITEM_KIND_TEXT || kind == proto::ITEM_KIND_URL;
}

mojom::ItemKind ToMojomKind(proto::ItemKind kind) {
  switch (kind) {
    case proto::ITEM_KIND_TEXT:
      return mojom::ItemKind::kText;
    case proto::ITEM_KIND_URL:
      return mojom::ItemKind::kLink;
    case proto::ITEM_KIND_IMAGE:
      return mojom::ItemKind::kImage;
    default:
      return mojom::ItemKind::kFile;
  }
}

mojom::TransferItemPtr ToMojomItem(uint32_t item_id,
                                   proto::ItemKind kind,
                                   const std::string& name,
                                   const std::string& mime_type,
                                   uint64_t size,
                                   base::span<const uint8_t> inline_data) {
  auto item = mojom::TransferItem::New();
  item->id = base::NumberToString(item_id);
  item->kind = ToMojomKind(kind);
  if (!IsTextual(kind)) {
    item->name = name;
  }
  item->mime_type = mime_type;
  item->size = size;
  if (IsTextual(kind)) {
    item->text = std::string(base::as_string_view(inline_data));
  }
  return item;
}

mojom::TransferItemPtr ToMojomItem(const proto::OfferItem& item) {
  return ToMojomItem(item.item_id(), item.kind(), item.name(), item.mime_type(),
                     item.size(), base::as_byte_span(item.inline_data()));
}

mojom::InboxItemPtr ToMojomInboxItem(const InboxEntry& entry) {
  auto item = mojom::InboxItem::New();
  item->id = base::NumberToString(entry.id);
  item->received_time = entry.received_time.InMillisecondsFSinceUnixEpoch();
  item->device_name = entry.device_name;
  item->kind = entry.kind;
  item->text = entry.text;
  item->file_name = entry.file_name;
  item->mime_type = entry.mime_type;
  item->size = entry.size;
  item->file_missing = entry.file_missing;
  item->saved_to_fallback = entry.saved_to_fallback;
  return item;
}

std::string TruncateUTF8(std::string value, size_t max_bytes) {
  if (value.size() > max_bytes) {
    base::TruncateUTF8ToByteSize(value, max_bytes, &value);
  }
  return value;
}

}  // namespace

BraveTransferService::ConnectionInfo::ConnectionInfo() = default;
BraveTransferService::ConnectionInfo::ConnectionInfo(ConnectionInfo&&) =
    default;
BraveTransferService::ConnectionInfo&
BraveTransferService::ConnectionInfo::operator=(ConnectionInfo&&) = default;
BraveTransferService::ConnectionInfo::~ConnectionInfo() = default;

BraveTransferService::TransferRecord::TransferRecord() = default;
BraveTransferService::TransferRecord::TransferRecord(TransferRecord&&) =
    default;
BraveTransferService::TransferRecord&
BraveTransferService::TransferRecord::operator=(TransferRecord&&) = default;
BraveTransferService::TransferRecord::~TransferRecord() = default;

BraveTransferService::PendingOffer::PendingOffer() = default;
BraveTransferService::PendingOffer::PendingOffer(PendingOffer&&) = default;
BraveTransferService::PendingOffer&
BraveTransferService::PendingOffer::operator=(PendingOffer&&) = default;
BraveTransferService::PendingOffer::~PendingOffer() = default;

// static
void BraveTransferService::RegisterProfilePrefs(PrefRegistrySimple* registry) {
  registry->RegisterBooleanPref(prefs::kEnabled, true);
  registry->RegisterIntegerPref(
      prefs::kReceiveMode,
      static_cast<int>(mojom::ReceiveMode::kTrustedDevices));
  registry->RegisterBooleanPref(prefs::kAutoAcceptTrusted, false);
  registry->RegisterBooleanPref(prefs::kAutoTrustSyncChain, true);
  registry->RegisterBooleanPref(prefs::kNotifyOnReceive, true);
  registry->RegisterFilePathPref(prefs::kSaveDirectory, base::FilePath());
  registry->RegisterStringPref(prefs::kStaticKey, std::string());
  registry->RegisterDictionaryPref(prefs::kTrustedDevices);
  registry->RegisterBooleanPref(prefs::kShowToolbarButton, true);
}

BraveTransferService::BraveTransferService(
    PrefService* prefs,
    const base::FilePath& inbox_path,
    os_crypt_async::OSCryptAsync* os_crypt_async,
    std::unique_ptr<TrustSource> trust_source,
    std::unique_ptr<DiscoveryProvider> discovery,
    std::unique_ptr<Connector> connector,
    std::unique_ptr<IncomingFileSink> file_sink,
    std::unique_ptr<Delegate> delegate)
    : prefs_(prefs),
      trust_source_(std::move(trust_source)),
      discovery_(std::move(discovery)),
      connector_(std::move(connector)),
      file_sink_(std::move(file_sink)),
      delegate_(std::move(delegate)),
      inbox_(base::ThreadPool::CreateSequencedTaskRunner(
                 {base::MayBlock(), base::TaskPriority::USER_VISIBLE,
                  base::TaskShutdownBehavior::BLOCK_SHUTDOWN}),
             inbox_path) {
  CHECK(prefs_);
  CHECK(trust_source_);
  CHECK(discovery_);
  CHECK(connector_);
  CHECK(file_sink_);
  CHECK(delegate_);
  trust_observation_.Observe(trust_source_.get());

  pref_change_registrar_.Init(prefs_);
  for (const char* pref : {prefs::kReceiveMode, prefs::kAutoAcceptTrusted,
                           prefs::kAutoTrustSyncChain, prefs::kNotifyOnReceive,
                           prefs::kSaveDirectory}) {
    pref_change_registrar_.Add(
        pref, base::BindRepeating(&BraveTransferService::OnPrefChanged,
                                  base::Unretained(this)));
  }
  pref_change_registrar_.Add(
      prefs::kEnabled,
      base::BindRepeating(&BraveTransferService::OnEnabledChanged,
                          base::Unretained(this)));

  if (os_crypt_async) {
    os_crypt_async->GetInstance(base::BindOnce(
        &BraveTransferService::OnEncryptorReady, weak_factory_.GetWeakPtr()));
  }
}

BraveTransferService::~BraveTransferService() = default;

void BraveTransferService::Shutdown() {
  idle_timer_.Stop();
  StopDiscovery();
  connections_.clear();
  pending_offers_.clear();
  trust_observation_.Reset();
  pref_change_registrar_.RemoveAll();
  weak_factory_.InvalidateWeakPtrs();
}

void BraveTransferService::AddObserver(Observer* observer) {
  observers_.AddObserver(observer);
}

void BraveTransferService::RemoveObserver(Observer* observer) {
  observers_.RemoveObserver(observer);
}

void BraveTransferService::AddPage() {
  ++page_count_;
  idle_timer_.Stop();
  UpdateDiscovery();
}

void BraveTransferService::RemovePage() {
  CHECK_GT(page_count_, 0);
  if (--page_count_ > 0) {
    return;
  }
  StopDiscovery();
  idle_timer_.Start(FROM_HERE, kIdleGracePeriod,
                    base::BindOnce(&BraveTransferService::CloseAllConnections,
                                   weak_factory_.GetWeakPtr()));
}

bool BraveTransferService::IsEnabled() const {
  return prefs_->GetBoolean(prefs::kEnabled);
}

bool BraveTransferService::IsDiscoveryActive() const {
  return discovery_->IsStarted();
}

mojom::ServiceStatePtr BraveTransferService::GetServiceState() const {
  auto state = mojom::ServiceState::New();
  state->sync_enabled = trust_source_->IsSyncActive();
  state->discovery_active = IsDiscoveryActive();
  return state;
}

std::vector<mojom::DevicePtr> BraveTransferService::GetDevices() const {
  std::vector<mojom::DevicePtr> devices;
  if (!IsChainTrustActive()) {
    return devices;
  }
  const base::DictValue& trusted = prefs_->GetDict(prefs::kTrustedDevices);
  const double now = base::Time::Now().InMillisecondsFSinceUnixEpoch();
  for (const ChainDevice& chain_device : trust_source_->GetChainDevices()) {
    const base::DictValue* entry = trusted.FindDict(chain_device.guid);
    if (entry && entry->FindBool(kRemovedField).value_or(false)) {
      continue;
    }
    auto device = mojom::Device::New();
    device->id = chain_device.guid;
    device->name = chain_device.name;
    device->type = chain_device.type;
    device->source = mojom::TrustSource::kSyncChain;
    device->policy = GetDevicePolicy(chain_device.guid);
    device->is_connected = !!FindReadyConnection(chain_device.guid);
    if (entry) {
      device->last_seen_time = entry->FindDouble(kLastSeenField).value_or(0);
      if (const std::string* key = entry->FindString(kKeyField)) {
        if (std::optional<std::vector<uint8_t>> bytes =
                base::Base64Decode(*key)) {
          device->key_fingerprint = static_key::Fingerprint(*bytes);
        }
      }
    }
    if (device->is_connected) {
      device->last_seen_time = now;
    }
    devices.push_back(std::move(device));
  }
  return devices;
}

bool BraveTransferService::SetDevicePolicy(const std::string& device_id,
                                           mojom::DevicePolicy policy) {
  if (!mojom::IsKnownEnumValue(policy) || device_id.empty()) {
    return false;
  }
  {
    ScopedDictPrefUpdate update(prefs_, prefs::kTrustedDevices);
    update->EnsureDict(device_id)->Set(kPolicyField, static_cast<int>(policy));
  }
  NotifyDevicesChanged();
  return true;
}

bool BraveTransferService::RemoveDevice(const std::string& device_id) {
  if (device_id.empty()) {
    return false;
  }
  {
    ScopedDictPrefUpdate update(prefs_, prefs::kTrustedDevices);
    base::DictValue* entry = update->EnsureDict(device_id);
    entry->Remove(kKeyField);
    entry->Set(kRemovedField, true);
  }
  for (auto& [id, info] : connections_) {
    if (info.device_guid == device_id) {
      info.peer->Close();
    }
  }
  NotifyDevicesChanged();
  return true;
}

mojom::SendResult BraveTransferService::SendText(const std::string& device_id,
                                                 const std::string& text) {
  if (text.empty()) {
    return mojom::SendResult::kFailed;
  }
  if (text.size() > kMaxSendBytes) {
    return mojom::SendResult::kTooLarge;
  }
  const std::string_view trimmed =
      base::TrimWhitespaceASCII(text, base::TRIM_ALL);
  const GURL url(trimmed);
  std::vector<OutgoingItem> items;
  if (url.is_valid() && url.SchemeIsHTTPOrHTTPS() &&
      trimmed.size() <= kMaxInlineBytes &&
      trimmed.find_first_of(" \t\r\n") == std::string_view::npos) {
    const std::string spec(trimmed);
    items.push_back(OutgoingItem::FromBytes(
        proto::ITEM_KIND_URL,
        spec.size() <= kMaxNameBytes ? spec : std::string("link"),
        "text/uri-list", Bytes(spec.begin(), spec.end())));
  } else {
    items.push_back(OutgoingItem::FromBytes(proto::ITEM_KIND_TEXT, "text.txt",
                                            "text/plain",
                                            Bytes(text.begin(), text.end())));
  }
  return QueueSend(device_id, std::move(items));
}

mojom::SendResult BraveTransferService::SendFiles(
    const std::string& device_id,
    std::vector<mojom::FileDataPtr> files) {
  if (files.empty() || files.size() > kMaxItems) {
    return mojom::SendResult::kFailed;
  }
  uint64_t total = 0;
  for (const mojom::FileDataPtr& file : files) {
    total += file->data.size();
  }
  if (total > kMaxSendBytes) {
    return mojom::SendResult::kTooLarge;
  }
  std::vector<OutgoingItem> items;
  for (mojom::FileDataPtr& file : files) {
    const bool is_image = base::StartsWith(
        file->mime_type, "image/", base::CompareCase::INSENSITIVE_ASCII);
    std::string name = TruncateUTF8(file->name, kMaxNameBytes);
    if (name.empty()) {
      name = "file";
    }
    std::string mime_type = file->mime_type.size() <= kMaxMimeTypeBytes
                                ? file->mime_type
                                : std::string();
    if (mime_type.empty()) {
      mime_type = "application/octet-stream";
    }
    items.push_back(OutgoingItem::FromBytes(
        is_image ? proto::ITEM_KIND_IMAGE : proto::ITEM_KIND_FILE,
        std::move(name), std::move(mime_type), std::move(file->data)));
  }
  return QueueSend(device_id, std::move(items));
}

mojom::SendResult BraveTransferService::QueueSend(
    const std::string& device_id,
    std::vector<OutgoingItem> items) {
  if (!IsEnabled()) {
    return mojom::SendResult::kFailed;
  }
  PeerConnection* peer = FindReadyConnection(device_id);
  if (!peer) {
    return mojom::SendResult::kDeviceUnavailable;
  }
  TransferRecord record;
  record.id = NewId();
  record.device_id = device_id;
  record.device_name = FindConnection(peer->id())->device_name;
  record.direction = mojom::TransferDirection::kSend;
  record.state = mojom::TransferState::kWaitingForResponse;
  uint32_t item_id = 1;
  for (const OutgoingItem& item : items) {
    record.items.push_back(ToMojomItem(item_id++, item.kind, item.name,
                                       item.mime_type, item.size,
                                       item.inline_data));
    record.bytes_total += item.size;
  }
  record.outgoing_items = std::move(items);

  PruneFinishedTransfers();
  const std::string id = record.id;
  TransferRecord& stored =
      transfers_.emplace(id, std::move(record)).first->second;
  transfer_order_.push_back(id);
  NotifyTransfer(stored);
  StartNextSend(device_id);
  return stored.state == mojom::TransferState::kFailed
             ? mojom::SendResult::kFailed
             : mojom::SendResult::kSuccess;
}

void BraveTransferService::StartNextSend(const std::string& device_guid) {
  PeerConnection* peer = FindReadyConnection(device_guid);
  if (!peer || peer->HasOutgoingTransfer()) {
    return;
  }
  for (const std::string& id : transfer_order_) {
    TransferRecord& record = transfers_.at(id);
    if (record.direction != mojom::TransferDirection::kSend ||
        record.device_id != device_guid || record.connection_id != 0 ||
        record.state != mojom::TransferState::kWaitingForResponse ||
        record.outgoing_items.empty()) {
      continue;
    }
    std::vector<OutgoingItem> items = std::move(record.outgoing_items);
    record.outgoing_items.clear();
    Result<std::string> protocol_id = peer->SendOffer(std::move(items));
    if (!protocol_id.has_value()) {
      LOG(ERROR) << "Brave Transfer: cannot send offer: "
                 << protocol_id.error();
      SetTransferState(record, mojom::TransferState::kFailed);
      continue;
    }
    record.connection_id = peer->id();
    record.protocol_id = std::move(protocol_id.value());
    NotifyTransfer(record);
    return;
  }
}

void BraveTransferService::CancelTransfer(const std::string& transfer_id) {
  auto it = transfers_.find(transfer_id);
  if (it == transfers_.end() || IsFinal(it->second.state)) {
    return;
  }
  TransferRecord& record = it->second;
  if (record.connection_id == 0) {
    record.outgoing_items.clear();
    SetTransferState(record, mojom::TransferState::kCancelled);
    return;
  }
  if (ConnectionInfo* info = FindConnection(record.connection_id)) {
    info->peer->Cancel(record.protocol_id);
  }
}

std::vector<mojom::TransferPtr> BraveTransferService::GetTransfers() const {
  std::vector<mojom::TransferPtr> result;
  for (const std::string& id : transfer_order_) {
    const TransferRecord& record = transfers_.at(id);
    std::vector<mojom::TransferItemPtr> items;
    for (const mojom::TransferItemPtr& item : record.items) {
      items.push_back(item.Clone());
    }
    result.push_back(
        mojom::Transfer::New(record.id, record.device_id, record.device_name,
                             record.direction, record.state, std::move(items),
                             record.bytes_transferred, record.bytes_total));
  }
  return result;
}

std::vector<mojom::IncomingOfferPtr> BraveTransferService::GetPendingOffers()
    const {
  std::vector<mojom::IncomingOfferPtr> offers;
  for (const auto& [id, pending] : pending_offers_) {
    offers.push_back(pending.info.Clone());
  }
  return offers;
}

bool BraveTransferService::RespondToOffer(const std::string& offer_id,
                                          bool accept) {
  auto it = pending_offers_.find(offer_id);
  if (it == pending_offers_.end()) {
    return false;
  }
  PendingOffer pending = std::move(it->second);
  pending_offers_.erase(it);
  observers_.Notify(&Observer::OnOfferRemoved, offer_id);

  ConnectionInfo* info = FindConnection(pending.connection_id);
  if (!info || !info->peer->is_ready()) {
    return false;
  }
  if (accept) {
    AcceptOffer(pending.connection_id, pending.protocol_id, pending.offer);
  } else {
    info->peer->Respond(pending.protocol_id, proto::DECISION_DECLINE);
  }
  return true;
}

void BraveTransferService::GetInbox(uint32_t offset,
                                    uint32_t count,
                                    GetInboxCallback callback) {
  inbox_.AsyncCall(&InboxStore::Get)
      .WithArgs(offset, std::min(count, kMaxInboxPage))
      .Then(base::BindOnce(
          [](GetInboxCallback callback, InboxPage page) {
            std::vector<mojom::InboxItemPtr> items;
            for (const InboxEntry& entry : page.entries) {
              items.push_back(ToMojomInboxItem(entry));
            }
            std::move(callback).Run(std::move(items), page.has_more);
          },
          std::move(callback)));
}

void BraveTransferService::GetInboxEntry(const std::string& item_id,
                                         GetInboxEntryCallback callback) {
  int64_t id = 0;
  if (!base::StringToInt64(item_id, &id)) {
    std::move(callback).Run(std::nullopt);
    return;
  }
  inbox_.AsyncCall(&InboxStore::GetById).WithArgs(id).Then(std::move(callback));
}

void BraveTransferService::ClearHistory(
    base::OnceCallback<void(bool success)> callback) {
  inbox_.AsyncCall(&InboxStore::Clear)
      .Then(base::BindOnce(&BraveTransferService::OnHistoryCleared,
                           weak_factory_.GetWeakPtr(), std::move(callback)));
}

void BraveTransferService::OnHistoryCleared(
    base::OnceCallback<void(bool success)> callback,
    bool success) {
  if (success) {
    observers_.Notify(&Observer::OnInboxChanged);
  }
  std::move(callback).Run(success);
}

mojom::SettingsPtr BraveTransferService::GetSettings() const {
  auto settings = mojom::Settings::New();
  const auto receive_mode =
      static_cast<mojom::ReceiveMode>(prefs_->GetInteger(prefs::kReceiveMode));
  settings->receive_mode = mojom::IsKnownEnumValue(receive_mode)
                               ? receive_mode
                               : mojom::ReceiveMode::kTrustedDevices;
  settings->auto_accept_trusted = prefs_->GetBoolean(prefs::kAutoAcceptTrusted);
  settings->auto_trust_sync_chain =
      prefs_->GetBoolean(prefs::kAutoTrustSyncChain);
  settings->notify_on_receive = prefs_->GetBoolean(prefs::kNotifyOnReceive);
  settings->save_directory =
      prefs_->GetFilePath(prefs::kSaveDirectory).AsUTF8Unsafe();
  settings->default_save_directory =
      delegate_->GetDefaultSaveDirectory().AsUTF8Unsafe();
  return settings;
}

bool BraveTransferService::UpdateSettings(mojom::SettingsPtr settings) {
  if (!settings || !mojom::IsKnownEnumValue(settings->receive_mode)) {
    return false;
  }
  prefs_->SetInteger(prefs::kReceiveMode,
                     static_cast<int>(settings->receive_mode));
  prefs_->SetBoolean(prefs::kAutoAcceptTrusted, settings->auto_accept_trusted);
  prefs_->SetBoolean(prefs::kAutoTrustSyncChain,
                     settings->auto_trust_sync_chain);
  prefs_->SetBoolean(prefs::kNotifyOnReceive, settings->notify_on_receive);
  // The page can only reset the save folder. Choosing one goes through the
  // browser's folder picker, so a renderer cannot pick an arbitrary path.
  if (settings->save_directory.empty()) {
    prefs_->ClearPref(prefs::kSaveDirectory);
  }
  return true;
}

void BraveTransferService::SetSaveDirectory(const base::FilePath& directory) {
  prefs_->SetFilePath(prefs::kSaveDirectory, directory);
}

void BraveTransferService::AcceptIncoming(std::unique_ptr<ByteStream> stream) {
  if (!stream || !static_key_ || !IsChainTrustActive()) {
    return;
  }
  std::optional<ChainKeys> keys = trust_source_->GetChainKeys();
  if (!keys) {
    return;
  }
  AddConnection(PeerConnection::CreateResponder(
                    next_connection_id_++, std::move(stream), *static_key_,
                    ResponderHandshake::PskForChain(*keys, base::Time::Now()),
                    LocalPeerInfo(), *this),
                std::string());
}

void BraveTransferService::OnTrustChanged() {
  if (!IsChainTrustActive()) {
    CloseAllConnections();
  } else {
    PruneTrustedDevices();
  }
  UpdateDiscovery();
  NotifyServiceStateChanged();
  NotifyDevicesChanged();
}

bool BraveTransferService::OnPeerAuthenticated(PeerConnection& connection) {
  ConnectionInfo* info = FindConnection(connection.id());
  if (!info || !IsChainTrustActive()) {
    return false;
  }
  const std::string& guid = connection.remote_info().sync_device_guid();
  if (guid.empty() || guid == trust_source_->GetLocalDeviceGuid()) {
    return false;
  }
  std::optional<ChainDevice> device;
  for (ChainDevice& chain_device : trust_source_->GetChainDevices()) {
    if (chain_device.guid == guid) {
      device = std::move(chain_device);
      break;
    }
  }
  if (!device) {
    DVLOG(1) << "Brave Transfer: peer is not in the Sync chain";
    return false;
  }
  for (const auto& [id, other] : connections_) {
    if (id != connection.id() && other.device_guid == guid &&
        !other.peer->is_closed()) {
      return false;
    }
  }

  const std::string key = base::Base64Encode(connection.remote_static_key());
  if (const base::DictValue* entry =
          prefs_->GetDict(prefs::kTrustedDevices).FindDict(guid)) {
    if (entry->FindBool(kRemovedField).value_or(false)) {
      return false;
    }
    const std::string* pinned = entry->FindString(kKeyField);
    if (pinned && *pinned != key) {
      LOG(WARNING) << "Brave Transfer: a Sync device presented a different "
                      "key than the pinned one";
      delegate_->NotifyKeyMismatch(device->name);
      return false;
    }
  }
  info->device_guid = guid;
  info->device_name = device->name;
  return true;
}

void BraveTransferService::OnConnectionReady(PeerConnection& connection) {
  ConnectionInfo* info = FindConnection(connection.id());
  if (!info) {
    return;
  }
  const std::string guid = info->device_guid;
  // Pinned only once both sides have accepted the session, so that a peer
  // that rejects this device is not pinned.
  {
    ScopedDictPrefUpdate update(prefs_, prefs::kTrustedDevices);
    base::DictValue* entry = update->EnsureDict(guid);
    entry->Set(kKeyField, base::Base64Encode(connection.remote_static_key()));
    entry->Set(kNameField, info->device_name);
    entry->Set(kLastSeenField,
               base::Time::Now().InMillisecondsFSinceUnixEpoch());
  }
  if (!info->instance_id.empty()) {
    retry_delays_.erase(info->instance_id);
  }
  NotifyDevicesChanged();
  StartNextSend(guid);
}

void BraveTransferService::OnTransferEvent(PeerConnection& connection,
                                           TransferEvent event) {
  ConnectionInfo* info = FindConnection(connection.id());
  if (!info) {
    return;
  }
  if (auto* offer = std::get_if<OfferReceived>(&event)) {
    HandleOffer(*info, std::move(offer->offer));
  } else if (auto* response = std::get_if<ResponseReceived>(&event)) {
    HandleResponse(connection, *response);
  } else if (auto* inline_item = std::get_if<InlineItemReceived>(&event)) {
    HandleInlineItem(connection, *inline_item);
  } else if (auto* chunk = std::get_if<ChunkReceived>(&event)) {
    HandleChunk(connection, std::move(*chunk));
  } else if (auto* item = std::get_if<ItemReceived>(&event)) {
    HandleItem(connection, *item);
  } else if (auto* finished = std::get_if<TransferFinished>(&event)) {
    HandleFinished(connection, finished->result);
  }
}

void BraveTransferService::OnChunkSent(PeerConnection& connection,
                                       const std::string& transfer_id,
                                       size_t bytes) {
  TransferRecord* record = FindTransfer(connection.id(), transfer_id);
  if (!record) {
    return;
  }
  record->bytes_transferred += bytes;
  NotifyTransfer(*record, /*force=*/false);
}

void BraveTransferService::OnConnectionClosed(PeerConnection& connection,
                                              const std::string& reason) {
  auto it = connections_.find(connection.id());
  if (it == connections_.end()) {
    return;
  }
  ConnectionInfo info = std::move(it->second);
  connections_.erase(it);
  DVLOG(1) << "Brave Transfer: connection closed: " << reason;

  FailTransfersFor(connection.id());
  std::vector<std::string> offer_ids;
  for (const auto& [id, pending] : pending_offers_) {
    if (pending.connection_id == connection.id()) {
      offer_ids.push_back(id);
    }
  }
  for (const std::string& id : offer_ids) {
    RemovePendingOffer(id);
  }

  if (!info.device_guid.empty()) {
    for (const std::string& id : transfer_order_) {
      TransferRecord& record = transfers_.at(id);
      if (record.device_id == info.device_guid && record.connection_id == 0 &&
          !IsFinal(record.state)) {
        record.outgoing_items.clear();
        SetTransferState(record, mojom::TransferState::kFailed);
      }
    }
    if (prefs_->GetDict(prefs::kTrustedDevices).FindDict(info.device_guid)) {
      ScopedDictPrefUpdate update(prefs_, prefs::kTrustedDevices);
      update->EnsureDict(info.device_guid)
          ->Set(kLastSeenField,
                base::Time::Now().InMillisecondsFSinceUnixEpoch());
    }
    NotifyDevicesChanged();
  }

  base::SequencedTaskRunner::GetCurrentDefault()->DeleteSoon(
      FROM_HERE, std::move(info.peer));
  if (!info.instance_id.empty()) {
    ScheduleRetry(info.instance_id);
  }
}

void BraveTransferService::OnEncryptorReady(
    scoped_refptr<os_crypt_async::Encryptor> encryptor) {
  const std::string stored = prefs_->GetString(prefs::kStaticKey);
  if (!stored.empty()) {
    std::string ciphertext;
    std::string plaintext;
    if (base::Base64Decode(stored, &ciphertext) &&
        encryptor->DecryptString(ciphertext, &plaintext) &&
        plaintext.size() == kKeySize) {
      Key key;
      base::span(key).copy_from(base::as_byte_span(plaintext));
      static_key_ = key;
    } else {
      LOG(WARNING) << "Brave Transfer: cannot decrypt the device key; "
                      "generating a new one";
    }
  }
  if (!static_key_) {
    const Key key = static_key::GeneratePrivateKey();
    std::string ciphertext;
    if (!encryptor->EncryptString(std::string(base::as_string_view(key)),
                                  &ciphertext)) {
      LOG(ERROR) << "Brave Transfer: cannot encrypt the device key";
      return;
    }
    prefs_->SetString(prefs::kStaticKey, base::Base64Encode(ciphertext));
    static_key_ = key;
  }
  UpdateDiscovery();
}

void BraveTransferService::OnPrefChanged() {
  observers_.Notify(&Observer::OnSettingsChanged);
  if (!IsChainTrustActive()) {
    CloseAllConnections();
  }
  UpdateDiscovery();
  NotifyDevicesChanged();
}

void BraveTransferService::OnEnabledChanged() {
  if (!IsEnabled()) {
    StopDiscovery();
    CloseAllConnections();
  } else {
    UpdateDiscovery();
  }
  NotifyServiceStateChanged();
  NotifyDevicesChanged();
}

bool BraveTransferService::IsChainTrustActive() const {
  return IsEnabled() && prefs_->GetBoolean(prefs::kAutoTrustSyncChain) &&
         trust_source_->IsSyncActive();
}

proto::PeerInfo BraveTransferService::LocalPeerInfo() const {
  proto::PeerInfo info;
  info.set_protocol_version(kProtocolVersion);
  std::string name =
      TruncateUTF8(trust_source_->GetLocalDeviceName(), kMaxDeviceNameBytes);
  info.set_device_name(name.empty() ? std::string("Brave") : name);
  info.set_device_type(proto::DEVICE_TYPE_DESKTOP);
  info.set_sync_device_guid(trust_source_->GetLocalDeviceGuid());
  info.set_capabilities(kCapabilityReceive | kCapabilitySend);
  return info;
}

void BraveTransferService::UpdateDiscovery() {
  std::optional<ChainKeys> keys;
  if (page_count_ > 0 && static_key_ && IsChainTrustActive()) {
    keys = trust_source_->GetChainKeys();
  }
  if (!keys) {
    StopDiscovery();
    return;
  }
  if (discovery_->IsStarted() && discovery_keys_ == keys) {
    return;
  }
  StopDiscovery();
  discovery_keys_ = std::move(keys);
  discovery_->Start(*discovery_keys_,
                    base::BindRepeating(&BraveTransferService::OnCandidate,
                                        weak_factory_.GetWeakPtr()),
                    base::BindRepeating(&BraveTransferService::OnCandidateLost,
                                        weak_factory_.GetWeakPtr()));
  NotifyServiceStateChanged();
}

void BraveTransferService::StopDiscovery() {
  const bool was_started = discovery_->IsStarted();
  if (was_started) {
    discovery_->Stop();
  }
  candidates_.clear();
  retry_delays_.clear();
  discovery_keys_.reset();
  if (was_started) {
    NotifyServiceStateChanged();
  }
}

void BraveTransferService::OnCandidate(const PeerCandidate& candidate) {
  if (!discovery_->IsStarted() || candidate.instance_id.empty()) {
    return;
  }
  candidates_.insert_or_assign(candidate.instance_id, candidate);
  MaybeConnect(candidate.instance_id);
}

void BraveTransferService::OnCandidateLost(const std::string& instance_id) {
  candidates_.erase(instance_id);
  retry_delays_.erase(instance_id);
}

void BraveTransferService::MaybeConnect(const std::string& instance_id) {
  auto it = candidates_.find(instance_id);
  if (it == candidates_.end() || !discovery_keys_ || !static_key_ ||
      dialing_.contains(instance_id)) {
    return;
  }
  for (const auto& [id, info] : connections_) {
    if (info.instance_id == instance_id && !info.peer->is_closed()) {
      return;
    }
  }
  dialing_.insert(instance_id);
  connector_->Connect(it->second.endpoints,
                      base::BindOnce(&BraveTransferService::OnConnected,
                                     weak_factory_.GetWeakPtr(), instance_id));
}

void BraveTransferService::OnConnected(const std::string& instance_id,
                                       std::unique_ptr<ByteStream> stream) {
  dialing_.erase(instance_id);
  if (!stream) {
    ScheduleRetry(instance_id);
    return;
  }
  auto it = candidates_.find(instance_id);
  if (it == candidates_.end() || !discovery_keys_ || !static_key_) {
    return;
  }
  AddConnection(
      PeerConnection::CreateInitiator(next_connection_id_++, std::move(stream),
                                      *static_key_, discovery_keys_->psk(),
                                      it->second.tag, LocalPeerInfo(), *this),
      instance_id);
}

void BraveTransferService::ScheduleRetry(const std::string& instance_id) {
  if (!candidates_.contains(instance_id)) {
    return;
  }
  auto [it, inserted] = retry_delays_.try_emplace(instance_id, kMinRetryDelay);
  const base::TimeDelta delay = it->second;
  it->second = std::min(delay * 2, kMaxRetryDelay);
  base::SequencedTaskRunner::GetCurrentDefault()->PostDelayedTask(
      FROM_HERE,
      base::BindOnce(&BraveTransferService::MaybeConnect,
                     weak_factory_.GetWeakPtr(), instance_id),
      delay);
}

void BraveTransferService::AddConnection(std::unique_ptr<PeerConnection> peer,
                                         const std::string& instance_id) {
  if (!peer) {
    return;
  }
  PeerConnection* raw = peer.get();
  ConnectionInfo info;
  info.peer = std::move(peer);
  info.instance_id = instance_id;
  connections_.emplace(raw->id(), std::move(info));
  raw->Start();
}

void BraveTransferService::CloseAllConnections() {
  for (auto& [id, info] : connections_) {
    info.peer->Close();
  }
}

void BraveTransferService::PruneTrustedDevices() {
  if (!trust_source_->IsSyncActive()) {
    return;
  }
  base::flat_set<std::string> chain_guids;
  for (const ChainDevice& device : trust_source_->GetChainDevices()) {
    chain_guids.insert(device.guid);
  }
  std::vector<std::string> stale;
  for (const auto [guid, value] : prefs_->GetDict(prefs::kTrustedDevices)) {
    if (!chain_guids.contains(guid)) {
      stale.push_back(guid);
    }
  }
  if (!stale.empty()) {
    ScopedDictPrefUpdate update(prefs_, prefs::kTrustedDevices);
    for (const std::string& guid : stale) {
      update->Remove(guid);
    }
  }
  for (auto& [id, info] : connections_) {
    if (!info.device_guid.empty() && !chain_guids.contains(info.device_guid)) {
      info.peer->Close();
    }
  }
}

BraveTransferService::ConnectionInfo* BraveTransferService::FindConnection(
    int connection_id) {
  auto it = connections_.find(connection_id);
  return it == connections_.end() ? nullptr : &it->second;
}

PeerConnection* BraveTransferService::FindReadyConnection(
    const std::string& device_guid) const {
  if (device_guid.empty()) {
    return nullptr;
  }
  for (const auto& [id, info] : connections_) {
    if (info.device_guid == device_guid && info.peer->is_ready()) {
      return info.peer.get();
    }
  }
  return nullptr;
}

BraveTransferService::TransferRecord* BraveTransferService::FindTransfer(
    int connection_id,
    const std::string& protocol_id) {
  for (auto& [id, record] : transfers_) {
    if (record.connection_id == connection_id &&
        record.protocol_id == protocol_id && !IsFinal(record.state)) {
      return &record;
    }
  }
  return nullptr;
}

std::optional<std::string> BraveTransferService::FindPendingOffer(
    int connection_id,
    const std::string& protocol_id) {
  for (const auto& [id, pending] : pending_offers_) {
    if (pending.connection_id == connection_id &&
        pending.protocol_id == protocol_id) {
      return id;
    }
  }
  return std::nullopt;
}

void BraveTransferService::HandleOffer(ConnectionInfo& info,
                                       proto::Offer offer) {
  PeerConnection& peer = *info.peer;
  const auto receive_mode =
      static_cast<mojom::ReceiveMode>(prefs_->GetInteger(prefs::kReceiveMode));
  const mojom::DevicePolicy policy = GetDevicePolicy(info.device_guid);
  if (receive_mode == mojom::ReceiveMode::kOff ||
      policy == mojom::DevicePolicy::kBlock) {
    peer.Respond(offer.transfer_id(), proto::DECISION_DECLINE);
    return;
  }
  const bool auto_accept = policy == mojom::DevicePolicy::kAlwaysAccept ||
                           (policy == mojom::DevicePolicy::kDefault &&
                            prefs_->GetBoolean(prefs::kAutoAcceptTrusted));
  if (auto_accept) {
    AcceptOffer(peer.id(), offer.transfer_id(), offer);
    return;
  }

  PendingOffer pending;
  pending.connection_id = peer.id();
  pending.protocol_id = offer.transfer_id();
  pending.info = mojom::IncomingOffer::New();
  pending.info->id = NewId();
  pending.info->device_id = info.device_guid;
  pending.info->device_name = info.device_name;
  for (const proto::OfferItem& item : offer.items()) {
    pending.info->items.push_back(ToMojomItem(item));
  }
  pending.info->expiry_time =
      (base::Time::Now() + kOfferTimeout).InMillisecondsFSinceUnixEpoch();
  pending.offer = std::move(offer);

  const std::string id = pending.info->id;
  const mojom::IncomingOffer& stored =
      *pending_offers_.emplace(id, std::move(pending)).first->second.info;
  observers_.Notify(&Observer::OnOfferReceived, stored);
  if (prefs_->GetBoolean(prefs::kNotifyOnReceive)) {
    delegate_->NotifyOfferPending(stored);
  }
}

void BraveTransferService::AcceptOffer(int connection_id,
                                       const std::string& protocol_id,
                                       const proto::Offer& offer) {
  ConnectionInfo* info = FindConnection(connection_id);
  if (!info || !info->peer->is_ready()) {
    return;
  }
  TransferRecord record;
  record.id = NewId();
  record.connection_id = connection_id;
  record.protocol_id = protocol_id;
  record.device_id = info->device_guid;
  record.device_name = info->device_name;
  record.direction = mojom::TransferDirection::kReceive;
  record.state = mojom::TransferState::kInProgress;
  for (const proto::OfferItem& item : offer.items()) {
    record.items.push_back(ToMojomItem(item));
    record.bytes_total += item.size();
  }

  PruneFinishedTransfers();
  const std::string id = record.id;
  TransferRecord& stored =
      transfers_.emplace(id, std::move(record)).first->second;
  transfer_order_.push_back(id);

  for (const proto::OfferItem& item : offer.items()) {
    if (!item.inline_data().empty()) {
      continue;
    }
    if (IsTextual(item.kind()) && item.size() <= kMaxInboxTextBytes) {
      stored.text_buffers.emplace(item.item_id(), std::string());
      continue;
    }
    std::string name = item.name();
    if (IsTextual(item.kind()) &&
        !base::EndsWith(name, ".txt", base::CompareCase::INSENSITIVE_ASCII)) {
      name += ".txt";
    }
    stored.open_files.insert(item.item_id());
    file_sink_->Begin(FileKey(stored, item.item_id()), SanitizeFilename(name),
                      base::BindOnce(&BraveTransferService::OnFileWriteFailed,
                                     weak_factory_.GetWeakPtr(), id));
  }
  NotifyTransfer(stored);
  info->peer->Respond(protocol_id, proto::DECISION_ACCEPT_ALL);
}

void BraveTransferService::HandleResponse(PeerConnection& connection,
                                          const ResponseReceived& response) {
  TransferRecord* record = FindTransfer(connection.id(), response.transfer_id);
  if (!record) {
    return;
  }
  if (response.decision == proto::DECISION_ACCEPT_SOME) {
    const base::flat_set<uint32_t> accepted(response.accepted_item_ids);
    record->bytes_total = 0;
    for (size_t i = 0; i < record->items.size(); ++i) {
      if (accepted.contains(static_cast<uint32_t>(i + 1))) {
        record->bytes_total += record->items[i]->size;
      }
    }
  }
  if (response.decision == proto::DECISION_ACCEPT_ALL ||
      response.decision == proto::DECISION_ACCEPT_SOME) {
    SetTransferState(*record, mojom::TransferState::kInProgress);
  }
}

void BraveTransferService::HandleInlineItem(PeerConnection& connection,
                                            const InlineItemReceived& item) {
  TransferRecord* record = FindTransfer(connection.id(), item.transfer_id);
  if (!record) {
    return;
  }
  InboxEntry entry;
  entry.received_time = base::Time::Now();
  entry.device_guid = record->device_id;
  entry.device_name = record->device_name;
  entry.kind = ToMojomKind(item.item.kind());
  entry.text = item.item.inline_data();
  entry.mime_type = item.item.mime_type();
  entry.size = item.item.size();
  entry.sha256 = base::HexEncodeLower(item.item.sha256());
  ++record->items_saved;
  AddInboxEntry(std::move(entry));
}

void BraveTransferService::HandleChunk(PeerConnection& connection,
                                       ChunkReceived chunk) {
  TransferRecord* record = FindTransfer(connection.id(), chunk.transfer_id);
  if (!record) {
    return;
  }
  record->bytes_transferred += chunk.data.size();
  if (auto it = record->text_buffers.find(chunk.item_id);
      it != record->text_buffers.end()) {
    it->second.append(chunk.data);
  } else if (record->open_files.contains(chunk.item_id)) {
    file_sink_->Append(FileKey(*record, chunk.item_id), std::move(chunk.data));
  }
  NotifyTransfer(*record, /*force=*/false);
}

void BraveTransferService::HandleItem(PeerConnection& connection,
                                      const ItemReceived& item) {
  TransferRecord* record = FindTransfer(connection.id(), item.transfer_id);
  if (!record) {
    return;
  }
  const uint32_t item_id = item.item.item_id();
  InboxEntry entry;
  entry.received_time = base::Time::Now();
  entry.device_guid = record->device_id;
  entry.device_name = record->device_name;
  entry.kind = ToMojomKind(item.item.kind());
  entry.mime_type = item.item.mime_type();
  entry.size = item.item.size();
  entry.sha256 = base::HexEncodeLower(item.item.sha256());

  if (auto it = record->text_buffers.find(item_id);
      it != record->text_buffers.end()) {
    entry.text = std::move(it->second);
    record->text_buffers.erase(it);
    ++record->items_saved;
    AddInboxEntry(std::move(entry));
    return;
  }
  if (record->open_files.erase(item_id)) {
    if (IsTextual(item.item.kind())) {
      entry.kind = mojom::ItemKind::kFile;
    }
    file_sink_->Commit(FileKey(*record, item_id),
                       base::BindOnce(&BraveTransferService::OnFileCommitted,
                                      weak_factory_.GetWeakPtr(), record->id,
                                      std::move(entry)));
  }
}

void BraveTransferService::HandleFinished(PeerConnection& connection,
                                          const TransferResult& result) {
  if (!result.is_outgoing) {
    if (std::optional<std::string> offer_id =
            FindPendingOffer(connection.id(), result.transfer_id)) {
      RemovePendingOffer(*offer_id);
    }
  }
  TransferRecord* record = FindTransfer(connection.id(), result.transfer_id);
  if (!record) {
    return;
  }
  mojom::TransferState state = mojom::TransferState::kFailed;
  if (result.status == proto::DONE_STATUS_OK) {
    state = mojom::TransferState::kCompleted;
  } else if (result.decision == proto::DECISION_DECLINE ||
             result.decision == proto::DECISION_BUSY ||
             result.decision == proto::DECISION_NO_SPACE) {
    state = mojom::TransferState::kDeclined;
  } else if (result.cancel_reason == proto::CANCEL_REASON_TIMEOUT) {
    state = mojom::TransferState::kExpired;
  } else if (result.cancel_reason == proto::CANCEL_REASON_USER) {
    state = mojom::TransferState::kCancelled;
  }

  if (state == mojom::TransferState::kCompleted) {
    record->bytes_transferred = record->bytes_total;
  }
  if (!result.is_outgoing) {
    for (uint32_t item_id : record->open_files) {
      file_sink_->Abort(FileKey(*record, item_id));
    }
    record->open_files.clear();
    record->text_buffers.clear();
    if (state == mojom::TransferState::kCompleted &&
        prefs_->GetBoolean(prefs::kNotifyOnReceive)) {
      delegate_->NotifyItemsReceived(record->device_name, record->items.size());
    }
  }
  record->connection_id = 0;
  const std::string device_id = record->device_id;
  SetTransferState(*record, state);
  if (result.is_outgoing) {
    StartNextSend(device_id);
  }
}

void BraveTransferService::OnFileWriteFailed(const std::string& transfer_id) {
  auto it = transfers_.find(transfer_id);
  if (it == transfers_.end() || IsFinal(it->second.state)) {
    return;
  }
  TransferRecord& record = it->second;
  if (ConnectionInfo* info = FindConnection(record.connection_id)) {
    info->peer->WriteFailed(record.protocol_id);
  }
  if (!IsFinal(record.state)) {
    for (uint32_t item_id : record.open_files) {
      file_sink_->Abort(FileKey(record, item_id));
    }
    record.open_files.clear();
    record.connection_id = 0;
    SetTransferState(record, mojom::TransferState::kFailed);
  }
}

void BraveTransferService::OnFileCommitted(const std::string& transfer_id,
                                           InboxEntry entry,
                                           std::optional<SavedFile> saved) {
  auto it = transfers_.find(transfer_id);
  TransferRecord* record = it == transfers_.end() ? nullptr : &it->second;
  if (!saved) {
    LOG(ERROR) << "Brave Transfer: cannot save a received file";
    if (record && record->state != mojom::TransferState::kFailed) {
      record->connection_id = 0;
      SetTransferState(*record, mojom::TransferState::kFailed);
    }
    return;
  }
  entry.file_path = saved->path;
  entry.file_name = saved->path.BaseName().AsUTF8Unsafe();
  entry.saved_to_fallback = saved->used_fallback;
  if (record) {
    ++record->items_saved;
  }
  AddInboxEntry(std::move(entry));
}

void BraveTransferService::AddInboxEntry(InboxEntry entry) {
  inbox_.AsyncCall(&InboxStore::Add)
      .WithArgs(std::move(entry))
      .Then(base::BindOnce(&BraveTransferService::OnInboxEntryAdded,
                           weak_factory_.GetWeakPtr()));
}

void BraveTransferService::OnInboxEntryAdded(std::optional<int64_t> id) {
  if (!id) {
    LOG(ERROR) << "Brave Transfer: cannot write the inbox";
    return;
  }
  observers_.Notify(&Observer::OnInboxChanged);
}

void BraveTransferService::SetTransferState(TransferRecord& record,
                                            mojom::TransferState state) {
  record.state = state;
  NotifyTransfer(record);
}

void BraveTransferService::NotifyTransfer(TransferRecord& record, bool force) {
  const base::TimeTicks now = base::TimeTicks::Now();
  if (!force && now - record.last_notified < kProgressInterval) {
    return;
  }
  record.last_notified = now;
  std::vector<mojom::TransferItemPtr> items;
  for (const mojom::TransferItemPtr& item : record.items) {
    items.push_back(item.Clone());
  }
  const mojom::TransferPtr transfer =
      mojom::Transfer::New(record.id, record.device_id, record.device_name,
                           record.direction, record.state, std::move(items),
                           record.bytes_transferred, record.bytes_total);
  observers_.Notify(&Observer::OnTransferUpdated, *transfer);
}

void BraveTransferService::PruneFinishedTransfers() {
  size_t finished = 0;
  for (const std::string& id : transfer_order_) {
    if (IsFinal(transfers_.at(id).state)) {
      ++finished;
    }
  }
  for (auto it = transfer_order_.begin();
       it != transfer_order_.end() && finished > kMaxFinishedTransfers;) {
    if (IsFinal(transfers_.at(*it).state)) {
      transfers_.erase(*it);
      it = transfer_order_.erase(it);
      --finished;
    } else {
      ++it;
    }
  }
}

void BraveTransferService::RemovePendingOffer(const std::string& offer_id) {
  if (pending_offers_.erase(offer_id)) {
    observers_.Notify(&Observer::OnOfferRemoved, offer_id);
  }
}

void BraveTransferService::FailTransfersFor(int connection_id) {
  for (const std::string& id : transfer_order_) {
    TransferRecord& record = transfers_.at(id);
    if (record.connection_id != connection_id || IsFinal(record.state)) {
      continue;
    }
    for (uint32_t item_id : record.open_files) {
      file_sink_->Abort(FileKey(record, item_id));
    }
    record.open_files.clear();
    record.text_buffers.clear();
    record.connection_id = 0;
    SetTransferState(record, mojom::TransferState::kFailed);
  }
}

void BraveTransferService::NotifyDevicesChanged() {
  observers_.Notify(&Observer::OnDevicesChanged);
}

void BraveTransferService::NotifyServiceStateChanged() {
  observers_.Notify(&Observer::OnServiceStateChanged);
}

mojom::DevicePolicy BraveTransferService::GetDevicePolicy(
    const std::string& guid) const {
  const base::DictValue* entry =
      prefs_->GetDict(prefs::kTrustedDevices).FindDict(guid);
  if (!entry) {
    return mojom::DevicePolicy::kDefault;
  }
  const auto policy = static_cast<mojom::DevicePolicy>(
      entry->FindInt(kPolicyField)
          .value_or(static_cast<int>(mojom::DevicePolicy::kDefault)));
  return mojom::IsKnownEnumValue(policy) ? policy
                                         : mojom::DevicePolicy::kDefault;
}

std::string BraveTransferService::FileKey(const TransferRecord& record,
                                          uint32_t item_id) const {
  return record.id + "/" + base::NumberToString(item_id);
}

}  // namespace brave_transfer
