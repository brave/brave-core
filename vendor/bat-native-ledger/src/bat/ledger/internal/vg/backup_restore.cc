/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <numeric>

#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "bat/ledger/internal/common/time_util.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/internal/vg/backup_restore.h"

namespace ledger {
namespace vg {

BackupRestore::BackupRestore(LedgerImpl* ledger) : ledger_(ledger) {}

BackupRestore::~BackupRestore() = default;

void BackupRestore::StartBackUpVGSpendStatus() {
  timer_.Start(FROM_HERE, base::Seconds(30), this,
               &BackupRestore::BackUpVGSpendStatus);
}

void BackupRestore::BackUpVGSpendStatus() {
  ledger_->database()->BackUpVGSpendStatus(base::BindOnce(
      &BackupRestore::OnBackUpVGSpendStatus, base::Unretained(this)));
}

void BackupRestore::OnBackUpVGSpendStatus(
    type::Result result,
    type::VirtualGrantSpendStatusPtr&& spend_status_ptr) const {
  if (result == type::Result::LEDGER_OK) {
    BLOG(1, "VG spend status: " << ExtractVGSpendStatus(spend_status_ptr));
  } else {
    BLOG(0, "BackupRestore::BackUpVGSpendStatus() failed!");
  }
}

std::string BackupRestore::ExtractVGSpendStatus(
    const type::VirtualGrantSpendStatusPtr& spend_status_ptr) const {
  DCHECK(spend_status_ptr);

  base::Value vg_spend_status{base::Value::Type::LIST};
  for (const auto& token_ptr : spend_status_ptr->tokens) {
    base::Value token{base::Value::Type::DICTIONARY};
    token.SetIntKey("token_id", token_ptr->token_id);
    token.SetIntKey("redeemed_at", token_ptr->redeemed_at);
    token.SetIntKey("redeem_type", static_cast<int>(token_ptr->redeem_type));
    vg_spend_status.Append(std::move(token));
  }

  base::Value root{base::Value::Type::DICTIONARY};
  root.SetKey("vg_spend_status", std::move(vg_spend_status));
  root.SetIntKey("backed_up_at", util::GetCurrentTimeStamp());

  std::string result{};
  base::JSONWriter::Write(root, &result);

  return result;
}

void BackupRestore::BackUpVGBody(type::CredsBatchType trigger_type,
                                 const std::string& trigger_id) const {
  ledger_->database()->BackUpVGBody(
      trigger_type, trigger_id,
      base::BindOnce(&BackupRestore::OnBackUpVGBody, base::Unretained(this)));
}

void BackupRestore::OnBackUpVGBody(type::VirtualGrantBodyPtr&& body_ptr) const {
  if (body_ptr) {
    BLOG(1, "VG body: " << ExtractVGBody(body_ptr));
  } else {
    BLOG(0, "BackupRestore::BackUpVGBodyForTrigger() failed!");
  }
}

std::string BackupRestore::ExtractVGBody(
    const type::VirtualGrantBodyPtr& body_ptr) const {
  DCHECK(body_ptr);

  base::Value vg_body{base::Value::Type::DICTIONARY};
  vg_body.SetStringKey("creds_id", body_ptr->creds_id);
  vg_body.SetIntKey("trigger_type", static_cast<int>(body_ptr->trigger_type));
  vg_body.SetStringKey("creds", body_ptr->creds);
  vg_body.SetStringKey("blinded_creds", body_ptr->blinded_creds);
  vg_body.SetStringKey("signed_creds", body_ptr->signed_creds);
  vg_body.SetStringKey("public_key", body_ptr->public_key);
  vg_body.SetStringKey("batch_proof", body_ptr->batch_proof);
  vg_body.SetIntKey("status", static_cast<int>(body_ptr->status));

  base::Value tokens{base::Value::Type::LIST};
  for (const auto& token_ptr : body_ptr->tokens) {
    base::Value token{base::Value::Type::DICTIONARY};
    token.SetIntKey("token_id", token_ptr->token_id);
    token.SetStringKey("token_value", token_ptr->token_value);
    token.SetDoubleKey("value", token_ptr->value);
    token.SetIntKey("expires_at", token_ptr->expires_at);
    tokens.Append(std::move(token));
  }
  vg_body.SetKey("tokens", std::move(tokens));

  base::Value root{base::Value::Type::DICTIONARY};
  root.SetKey("vg_body", std::move(vg_body));
  root.SetIntKey("backed_up_at", util::GetCurrentTimeStamp());

  std::string result{};
  base::JSONWriter::Write(root, &result);

  return result;
}

void BackupRestore::RestoreVGs(const std::string& vg_bodies,
                               const std::string& vg_spend_statuses,
                               ledger::RestoreVGsCallback callback) const {
  type::VirtualGrants vgs{};
  if (ParseVirtualGrantBodies(vg_bodies, vgs) &&
      ParseVirtualGrantSpendStatuses(vg_spend_statuses, vgs)) {
    ledger_->database()->RestoreVGs(
        std::move(vgs),
        base::BindOnce(&BackupRestore::OnRestoreVGs, base::Unretained(this),
                       std::move(callback)));
  }
}

bool BackupRestore::ParseVirtualGrantBodies(const std::string& json,
                                            type::VirtualGrants& vgs) const {
  base::DictionaryValue* root = nullptr;
  auto value = base::JSONReader::Read(json);
  if (!value || !value->GetAsDictionary(&root)) {
    BLOG(0, "Invalid vg_bodies JSON!");
    return false;
  }
  DCHECK(root);

  auto* vg_bodies = root->FindListKey("vg_bodies");
  if (!vg_bodies) {
    BLOG(0, "Invalid vg_bodies format!");
    return false;
  }

  for (const auto& vg_body : vg_bodies->GetList()) {
    auto* creds_id = vg_body.FindStringKey("creds_id");
    auto trigger_type = vg_body.FindIntKey("trigger_type");
    auto* creds = vg_body.FindStringKey("creds");
    auto* blinded_creds = vg_body.FindStringKey("blinded_creds");
    auto* signed_creds = vg_body.FindStringKey("signed_creds");
    auto* public_key = vg_body.FindStringKey("public_key");
    auto* batch_proof = vg_body.FindStringKey("batch_proof");
    auto status = vg_body.FindIntKey("status");
    auto* tokens = vg_body.FindListKey("tokens");

    if (!creds_id || !trigger_type || !creds || !blinded_creds ||
        !signed_creds || !public_key || !batch_proof || !status || !tokens) {
      BLOG(0, "Invalid vg_bodies format!");
      return false;
    }

    for (const auto& token : tokens->GetList()) {
      auto token_id = token.FindIntKey("token_id");
      auto* token_value = token.FindStringKey("token_value");
      auto value = token.FindDoubleKey("value");
      auto expires_at = token.FindIntKey("expires_at");

      if (!token_id || !token_value || !value || !expires_at) {
        BLOG(0, "Invalid vg_bodies format!");
        return false;
      }

      auto vg = type::VirtualGrant::New();
      vg->creds_id = *creds_id;
      vg->trigger_type = static_cast<type::CredsBatchType>(*trigger_type);
      vg->creds = *creds;
      vg->blinded_creds = *blinded_creds;
      vg->signed_creds = *signed_creds;
      vg->public_key = *public_key;
      vg->batch_proof = *batch_proof;
      vg->status = static_cast<type::CredsBatchStatus>(*status);
      vg->token_id = *token_id;
      vg->token_value = *token_value;
      vg->value = *value;
      vg->expires_at = *expires_at;

      vgs.emplace(*creds_id, std::move(vg));
    }
  }

  return true;
}

bool BackupRestore::ParseVirtualGrantSpendStatuses(
    const std::string& json,
    type::VirtualGrants& vgs) const {
  base::DictionaryValue* root = nullptr;
  auto value = base::JSONReader::Read(json);
  if (!value || !value->GetAsDictionary(&root)) {
    BLOG(0, "Invalid vg_spend_statuses JSON!");
    return false;
  }
  DCHECK(root);

  auto* vg_spend_statuses = root->FindListKey("vg_spend_statuses");
  if (!vg_spend_statuses) {
    BLOG(0, "Invalid vg_spend_statuses format!");
    return false;
  }

  if (vgs.size() != vg_spend_statuses->GetList().size()) {
    BLOG(0,
         "The number of tokens in vg_bodies doesn't match the number of tokens "
         "in vg_spend_statuses!");
    return false;
  }

  std::map<uint64_t, type::VirtualGrants::iterator> token_id_2_vg{};
  for (auto it = vgs.begin(); it != vgs.end(); ++it) {
    token_id_2_vg.emplace(it->second->token_id, it);
  }

  for (const auto& vg_spend_status : vg_spend_statuses->GetList()) {
    auto token_id = vg_spend_status.FindIntKey("token_id");
    auto redeemed_at = vg_spend_status.FindIntKey("redeemed_at");
    auto redeem_type = vg_spend_status.FindIntKey("redeem_type");

    if (!token_id || !redeemed_at || !redeem_type) {
      BLOG(0, "Invalid vg_spend_statuses format!");
      return false;
    }

    const auto it = token_id_2_vg.find(*token_id);
    if (it == token_id_2_vg.end()) {
      BLOG(0,
           "The set of tokens in vg_bodies doesn't match the set of tokens "
           "in vg_spend_statuses!");
      return false;
    }

    auto& vg = *it->second->second;
    vg.redeemed_at = *redeemed_at;
    vg.redeem_type = static_cast<type::RewardsType>(*redeem_type);

    token_id_2_vg.erase(*token_id);
  }

  if (!token_id_2_vg.empty()) {
    BLOG(0,
         "The set of tokens in vg_bodies doesn't match the set of tokens "
         "in vg_spend_statuses!");
    return false;
  }

  return true;
}

void BackupRestore::OnRestoreVGs(ledger::RestoreVGsCallback callback,
                                 type::Result result) const {
  BLOG(1, "BackupRestore::RestoreVGs(): " << result);
  std::move(callback).Run(result);
}

std::string BackupRestore::GetVirtualGrantBodies(
    const type::VirtualGrants& vgs) const {
  base::Value vg_bodies{base::Value::Type::LIST};

  for (auto creds_id_cit = vgs.cbegin(); creds_id_cit != vgs.cend();
       creds_id_cit = vgs.upper_bound(creds_id_cit->first)) {
    const auto& creds_id = creds_id_cit->first;
    auto creds_id_range = vgs.equal_range(creds_id);

    const auto& vg = creds_id_range.first->second;
    base::Value vg_body{base::Value::Type::DICTIONARY};
    vg_body.SetStringKey("creds_id", vg->creds_id);
    vg_body.SetIntKey("trigger_type", static_cast<int>(vg->trigger_type));
    vg_body.SetStringKey("creds", vg->creds);
    vg_body.SetStringKey("blinded_creds", vg->blinded_creds);
    vg_body.SetStringKey("signed_creds", vg->signed_creds);
    vg_body.SetStringKey("public_key", vg->public_key);
    vg_body.SetStringKey("batch_proof", vg->batch_proof);
    vg_body.SetIntKey("status", static_cast<int>(vg->status));

    base::Value tokens{base::Value::Type::LIST};
    do {
      const auto& vg = creds_id_range.first->second;
      base::Value token{base::Value::Type::DICTIONARY};
      token.SetIntKey("token_id", vg->token_id);
      token.SetStringKey("token_value", vg->token_value);
      token.SetDoubleKey("value", vg->value);
      token.SetIntKey("expires_at", vg->expires_at);
      tokens.Append(std::move(token));
    } while (++creds_id_range.first != creds_id_range.second);
    vg_body.SetKey("tokens", std::move(tokens));

    vg_bodies.Append(std::move(vg_body));
  }

  base::Value root{base::Value::Type::DICTIONARY};
  root.SetKey("vg_bodies", std::move(vg_bodies));
  root.SetIntKey("backed_up_at", util::GetCurrentTimeStamp());

  std::string result{};
  base::JSONWriter::Write(root, &result);

  return result;
}

std::string BackupRestore::GetVirtualGrantSpendStatuses(
    const type::VirtualGrants& vgs) const {
  std::vector<type::VirtualGrants::const_iterator> sorted_vgs(vgs.size());
  std::iota(sorted_vgs.begin(), sorted_vgs.end(), vgs.cbegin());
  std::sort(sorted_vgs.begin(), sorted_vgs.end(), [](auto lhs, auto rhs) {
    return lhs->second->token_id < rhs->second->token_id;
  });

  base::Value tokens{base::Value::Type::LIST};
  for (auto vg_cit : sorted_vgs) {
    const auto& vg = vg_cit->second;
    base::Value token{base::Value::Type::DICTIONARY};
    token.SetIntKey("token_id", vg->token_id);
    token.SetIntKey("redeemed_at", vg->redeemed_at);
    token.SetIntKey("redeem_type", static_cast<int>(vg->redeem_type));
    tokens.Append(std::move(token));
  }

  base::Value root{base::Value::Type::DICTIONARY};
  root.SetKey("vg_spend_statuses", std::move(tokens));
  root.SetIntKey("backed_up_at", util::GetCurrentTimeStamp());

  std::string vg_spend_statuses{};
  base::JSONWriter::Write(root, &vg_spend_statuses);

  return vg_spend_statuses;
}

}  // namespace vg
}  // namespace ledger
