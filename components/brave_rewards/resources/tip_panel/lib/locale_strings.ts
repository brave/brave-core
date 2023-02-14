/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { Locale, LocaleContext } from '../../shared/lib/locale_context'

export const localeStrings = {
  balanceLabel: 'Rewards balance',
  monthlyToggleLabel: 'Set as monthly contribution',
  feeNotice: 'Brave collects $1 of the sent amount as a processing fee.',
  termsOfService: 'By proceeding, you agree to the $1Terms of Service$2 and $3Privacy Policy$4.',
  sendFormTitle: 'Support this creator',
  sendButtonLabel: 'Send',
  web3ButtonLabel: 'Use Web3 Wallet',
  verifiedTooltipTitle: 'Verified Creator',
  verifiedTooltipText: 'This Creator has registered with Brave and can receive contributions from Brave Rewards users.',
  monthlyTooltipText: 'Make this a monthly recurring contribution. You can cancel it at any time.',
  learnMoreLabel: 'Learn more',
  customAmountLabel: 'Custom',
  monthlySetTitle: 'You\'re already sending monthly contributions to this creator',
  monthlySetText: 'You can still send a one-time contribution now, and you can cancel your monthly contributions to this Creator by removing them from $1Monthly Contributions$2.',
  providerMismatchTitle: 'Can\'t send your contribution',
  providerMismatchText: 'This creator is unable to receive contributions from $1.',
  providerMismatchWeb3Text: 'You can still send crypto contributions to this creator by using your Web3 wallet.',
  web3OnlyTitle: 'Send Crypto contributions',
  reconnectTitle: 'Logged out of $1',
  reconnectText: 'It looks like you\'re logged out of $1 for Brave Rewards. Log in again to continue.',
  reconnectWeb3Text: 'You can also send crypto contributions to this creator by using your Web3 wallet.',
  reconnectButtonLabel: 'Log in to $1',
  insufficientBalanceTitle: 'You don\'t have enough tokens',
  insufficientBalanceText: 'You can only send up to $1 BAT using your Brave Rewards balance.',
  contributionFailedTitle: 'There was a problem sending your contribution',
  contributionFailedText: 'Please try again.',
  tryAgainButtonLabel: 'Try again',
  contributionSentTitle: 'Contribution sent!',
  contributionSentText: 'Every bit helps supporting your favorite creators',
  shareButtonLabel: 'Share your support',
  notConnectedTitle: 'Want to support your favorite creators?',
  notConnectedText: 'Just connect a custodial wallet account so you can start earning BAT and support creators.',
  connectButtonLabel: 'Connect'
}

export type StringKey = keyof typeof localeStrings

export function useLocaleContext () {
  return React.useContext<Locale<StringKey>>(LocaleContext)
}
