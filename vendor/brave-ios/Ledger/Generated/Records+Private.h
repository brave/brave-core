/* WARNING: THIS FILE IS GENERATED. ANY CHANGES TO THIS FILE WILL BE OVERWRITTEN
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#import <Foundation/Foundation.h>
#import "Records.h"

#include "bat/ledger/export.h"
#include "bat/ledger/auto_contribute_props.h"
#include "bat/ledger/balance_report_info.h"
#include "bat/ledger/grant.h"
#include "bat/ledger/ledger.h"
#include "bat/ledger/ledger_callback_handler.h"
#include "bat/ledger/ledger_client.h"
#include "bat/ledger/media_event_info.h"
#include "bat/ledger/pending_contribution.h"
#include "bat/ledger/publisher_info.h"
#include "bat/ledger/reconcile_info.h"
#include "bat/ledger/rewards_internals_info.h"
#include "bat/ledger/transaction_info.h"
#include "bat/ledger/transactions_info.h"
#include "bat/ledger/wallet_properties.h"

@interface BATAutoContributeProps (Private)
- (instancetype)initWithAutoContributeProps:(const ledger::AutoContributeProps&)obj;
@end

@interface BATBalanceReportInfo (Private)
- (instancetype)initWithBalanceReportInfo:(const ledger::BalanceReportInfo&)obj;
@end

@interface BATPublisherBanner (Private)
- (instancetype)initWithPublisherBanner:(const ledger::PublisherBanner&)obj;
@end

@interface BATReconcileInfo (Private)
- (instancetype)initWithReconcileInfo:(const ledger::ReconcileInfo&)obj;
@end

@interface BATRewardsInternalsInfo (Private)
- (instancetype)initWithRewardsInternalsInfo:(const ledger::RewardsInternalsInfo&)obj;
@end

@interface BATTransactionInfo (Private)
- (instancetype)initWithTransactionInfo:(const ledger::TransactionInfo&)obj;
@end

@interface BATTransactionsInfo (Private)
- (instancetype)initWithTransactionsInfo:(const ledger::TransactionsInfo&)obj;
@end

@interface BATMediaEventInfo (Private)
- (instancetype)initWithMediaEventInfo:(const ledger::MediaEventInfo&)obj;
@end
