/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#pragma once

#include "bat/confirmations/confirmations_client.h"
#include "bat/confirmations/export.h"
#include "bat/confirmations/catalog_issuers_info.h"

namespace confirmations {

extern bool _is_debug;
extern bool _is_production;

class CONFIRMATIONS_EXPORT Confirmations {
 public:
  Confirmations() = default;
  virtual ~Confirmations() = default;

  static Confirmations* CreateInstance(
    ConfirmationsClient* confirmations_client);

  // Should be called when Brave Ads are enabled or disabled on the Client
  virtual void Initialize() = 0;

  // Should be called when a new catalog has been downloaded in Brave Ads
  virtual void OnCatalogIssuersChanged(const CatalogIssuersInfo& info) = 0;

 private:
  // Not copyable, not assignable
  Confirmations(const Confirmations&) = delete;
  Confirmations& operator=(const Confirmations&) = delete;
};

}  // namespace confirmations
