/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_SYNC_SYNCABLE_WRITE_TRANSACTION_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_SYNC_SYNCABLE_WRITE_TRANSACTION_H_

#define BRAVE_SYNC_ATTACHABLE_WRITE_TRANSACTION_H \
public: \
 WriteTransaction(const base::Location& from_here, UserShare* share, \
     syncable::WriteTransaction* syncable_wr_tr); \
private: \
 bool keep_ = false;

#include "../../../../../components/sync/syncable/write_transaction.h"
#undef BRAVE_SYNC_ATTACHABLE_WRITE_TRANSACTION_H

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_SYNC_SYNCABLE_WRITE_TRANSACTION_H_
