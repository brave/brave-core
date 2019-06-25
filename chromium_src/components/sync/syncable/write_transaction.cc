/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "../../../../components/sync/syncable/write_transaction.cc"   // NOLINT

namespace syncer {

WriteTransaction::WriteTransaction(const base::Location& from_here,
    UserShare* share, syncable::WriteTransaction* syncable_wr_tr) :
    BaseTransaction(share), transaction_(syncable_wr_tr) {
    keep_ = true;
}

}  // namespace syncer
