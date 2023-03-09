/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
// Components
import { WalletInfo } from './walletInfo'
import { WalletHistory } from './walletHistory'
import { Balance } from './balance'
import { ExternalWallet } from './externalWallet'
import { Button } from 'brave-ui/components'
import { BalanceWrapper, ButtonWrapper } from '../style'

// Utils
import { getLocale } from '../../../../common/locale'

interface Props {
  data: RewardsInternals.State
  onGet: () => void
}

export class General extends React.Component<Props, {}> {
  render () {
    return (
      <>
        <ButtonWrapper>
          <Button
            text={getLocale('refreshButton')}
            size={'medium'}
            type={'accent'}
            onClick={this.props.onGet}
          />
        </ButtonWrapper>
        <WalletInfo state={this.props.data} />
        <WalletHistory
          paymentId={this.props.data.info.walletPaymentId}
          logEntries={this.props.data.eventLogs}
        />
        {this.props.data.info.bootStamp > 0 &&
          <BalanceWrapper>
            <Balance
              balance={this.props.data.balance}
              externalWallet={this.props.data.externalWallet} />
          </BalanceWrapper>
        }
        <ExternalWallet info={this.props.data.externalWallet} />
      </>
    )
  }
}
