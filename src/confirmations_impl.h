/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#pragma once

#include "bat/confirmations/confirmations.h"
#include "bat/confirmations/ad_info.h"

namespace confirmations {

class ConfirmationsImpl : public Confirmations {
 public:
  explicit ConfirmationsImpl(ConfirmationsClient* confirmations_client);
  ~ConfirmationsImpl() override;

  void Initialize() override;

  void OnCatalogIssuersChanged(const CatalogIssuersInfo& info) override;

  void OnTimer(const uint32_t timer_id) override;

 private:
  bool is_initialized_;

  ConfirmationsClient* confirmations_client_;  // NOT OWNED

  // Not copyable, not assignable
  ConfirmationsImpl(const ConfirmationsImpl&) = delete;
  ConfirmationsImpl& operator=(const ConfirmationsImpl&) = delete;
};

}  // namespace confirmations
