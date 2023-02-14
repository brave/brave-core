/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { Optional } from '../../shared/lib/optional'
import { ExternalWalletProvider, getExternalWalletProviderName } from '../../shared/lib/external_wallet'
import { useLocaleContext } from '../lib/locale_strings'
import { batAmountFormatter } from '../lib/formatters'
import { BatIcon } from '../../shared/components/icons/bat_icon'
import { WalletProviderIcon } from '../../shared/components/icons/wallet_provider_icon'

import * as style from './balance_view.style'

function formatUnavailableBalance () {
  return batAmountFormatter.format(10.25).replace(/\d/g, '-')
}

interface Props {
  userBalance: Optional<number>
  walletProvider: ExternalWalletProvider | null
}

export function BalanceView (props: Props) {
  const { getString } = useLocaleContext()
  return (
    <style.root>
      <style.batIcon>
        <BatIcon />
      </style.batIcon>
      <div>
        <style.title>
          <div>{getString('balanceLabel')}</div>
          {
            props.walletProvider &&
              <style.provider>
                <WalletProviderIcon provider={props.walletProvider} />
                {getExternalWalletProviderName(props.walletProvider)}
              </style.provider>
          }
        </style.title>
        <style.amount>
          {
            props.userBalance.hasValue()
             ? batAmountFormatter.format(props.userBalance.value())
             : formatUnavailableBalance()
          } BAT
        </style.amount>
      </div>
    </style.root>
  )
}
