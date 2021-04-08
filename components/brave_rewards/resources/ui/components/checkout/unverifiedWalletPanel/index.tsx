/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { LocaleContext } from '../localeContext'

import {
  ConnectWallet,
  DefaultEmoteSadIcon,
  LineSeparator,
  UnverifiedWalletMessage,
  UnverifiedWalletTitle,
  StyledImage,
  StyledNoticeLink
} from './style'

import walletIconUrl from './assets/graphic-wallet.svg'

export function UnverifiedWalletPanel () {
  const locale = React.useContext(LocaleContext)

  return (
    <>
      <UnverifiedWalletTitle><DefaultEmoteSadIcon /> {locale.get('unverifiedWalletTitle')}</UnverifiedWalletTitle>
      <UnverifiedWalletMessage>{locale.get('unverifiedWalletMessage')}</UnverifiedWalletMessage>
      <LineSeparator />
      <ConnectWallet>{locale.get('connectWalletPrefix')}<StyledNoticeLink href={'chrome://rewards/#verify'} target={'_blank'}> {locale.get('connectWalletSuffix')}</StyledNoticeLink></ConnectWallet>
      <StyledImage src={walletIconUrl} />
    </>
  )
}
