/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/ledger_database.h"

#include "bat/ledger/internal/ledger_database_impl.h"

namespace ledger {

void CreateLedgerDatabaseOnTaskRunner(
    const base::FilePath& file_path,
    mojo::PendingReceiver<mojom::LedgerDatabase> receiver,
    scoped_refptr<base::SequencedTaskRunner> task_runner) {
  LedgerDatabaseImpl::CreateOnTaskRunner(file_path, std::move(receiver),
                                         task_runner);
}

}  // namespace ledger
