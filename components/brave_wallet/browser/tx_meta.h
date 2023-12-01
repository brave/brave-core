/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_TX_META_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_TX_META_H_

#include <optional>
#include <string>

#include "base/time/time.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "url/origin.h"

namespace base {
class Value;
}  // namespace base

namespace brave_wallet {

class TxMeta {
 public:
  TxMeta();
  virtual ~TxMeta();

  static std::string GenerateMetaID();

  virtual base::Value::Dict ToValue() const;
  virtual mojom::TransactionInfoPtr ToTransactionInfo() const = 0;

  const std::string& id() const { return id_; }
  mojom::TransactionStatus status() const { return status_; }
  const mojom::AccountIdPtr& from() const { return from_; }
  base::Time created_time() const { return created_time_; }
  base::Time submitted_time() const { return submitted_time_; }
  base::Time confirmed_time() const { return confirmed_time_; }
  const std::string& tx_hash() const { return tx_hash_; }
  const std::optional<url::Origin>& origin() const { return origin_; }
  const std::string& chain_id() const { return chain_id_; }

  void set_id(const std::string& id) { id_ = id; }
  void set_status(mojom::TransactionStatus status) { status_ = status; }
  void set_from(const mojom::AccountIdPtr& from) { from_ = from.Clone(); }
  void set_created_time(base::Time created_time) {
    created_time_ = created_time;
  }
  void set_submitted_time(base::Time submitted_time) {
    submitted_time_ = submitted_time;
  }
  void set_confirmed_time(base::Time confirmed_time) {
    confirmed_time_ = confirmed_time;
  }
  void set_tx_hash(const std::string& tx_hash) { tx_hash_ = tx_hash; }
  void set_origin(const std::optional<url::Origin>& origin) {
    origin_ = origin;
  }
  void set_chain_id(const std::string& chain_id) { chain_id_ = chain_id; }

 protected:
  bool operator==(const TxMeta&) const;

  std::string id_;
  mojom::TransactionStatus status_ = mojom::TransactionStatus::Unapproved;
  mojom::AccountIdPtr from_;
  base::Time created_time_;
  base::Time submitted_time_;
  base::Time confirmed_time_;
  std::string tx_hash_;
  std::optional<url::Origin> origin_;
  std::string chain_id_;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_TX_META_H_
