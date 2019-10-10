/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#import <Foundation/Foundation.h>
#import "Records.h"

#include "bat/ledger/export.h"
#include "bat/ledger/mojom_structs.h"
#include "bat/ledger/ledger.h"
#include "bat/ledger/ledger_callback_handler.h"
#include "bat/ledger/ledger_client.h"
#include "bat/ledger/transaction_info.h"
#include "bat/ledger/transactions_info.h"

@interface BATTransactionInfo (Private)
- (instancetype)initWithTransactionInfo:(const ledger::TransactionInfo&)obj;
@end

@interface BATTransactionsInfo (Private)
- (instancetype)initWithTransactionsInfo:(const ledger::TransactionsInfo&)obj;
@end
