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
  monthlySetText: 'You can send more now, and you can cancel your monthly contributions to this Creator by removing them from $1Monthly Contributions$2.',
  providerMismatchTitle: 'Can\'t send your contribution',
  providerMismatchText: 'This creator is unable to receive contributions from $1.',
  web3OnlyTitle: 'Send Crypto contributions',
  web3OnlyText: 'This creator doesn\'t have a Brave Rewards address for contributions.',
  web3GetStartedText: 'You can send contributions using your web3 wallet. Click the button below to start.',
  reconnectTitle: 'Connection with $1 lost',
  reconnectText: 'It seems that we\'ve lost connection with your $1 account. Connect your account again and retry.',
  reconnectButtonLabel: 'Reconnect',
  insufficientBalanceTitle: 'You don\'t have enough tokens',
  insufficientBalanceText: 'You can only send up to $1 BAT using your Brave Rewards balance.',
  contributionFailedTitle: 'There was a problem sending your tip',
  contributionFailedText: 'Please try again',
  tryAgainButtonLabel: 'Try again',
  contributionSentTitle: '$1 BAT contribution sent!',
  contributionSentText: 'Every bit helps supporting your favorite creators',
  tweetButtonLabel: 'Tweet about your support',
  notConnectedTitle: 'Want to support your favorite creators?',
  notConnectedText: 'Just connect a custodial wallet account so you can start earning BAT and support creators.',
  connectButtonLabel: 'Connect'
}

export type StringKey = keyof typeof localeStrings

export function useLocaleContext () {
  return React.useContext<Locale<StringKey>>(LocaleContext)
}
