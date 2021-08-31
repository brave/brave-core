// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import BraveCore
import BraveShared
import Shared

extension Ledger.DrainStatus: RepresentableOptionType {
    public var displayString: String {
        switch self {
        case .invalid:
            return Strings.Rewards.legacyWalletTransferStatusShortformInvalid
        case .pending:
            return Strings.Rewards.legacyWalletTransferStatusShortformPending
        case .inProgress:
            return Strings.Rewards.legacyWalletTransferStatusShortformInProgress
        case .delayed:
            return Strings.Rewards.legacyWalletTransferStatusShortformDelayed
        case .complete:
            return Strings.Rewards.legacyWalletTransferStatusShortformCompleted
        @unknown default:
            return ""
        }
    }
    var statusButtonTitle: String {
        switch self {
        case .invalid:
            return Strings.Rewards.legacyWalletTransferStatusButtonInvalidTitle
        case .pending:
            return Strings.Rewards.legacyWalletTransferStatusButtonPendingTitle
        case .inProgress:
            return Strings.Rewards.legacyWalletTransferStatusButtonInProgressTitle
        case .delayed:
            return Strings.Rewards.legacyWalletTransferStatusButtonDelayedTitle
        case .complete:
            return Strings.Rewards.legacyWalletTransferStatusButtonCompletedTitle
        @unknown default:
            return ""
        }
    }
    var transferStatusTitle: String {
        switch self {
        case .invalid:
            return Strings.Rewards.legacyWalletTransferStatusInvalidTitle
        case .pending:
            return Strings.Rewards.legacyWalletTransferStatusPendingTitle
        case .inProgress:
            return Strings.Rewards.legacyWalletTransferStatusInProgressTitle
        case .delayed:
            return Strings.Rewards.legacyWalletTransferStatusDelayedTitle
        case .complete:
            return Strings.Rewards.legacyWalletTransferStatusCompletedTitle
        @unknown default:
            return ""
        }
    }
    var transferStatusBody: String {
        switch self {
        case .invalid:
            return Strings.Rewards.legacyWalletTransferStatusInvalidBody
        case .pending:
            return Strings.Rewards.legacyWalletTransferStatusPendingBody
        case .inProgress:
            return Strings.Rewards.legacyWalletTransferStatusInProgressBody
        case .delayed:
            return Strings.Rewards.legacyWalletTransferStatusDelayedBody
        case .complete:
            return Strings.Rewards.legacyWalletTransferStatusCompletedBody
        @unknown default:
            return ""
        }
    }
}
