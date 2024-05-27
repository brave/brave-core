// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ipfs/pin/ipfs_local_pin_service.h"

#include <memory>
#include <optional>
#include <set>
#include <string>
#include <utility>

#include "base/containers/contains.h"
#include "base/functional/callback.h"
#include "base/strings/strcat.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "brave/components/ipfs/pref_names.h"
#include "components/prefs/scoped_user_pref_update.h"

namespace ipfs {

bool PinData::operator==(const PinData& d) const {
  return this->cid == d.cid && this->pinning_mode == d.pinning_mode;
}

namespace {
const char kRecursiveMode[] = "recursive";
const char kDirectMode[] = "direct";

std::string GetPrefNameFromPinningMode(PinningMode mode) {
  switch (mode) {
    case DIRECT:
      return kDirectMode;
    case RECURSIVE:
      return kRecursiveMode;
  }
  NOTREACHED_IN_MIGRATION();
  return kRecursiveMode;
}

}  // namespace

// Splits ipfs:// url to a list of PinData items
std::optional<std::vector<PinData>> IpfsLocalPinService::ExtractPinData(
    const std::string& ipfs_url) {
  auto gurl = GURL(ipfs_url);
  if (!gurl.SchemeIs(ipfs::kIPFSScheme)) {
    return std::nullopt;
  }
  // This will just remove ipfs:// scheme
  auto path = gurl.path();
  std::vector<std::string> slit =
      base::SplitString(path, "/", base::WhitespaceHandling::KEEP_WHITESPACE,
                        base::SplitResult::SPLIT_WANT_NONEMPTY);
  if (slit.empty()) {
    return std::nullopt;
  }
  std::vector<PinData> result;
  std::string builder = "/ipfs";
  for (const auto& part : slit) {
    builder += "/";
    builder += part;
    result.push_back(PinData{builder, PinningMode::DIRECT});
  }
  result[result.size() - 1].pinning_mode = PinningMode::RECURSIVE;
  return result;
}

std::optional<std::vector<ipfs::PinData>>
IpfsLocalPinService::ExtractMergedPinData(
    const std::vector<std::string>& ipfs_urls) {
  std::vector<ipfs::PinData> result;
  for (const auto& ipfs_url : ipfs_urls) {
    auto pins_data_for_url = ExtractPinData(ipfs_url);
    if (!pins_data_for_url) {
      return std::nullopt;
    }
    for (const auto& item : pins_data_for_url.value()) {
      if (!base::Contains(result, item)) {
        result.push_back(item);
      }
    }
  }
  return result;
}

AddLocalPinJob::AddLocalPinJob(PrefService* prefs_service,
                               IpfsService* ipfs_service,
                               const std::string& key,
                               const std::vector<PinData>& pins_data,
                               AddPinCallback callback)
    : prefs_service_(prefs_service),
      ipfs_service_(ipfs_service),
      key_(key),
      pins_data_(pins_data),
      callback_(std::move(callback)) {}

AddLocalPinJob::~AddLocalPinJob() = default;

void AddLocalPinJob::Start() {
  auto callback = base::BarrierCallback<std::optional<AddPinResult>>(
      2, base::BindOnce(&AddLocalPinJob::OnAddPinResult,
                        weak_ptr_factory_.GetWeakPtr()));

  std::vector<std::string> recursive_cids;
  std::vector<std::string> direct_cids;

  for (const auto& pin_data : pins_data_) {
    if (pin_data.pinning_mode == PinningMode::RECURSIVE) {
      recursive_cids.push_back(pin_data.cid);
    } else {
      direct_cids.push_back(pin_data.cid);
    }
  }

  ipfs_service_->AddPin(
      recursive_cids, true,
      base::BindOnce(&AddLocalPinJob::Accumulate,
                     weak_ptr_factory_.GetWeakPtr(), callback));
  ipfs_service_->AddPin(
      direct_cids, false,
      base::BindOnce(&AddLocalPinJob::Accumulate,
                     weak_ptr_factory_.GetWeakPtr(), callback));
}

void AddLocalPinJob::Accumulate(
    base::OnceCallback<void(std::optional<AddPinResult>)> callback,
    std::optional<AddPinResult> result) {
  if (!result) {
    pinning_failed_ = true;
  }

  std::move(callback).Run(std::move(result));
}

void AddLocalPinJob::OnAddPinResult(
    std::vector<std::optional<AddPinResult>> result) {
  if (is_canceled_) {
    std::move(callback_).Run(false);
    return;
  }

  if (pinning_failed_) {
    std::move(callback_).Run(false);
    return;
  }

  {
    ScopedDictPrefUpdate update(prefs_service_, kIPFSPinnedCids);
    base::Value::Dict& update_dict = update.Get();

    for (const auto& add_pin_result : result) {
      auto* mode_dict = update_dict.EnsureDict(GetPrefNameFromPinningMode(
          add_pin_result->recursive ? PinningMode::RECURSIVE
                                    : PinningMode::DIRECT));
      for (const auto& cid : add_pin_result->pins) {
        base::Value::List* list = mode_dict->EnsureList(cid);
        list->EraseValue(base::Value(key_));
        list->Append(base::Value(key_));
      }
    }
  }
  std::move(callback_).Run(true);
}

RemoveLocalPinJob::RemoveLocalPinJob(PrefService* prefs_service,
                                     const std::string& key,
                                     RemovePinCallback callback)
    : prefs_service_(prefs_service),
      key_(key),
      callback_(std::move(callback)) {}

RemoveLocalPinJob::~RemoveLocalPinJob() = default;

void RemoveLocalPinJob::Start() {
  {
    ScopedDictPrefUpdate update(prefs_service_, kIPFSPinnedCids);
    base::Value::Dict& pinning_modes_dict = update.Get();
    // Iterate over pinning modes
    for (const auto pair : pinning_modes_dict) {
      auto* cids_dict = pair.second.GetIfDict();
      if (!cids_dict) {
        NOTREACHED_IN_MIGRATION() << "Corrupted prefs structure.";
        continue;
      }

      std::vector<std::string> remove_list;
      // Iterate over CIDs
      for (const auto cid : *cids_dict) {
        base::Value::List* list = cid.second.GetIfList();
        if (list) {
          list->EraseValue(base::Value(key_));
          if (list->empty()) {
            remove_list.push_back(cid.first);
          }
        }
      }
      for (const auto& cid : remove_list) {
        cids_dict->Remove(cid);
      }
    }
  }
  std::move(callback_).Run(true);
}

VerifyLocalPinJob::VerifyLocalPinJob(PrefService* prefs_service,
                                     IpfsService* ipfs_service,
                                     const std::string& key,
                                     const std::vector<PinData>& pins_data,
                                     ValidatePinsCallback callback)
    : prefs_service_(prefs_service),
      ipfs_service_(ipfs_service),
      key_(key),
      pins_data_(pins_data),
      callback_(std::move(callback)) {}

VerifyLocalPinJob::~VerifyLocalPinJob() = default;

void VerifyLocalPinJob::Start() {
  std::vector<std::string> cids;
  for (const auto& pin_data : pins_data_) {
    cids.push_back(pin_data.cid);
  }

  ipfs_service_->GetPins(cids, "all", true,
                         base::BindOnce(&VerifyLocalPinJob::OnGetPinsResult,
                                        weak_ptr_factory_.GetWeakPtr()));
}

void VerifyLocalPinJob::OnGetPinsResult(std::optional<GetPinsResult> result) {
  if (is_canceled_) {
    std::move(callback_).Run(std::nullopt);
    return;
  }

  if (!result) {
    std::move(callback_).Run(false);
    return;
  }

  // TODO(cypt4): Check exact pinning modes for each cid.
  std::move(callback_).Run(result->size() == pins_data_.size());
}

GcJob::GcJob(PrefService* prefs_service,
             IpfsService* ipfs_service,
             GcCallback callback)
    : prefs_service_(prefs_service),
      ipfs_service_(ipfs_service),
      callback_(std::move(callback)) {}

GcJob::~GcJob() = default;

void GcJob::Start() {
  auto callback = base::BarrierCallback<std::optional<GetPinsResult>>(
      2,
      base::BindOnce(&GcJob::OnGetPinsResult, weak_ptr_factory_.GetWeakPtr()));
  ipfs_service_->GetPins(
      std::nullopt, GetPrefNameFromPinningMode(PinningMode::RECURSIVE), true,
      base::BindOnce(&GcJob::Accumulate, weak_ptr_factory_.GetWeakPtr(),
                     callback));
  ipfs_service_->GetPins(
      std::nullopt, GetPrefNameFromPinningMode(PinningMode::DIRECT), true,
      base::BindOnce(&GcJob::Accumulate, weak_ptr_factory_.GetWeakPtr(),
                     callback));
}

void GcJob::Accumulate(
    base::OnceCallback<void(std::optional<GetPinsResult>)> callback,
    std::optional<GetPinsResult> result) {
  if (!result) {
    gc_job_failed_ = true;
  }

  std::move(callback).Run(std::move(result));
}

void GcJob::OnGetPinsResult(std::vector<std::optional<GetPinsResult>> result) {
  if (is_canceled_) {
    std::move(callback_).Run(false);
    return;
  }

  if (gc_job_failed_) {
    std::move(callback_).Run(false);
    return;
  }

  std::vector<std::string> cids_to_delete;
  const base::Value::Dict& pinning_modes_dict =
      prefs_service_->GetDict(kIPFSPinnedCids);
  // Check both recursive and direct mode dictionaries. If there is no CID in
  // both, then unpin.
  for (const auto& it : result) {
    for (const auto& cid : it.value()) {
      if (!pinning_modes_dict.FindListByDottedPath(
              base::StrCat({GetPrefNameFromPinningMode(PinningMode::RECURSIVE),
                            ".", cid.first})) &&
          !pinning_modes_dict.FindListByDottedPath(
              base::StrCat({GetPrefNameFromPinningMode(PinningMode::DIRECT),
                            ".", cid.first}))) {
        cids_to_delete.push_back(cid.first);
      }
    }
  }

  if (!cids_to_delete.empty()) {
    ipfs_service_->RemovePin(cids_to_delete,
                             base::BindOnce(&GcJob::OnPinsRemovedResult,
                                            weak_ptr_factory_.GetWeakPtr()));
  } else {
    std::move(callback_).Run(true);
  }
}

void GcJob::OnPinsRemovedResult(std::optional<RemovePinResult> result) {
  std::move(callback_).Run(result.has_value());
}

IpfsLocalPinService::IpfsLocalPinService(PrefService* prefs_service,
                                         IpfsService* ipfs_service)
    : prefs_service_(prefs_service), ipfs_service_(ipfs_service) {
  ipfs_base_pin_service_ = std::make_unique<IpfsBasePinService>(ipfs_service_);
}

void IpfsLocalPinService::Reset(base::OnceCallback<void(bool)> callback) {
  weak_ptr_factory_.InvalidateWeakPtrs();
  ipfs_base_pin_service_ = std::make_unique<IpfsBasePinService>(ipfs_service_);
  gc_task_posted_ = false;
  ipfs_service_->LsPinCli(base::BindOnce(&IpfsLocalPinService::OnLsPinCliResult,
                                         weak_ptr_factory_.GetWeakPtr(),
                                         std::move(callback)));
}

void IpfsLocalPinService::OnLsPinCliResult(
    base::OnceCallback<void(bool)> callback,
    std::optional<std::string> result) {
  if (!result) {
    std::move(callback).Run(false);
    return;
  }
  std::vector<std::string> values = base::SplitString(
      result.value(), "\n\r", base::TRIM_WHITESPACE, base::SPLIT_WANT_NONEMPTY);
  if (values.empty()) {
    OnRemovePinCliResult(std::move(callback), true);
    return;
  }
  ipfs_service_->RemovePinCli(
      std::set(values.begin(), values.end()),
      base::BindOnce(&IpfsLocalPinService::OnRemovePinCliResult,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
}

void IpfsLocalPinService::OnRemovePinCliResult(
    base::OnceCallback<void(bool)> callback,
    bool result) {
  if (!result) {
    std::move(callback).Run(false);
    return;
  }
  prefs_service_->ClearPref(kIPFSPinnedCids);
  std::move(callback).Run(true);
}

void IpfsLocalPinService::ScheduleGcTask() {
  base::SequencedTaskRunner::GetCurrentDefault()->PostDelayedTask(
      FROM_HERE,
      base::BindOnce(&IpfsLocalPinService::AddGcTask,
                     weak_ptr_factory_.GetWeakPtr()),
      base::Minutes(1));
}

IpfsLocalPinService::IpfsLocalPinService() = default;

void IpfsLocalPinService::SetIpfsBasePinServiceForTesting(
    std::unique_ptr<IpfsBasePinService> service) {
  ipfs_base_pin_service_ = std::move(service);
}

IpfsLocalPinService::~IpfsLocalPinService() = default;

void IpfsLocalPinService::AddPins(const std::string& key,
                                  const std::vector<std::string>& ipfs_urls,
                                  AddPinCallback callback) {
  auto pins_data = ExtractMergedPinData(ipfs_urls);
  if (!pins_data) {
    NOTREACHED_IN_MIGRATION();
    std::move(callback).Run(false);
    return;
  }
  ipfs_base_pin_service_->AddJob(std::make_unique<AddLocalPinJob>(
      prefs_service_, ipfs_service_, key, pins_data.value(),
      base::BindOnce(&IpfsLocalPinService::OnAddJobFinished,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback))));
}

void IpfsLocalPinService::RemovePins(const std::string& key,
                                     RemovePinCallback callback) {
  ipfs_base_pin_service_->AddJob(std::make_unique<RemoveLocalPinJob>(
      prefs_service_, key,
      base::BindOnce(&IpfsLocalPinService::OnRemovePinsFinished,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback))));
}

void IpfsLocalPinService::ValidatePins(
    const std::string& key,
    const std::vector<std::string>& ipfs_urls,
    ValidatePinsCallback callback) {
  auto pins_data = ExtractMergedPinData(ipfs_urls);
  if (!pins_data) {
    NOTREACHED_IN_MIGRATION();
    std::move(callback).Run(std::nullopt);
    return;
  }
  ipfs_base_pin_service_->AddJob(std::make_unique<VerifyLocalPinJob>(
      prefs_service_, ipfs_service_, key, pins_data.value(),
      base::BindOnce(&IpfsLocalPinService::OnValidateJobFinished,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback))));
}

