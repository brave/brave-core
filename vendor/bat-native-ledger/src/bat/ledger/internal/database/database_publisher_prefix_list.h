/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_DATABASE_DATABASE_PUBLISHER_PREFIX_LIST_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_DATABASE_DATABASE_PUBLISHER_PREFIX_LIST_H_

#include <memory>
#include <string>

#include "bat/ledger/internal/database/database_table.h"
#include "bat/ledger/internal/publisher/prefix_list_reader.h"

namespace ledger {
namespace database {

using SearchPublisherPrefixListCallback = std::function<void(bool)>;

class DatabasePublisherPrefixList : public DatabaseTable {
 public:
  explicit DatabasePublisherPrefixList(LedgerImpl* ledger);
  ~DatabasePublisherPrefixList() override;

  void Reset(std::unique_ptr<publisher::PrefixListReader> reader,
             ledger::LegacyResultCallback callback);

  void Search(
      const std::string& publisher_key,
      SearchPublisherPrefixListCallback callback);

 private:
  void InsertNext(publisher::PrefixIterator begin,
                  ledger::LegacyResultCallback callback);

  std::unique_ptr<publisher::PrefixListReader> reader_;
};

}  // namespace database
}  // namespace ledger

#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_DATABASE_DATABASE_PUBLISHER_PREFIX_LIST_H_
