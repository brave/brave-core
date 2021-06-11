/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_INCLUDE_BAT_LEDGER_LEDGER_DATABASE_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_INCLUDE_BAT_LEDGER_LEDGER_DATABASE_H_

#include "base/files/file_path.h"
#include "base/sequenced_task_runner.h"
#include "bat/ledger/export.h"
#include "brave/vendor/bat-native-ledger/include/bat/ledger/public/interfaces/ledger_database.mojom.h"

namespace ledger {

// Creates a "self-owned" instance of |mojom::LedgerDatabase| on the specified
// task runner. The task runner must allow blocking IO calls.
LEDGER_EXPORT void CreateLedgerDatabaseOnTaskRunner(
    const base::FilePath& file_path,
    mojo::PendingReceiver<mojom::LedgerDatabase> receiver,
    scoped_refptr<base::SequencedTaskRunner> task_runner);

}  // namespace ledger

#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_INCLUDE_BAT_LEDGER_LEDGER_DATABASE_H_