void IpfsLocalPinService::OnRemovePinsFinished(RemovePinCallback callback,
                                               bool status) {
  std::move(callback).Run(status);
  if (status) {
    base::SequencedTaskRunner::GetCurrentDefault()->PostDelayedTask(
        FROM_HERE,
        base::BindOnce(&IpfsLocalPinService::AddGcTask,
                       weak_ptr_factory_.GetWeakPtr()),
        base::Minutes(1));
  }
  ipfs_base_pin_service_->OnJobDone(status);
}

void IpfsLocalPinService::OnAddJobFinished(AddPinCallback callback,
                                           bool status) {
  std::move(callback).Run(status);
  ipfs_base_pin_service_->OnJobDone(status);
}

void IpfsLocalPinService::OnValidateJobFinished(ValidatePinsCallback callback,
                                                std::optional<bool> status) {
  std::move(callback).Run(status);
  ipfs_base_pin_service_->OnJobDone(status.value_or(false));
}

void IpfsLocalPinService::AddGcTask() {
  if (gc_task_posted_) {
    return;
  }
  gc_task_posted_ = true;
  ipfs_base_pin_service_->AddJob(std::make_unique<GcJob>(
      prefs_service_, ipfs_service_,
      base::BindOnce(&IpfsLocalPinService::OnGcFinishedCallback,
                     weak_ptr_factory_.GetWeakPtr())));
}

void IpfsLocalPinService::OnGcFinishedCallback(bool status) {
  gc_task_posted_ = false;
  ipfs_base_pin_service_->OnJobDone(status);
}

bool IpfsLocalPinService::HasJobs() {
  return ipfs_base_pin_service_->HasJobs();
}

}  // namespace ipfs
