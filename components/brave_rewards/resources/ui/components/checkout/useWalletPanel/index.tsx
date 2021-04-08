/* Copyright 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { LocaleContext, LocaleData, getLocaleWithTag } from '../localeContext'
import { FormSection } from '../formSection'

import {
  Content,
  WalletInfoPanel,
  ActionPanel,
  ActionPanelButton,
  BatAmount,
  BatSymbol,
  BatIcon,
  ExchangeAmount,
  LastUpdated,
  NotEnoughFunds,
  TermsOfSale
} from './style'

interface ActionButtonProps {
  locale: LocaleData,
  onClick: () => void
}

function PayWithWalletButton (props: ActionButtonProps) {
  const handleClick = () => props.onClick()
  return (
    <ActionPanelButton
      text={props.locale.get('payWithBat')}
      size='medium'
      onClick={handleClick}
      type='accent'
      brand='rewards'
    />
  )
}

interface UseWalletPanelProps {
  canAddFunds: boolean
  balance: string
  balanceConverted: string
  lastUpdated: string
  hasSufficientFunds: boolean
  rewardsEnabled: boolean
  walletVerified: boolean
  onShowAddFunds?: () => void
  onPayWithWallet: () => void
}

export function UseWalletPanel (props: UseWalletPanelProps) {
  if (!props.rewardsEnabled) {
    return null
  }

  const locale = React.useContext(LocaleContext)
  const tags = getLocaleWithTag(locale.get('payWithBatTermsOfSale'))

  return (
    <>
      <FormSection title={locale.get('useTokenBalance')}>
        <Content>
          <WalletInfoPanel hasSufficientFunds={props.hasSufficientFunds}>
            <BatIcon />
            <BatAmount>
              {props.balance}
              <BatSymbol>{locale.get('bat')}</BatSymbol>
            </BatAmount>
            <ExchangeAmount>{props.balanceConverted}</ExchangeAmount>
            <LastUpdated>{locale.get('updated')} {props.lastUpdated}</LastUpdated>
          </WalletInfoPanel>
          <ActionPanel>
            {
              !props.walletVerified
              ? <NotEnoughFunds>{locale.get('unverifiedWallet')}</NotEnoughFunds>
                : props.hasSufficientFunds
                  ? <PayWithWalletButton locale={locale} onClick={props.onPayWithWallet} />
                  : <NotEnoughFunds>{locale.get('notEnoughFunds')}</NotEnoughFunds>
            }
          </ActionPanel>
        </Content>
      </FormSection>
      {
        props.hasSufficientFunds &&
          <TermsOfSale>
            <span>
              {tags.beforeTag}
              <a href='javascript:void 0'>{tags.duringTag}</a>
              {tags.afterTag}
            </span>
          </TermsOfSale>
      }
    </>
  )
}
