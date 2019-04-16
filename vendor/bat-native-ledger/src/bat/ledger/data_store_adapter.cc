/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/data_store_adapter.h"

#include "bat/ledger/internal/sql_data_store_adapter.h"

namespace ledger {

DataStoreAdapter* DataStoreAdapter::CreateInstance(Type type) {
  // TODO(bridiver) - add case statement
  return new SqlDataStoreAdapter();
}

}  // namespace ledger
