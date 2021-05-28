/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import {
  StyledAlertClose,
  StyledAlertWrapper,
  StyledWrapper,
  StyledHeader,
  StyledTitle,
  StyledBalance,
  StyledBalanceTokens,
  StyledBalanceCurrency,
  StyledBalanceConverted
} from './style'

import Alert, { Type as AlertType } from '../alert'

import { getLocale } from 'brave-ui/helpers'

import { CloseCircleOIcon } from 'brave-ui/components/icons'

export interface Props {
  id?: string
  balance: string
  converted?: string
  onlyAnonWallet?: boolean
  alert?: AlertWallet | null
  wallet?: Rewards.ExternalWallet
  onClick: () => void
}

export interface AlertWallet {
  node: React.ReactNode
  type: AlertType
  onAlertClose?: () => void
}

export default class WalletInfoHeader extends React.PureComponent<Props, {}> {
  render () {
    const { id, balance, converted, onlyAnonWallet, alert, onClick } = this.props
    const batFormatString = onlyAnonWallet ? getLocale('batPoints') : getLocale('bat')

    return (
      <StyledWrapper
        id={id}
        onClick={onClick}
      >
      {
        alert && alert.node ?
        <StyledAlertWrapper>
          {
            alert.onAlertClose ?
            <StyledAlertClose onClick={alert.onAlertClose}>
              <CloseCircleOIcon />
            </StyledAlertClose> : null
          }
          <Alert type={alert.type} bg={true}>
            {alert.node}
        </Alert>
        </StyledAlertWrapper> :
        <StyledHeader>
          <StyledTitle>{getLocale('yourWallet')}</StyledTitle>
            <StyledBalance>
              <StyledBalanceTokens>
                {balance} <StyledBalanceCurrency>{batFormatString}</StyledBalanceCurrency>
              </StyledBalanceTokens>
              {
                converted
                ? <StyledBalanceConverted>{converted}</StyledBalanceConverted>
                : null
              }
            </StyledBalance>
        </StyledHeader>
      }
      </StyledWrapper>
    )
  }
}
