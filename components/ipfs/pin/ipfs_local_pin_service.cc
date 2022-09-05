/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/ipfs/pin/ipfs_local_pin_service.h"

#include "brave/components/ipfs/pin/ipfs_pin_service_add_job.h"
#include "brave/components/ipfs/pref_names.h"
#include "components/prefs/scoped_user_pref_update.h"

namespace ipfs {

AddLocalPinJob::AddLocalPinJob(PrefService* prefs_service,
                               IpfsService* ipfs_service,
                               const std::string& prefix,
                               const std::vector<std::string>& cids,
                               JobFinishedCallback callback)
    : IpfsCIDListJob(cids, std::move(callback)),
      prefs_service_(prefs_service),
      ipfs_service_(ipfs_service),
      prefix_(prefix) {}

AddLocalPinJob::~AddLocalPinJob() {}

void AddLocalPinJob::DoWork(const std::string& cid) {
  LOG(ERROR) << "XXXZZZ AddLocalPinJob pin " << cid;
  std::vector<std::string> cids;
  cids.push_back(cid);
  ipfs_service_->AddPin(
      cids, true,
      base::BindOnce(&AddLocalPinJob::OnAddPinResult, base::Unretained(this)));
}

void AddLocalPinJob::OnAddPinResult(bool status,
                                    absl::optional<AddPinResult> result) {
  LOG(ERROR) << "XXXZZZ on add pin result " << status;
  if (status && result) {
    DictionaryPrefUpdate update(prefs_service_, kIPFSPinnedCids);
    base::Value::Dict& update_dict = update->GetDict();

    for (const auto& cid : result->pins) {
      base::Value::List* list = update_dict.FindList(cid);
      if (!list) {
        update_dict.Set(cid, base::Value::List());
        list = update_dict.FindList(cid);
      }
      bool should_add = true;
      for (const auto& value : *list) {
        if (value.GetString() == cid) {
          should_add = false;
          break;
        }
      }
      if (should_add) {
        list->Append(prefix_);
      }
    }
    Continue();
  } else {
    Fail();
  }
}

RemoveLocalPinJob::RemoveLocalPinJob(PrefService* prefs_service,
                                     IpfsService* ipfs_service,
                                     const std::string& prefix,
                                     const std::vector<std::string>& cids,
                                     JobFinishedCallback callback)
    : IpfsCIDListJob(cids, std::move(callback)),
      prefs_service_(prefs_service),
      ipfs_service_(ipfs_service),
      prefix_(prefix) {}

RemoveLocalPinJob::~RemoveLocalPinJob() {}

void RemoveLocalPinJob::DoWork(const std::string& cid) {
  std::vector<std::string> cids;
  cids.push_back(cid);
  ipfs_service_->RemovePin(cids,
                           base::BindOnce(&RemoveLocalPinJob::OnRemovePinResult,
                                          base::Unretained(this)));
}

void RemoveLocalPinJob::OnRemovePinResult(
    bool status,
    absl::optional<RemovePinResult> result) {
  LOG(ERROR) << "XXXZZZ on add pin result " << status;
  if (status && result) {
    DictionaryPrefUpdate update(prefs_service_, kIPFSPinnedCids);
    base::Value::Dict& update_dict = update->GetDict();

    for (const auto& cid : result.value()) {
      base::Value::List* list = update_dict.FindList(cid);
      if (!list) {
        update_dict.Set(cid, base::Value::List());
        list = update_dict.FindList(cid);
      }
      bool should_add = true;
      for (const auto& value : *list) {
        if (value.GetString() == cid) {
          should_add = false;
          break;
        }
      }
      if (should_add) {
        list->EraseValue(base::Value(prefix_));
      }
    }
    Continue();
  } else {
    Fail();
  }
}

VerifyLocalPinJob::VerifyLocalPinJob(PrefService* prefs_service,
                                     IpfsService* ipfs_service,
                                     const std::string& prefix,
                                     const std::vector<std::string>& cids,
                                     JobFinishedCallback callback) : IpfsCIDListJob(cids, std::move(callback)),
    prefs_service_(prefs_service),
    ipfs_service_(ipfs_service),
    prefix_(prefix) {
}

VerifyLocalPinJob::~VerifyLocalPinJob() {
}

void VerifyLocalPinJob::DoWork(const std::string& cid) {
  std::vector<std::string> cids;
  cids.push_back(cid);
  ipfs_service_->GetPins(cids,
                         "all",
                         true,
                         base::BindOnce(&VerifyLocalPinJob::OnGetPinsResult,
                                        base::Unretained(this)));
}

void VerifyLocalPinJob::OnGetPinsResult(bool status, absl::optional<GetPinsResult> result) {
    if (status && result) {
      DictionaryPrefUpdate update(prefs_service_, kIPFSPinnedCids);
//      base::Value::Dict& update_dict = update->GetDict();

//      for (const auto& cid : result.value()) {
//        base::Value::List* list = update_dict.FindList(cid);
//        if (!list) {
//          update_dict.Set(cid, base::Value::List());
//          list = update_dict.FindList(cid);
//        }
//        bool should_add = true;
//        for (const auto& value : *list) {
//          if (value.GetString() == cid) {
//            should_add = false;
//            break;
//          }
//        }
//        if (should_add) {
//          list->EraseValue(base::Value(prefix_));
//        }
//      }
      Continue();
    } else {
      Fail();
    }
}

IpfsLocalPinService::IpfsLocalPinService(PrefService* prefs_service,
                                         IpfsService* ipfs_service)
    : prefs_service_(prefs_service), ipfs_service_(ipfs_service) {
  ipfs_base_pin_service_ = std::make_unique<IpfsBasePinService>(ipfs_service_);
}

IpfsLocalPinService::~IpfsLocalPinService() {}

void IpfsLocalPinService::AddPins(const std::string& prefix,
                                  const std::vector<std::string>& cids,
                                  AddPinCallback callback) {
  LOG(ERROR) << "XXXZZZ add pins " << prefix;
  ipfs_base_pin_service_->AddJob(std::make_unique<AddLocalPinJob>(
      prefs_service_, ipfs_service_, prefix, cids,
      base::BindOnce(&IpfsLocalPinService::OnAddJobFinished,
                     base::Unretained(this), std::move(callback))));
}

void IpfsLocalPinService::RemovePins(const std::string& prefix,
                                     const std::vector<std::string>& cids,
                                     RemovePinCallback callback) {
  ipfs_base_pin_service_->AddJob(std::make_unique<RemoveLocalPinJob>(
      prefs_service_, ipfs_service_, prefix, cids,
      base::BindOnce(&IpfsLocalPinService::OnAddJobFinished,
                     base::Unretained(this), std::move(callback))));
}

void IpfsLocalPinService::ValidatePins(const std::string& prefix,
                                       const std::vector<std::string>& cids,
                                       ValidatePinsCallback callback) {
    ipfs_base_pin_service_->AddJob(std::make_unique<VerifyLocalPinJob>(
        prefs_service_, ipfs_service_, prefix, cids,
        base::BindOnce(&IpfsLocalPinService::OnAddJobFinished,
                       base::Unretained(this), std::move(callback))));
}

void IpfsLocalPinService::OnGetPinsResult(
    const std::string& prefix,
    const std::vector<std::string>& cids,
    ValidatePinsCallback callback,
    bool status,
    absl::optional<GetPinsResult> result) {
  if (!result) {
    std::move(callback).Run(false);
    return;
  }
  for (const auto& cid : cids) {
    if (result.value().find(cid) == result.value().end()) {
      std::move(callback).Run(false);
      return;
    }
  }
  std::move(callback).Run(true);
}

void IpfsLocalPinService::OnRemovePinsFinished(RemovePinCallback callback,
                                               bool status) {
  LOG(ERROR) << "XXXZZZ OnRemovePinsFinished " << status;

  ipfs_base_pin_service_->OnJobDone(status);
  std::move(callback).Run(status);
}

void IpfsLocalPinService::OnAddJobFinished(ValidatePinsCallback callback,
                                           bool status) {
  LOG(ERROR) << "XXXZZZ OnAddJobFinished " << status;
  ipfs_base_pin_service_->OnJobDone(status);
  std::move(callback).Run(status);
}

}  // namespace ipfs
