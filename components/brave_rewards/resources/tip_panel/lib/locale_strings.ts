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
  sendWithButtonLabel: 'Send with $1',
  web3ButtonLabel: 'Use Web3 Wallet',
  verifiedTooltipTitle: 'Verified Creator',
  verifiedTooltipText: 'This creator has registered with Brave and can receive contributions from Brave Rewards users.',
  monthlyTooltipText: 'Make this a monthly recurring contribution. You can cancel it at any time.',
  learnMoreLabel: 'Learn more',
  customAmountLabel: 'Custom',
  monthlySetTitle: 'You\'re already sending monthly contributions to this creator',
  monthlySetText: 'You can still send a contribution now, and you can cancel your monthly contributions to this creator by removing them from $1Monthly Contributions$2.',
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
  contributionSentText: 'Thanks for supporting your favorite creators',
  shareButtonLabel: 'Share your support',
  shareText: 'I earned crypto rewards with the Brave Browser, and just contributed $1 BAT of it to $2. Learn how you can support your favorite creators and sites too: https://brave.com/rewards',
  unexpectedErrorTitle: 'An unexpected error has occurred',
  unexpectedErrorText: 'Please reopen the contribution panel to try again.',
  defaultCreatorDescription: 'Welcome! Users can support this content creator by sending them contributions. It’s a way of thanking them for making great content.',
  platformPublisherTitle: '$1 on $2',
  selfCustodyTitle: 'Support your favorite creators',
  selfCustodyHeader: 'Show your love and send a token of your gratitude',
  selfCustodyText: 'Use your Web3 wallet to send a one time contribution to this creator and show them your appreciation',
  selfCustodySendButtonLabel: 'Send with Web3 wallet',
  selfCustodyNoWeb3Label: 'It looks like this creator isn\'t set up to receive contributions via Web3 wallets yet.'
}

export type StringKey = keyof typeof localeStrings

export function useLocaleContext () {
  return React.useContext<Locale<StringKey>>(LocaleContext)
}
