/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { localeStrings as walletCardStrings } from '../../shared/components/wallet_card/stories/locale_strings'

export const localeStrings = {
  ...walletCardStrings,
  summary: 'Summary',
  tip: 'Tip',
  unverifiedCreator: 'Unverified Creator',
  verifiedCreator: 'Verified Creator',
  refreshStatus: 'Refresh Status',
  attention: 'Attention',
  sendTip: 'Send Tip',
  note: 'Note',
  unverifiedNote: 'This creator has not yet signed up to receive contributions from Brave users. Any tips you send will remain in your wallet until they verify.',
  providerNotSupportedNote: 'This creator is currently not configured to receive tips from your wallet. Any tips you make will remain pending in your wallet and retry automatically for 90 days.',
  unverifiedLinks: '$1Learn more$2 or $3Don’t show again$4.',
  includeInAutoContribute: 'Include in Auto-Contribute',
  monthlyContribution: 'Monthly Contribution',
  set: 'Set',
  changeAmount: 'Change amount',
  cancel: 'Cancel'
}
