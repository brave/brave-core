/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import Icon from '@brave/leo/react/icon'

import { LocaleContext, formatMessage } from '../../lib/locale_context'

import {
  ExternalWallet,
  getExternalWalletProviderName,
  isSelfCustodyProvider
} from '../../lib/external_wallet'

import { ProviderPayoutStatus } from '../../lib/provider_payout_status'
import { UserType } from '../../lib/user_type'
import { Optional } from '../../../shared/lib/optional'

import { useCounterAnimation } from './counter_animation'
import { TokenAmount } from '../token_amount'
import { ExchangeAmount } from '../exchange_amount'
import { ExternalWalletView } from './external_wallet_view'
import { ExternalWalletAction } from './external_wallet_action'
import { RewardsSummary, RewardsSummaryData } from './rewards_summary'
import { PendingRewardsView } from './pending_rewards_view'
import { ArrowCircleIcon } from '../icons/arrow_circle_icon'
import { LoadingIcon } from '../../../shared/components/icons/loading_icon'
import { CaretIcon } from '../icons/caret_icon'

import * as style from './wallet_card.style'

const monthFormatter = new Intl.DateTimeFormat(undefined, {
  month: 'short'
})

function getIntegerDigits (num: number) {
  return num <= 1 ? 1 : Math.floor(Math.log10(num)) + 1
}

interface Props {
  userType: UserType
  balance: Optional<number>
  externalWallet: ExternalWallet | null
  providerPayoutStatus: ProviderPayoutStatus
  adsReceivedThisMonth?: number
  minEarningsThisMonth: number
  maxEarningsThisMonth: number
  minEarningsLastMonth: number
  maxEarningsLastMonth: number
  nextPaymentDate: number
  exchangeRate: number
  exchangeCurrency: string
  showSummary: boolean
  summaryData: RewardsSummaryData
  autoContributeEnabled: boolean
  onExternalWalletAction: (action: ExternalWalletAction) => void
  onManageAds?: () => void
}

export function WalletCard (props: Props) {
  const { getString } = React.useContext(LocaleContext)
  const balanceCounterValue =
    useCounterAnimation(props.balance.valueOr(0), 450)
  const { externalWallet } = props

  const walletDisconnected = externalWallet && !externalWallet.authenticated

  // The contribution summary is not currently shown for self-custody users.
  const showSummary = props.showSummary &&
    (!externalWallet || !isSelfCustodyProvider(externalWallet.provider))

  function renderBalance () {
    if (externalWallet && walletDisconnected) {
      const onReconnectClick = () => {
        props.onExternalWalletAction('reconnect')
      }

      return (
        <style.disconnectedBalance onClick={onReconnectClick}>
          {
            formatMessage(getString('rewardsLogInToSeeBalance'), {
              placeholders: {
                $2: getExternalWalletProviderName(externalWallet.provider)
              },
              tags: {
                $1: (content) => <strong key='1'>{content}</strong>
              }
            })
          }
          <ArrowCircleIcon />
        </style.disconnectedBalance>
      )
    }

    return (
      <style.rewardsBalance>
        <style.balanceHeader>
          {getString('walletBalanceTitle')}
        </style.balanceHeader>
        <style.batAmountForTesting data-test-id='rewards-balance-text'>
          <TokenAmount amount={props.balance.valueOr(0)} />
        </style.batAmountForTesting>
        <style.batAmount>
          {
            !props.balance.hasValue()
              ? <style.balanceSpinner>
                  <LoadingIcon /> {getString('loading')}
                </style.balanceSpinner>
              : <TokenAmount
                  minimumIntegerDigits={getIntegerDigits(props.balance.value())}
                  amount={balanceCounterValue}
                />
          }
        </style.batAmount>
        <style.exchangeAmount>
        {
          props.balance.hasValue() &&
            <>
              ≈&nbsp;
              <ExchangeAmount
                amount={balanceCounterValue}
                rate={props.exchangeRate}
              />
            </>
        }
        </style.exchangeAmount>
      </style.rewardsBalance>
    )
  }

  return (
    <style.root className={showSummary ? 'show-summary' : ''}>
      <style.statusPanel>
        <style.statusIndicator>
          <ExternalWalletView
            externalWallet={props.externalWallet}
            onExternalWalletAction={props.onExternalWalletAction}
          />
        </style.statusIndicator>
        <style.earnings className={props.adsReceivedThisMonth === undefined ? 'hidden' : ''}>
          <style.earningsHeader>
            <style.earningsHeaderTitle>
              {getString('walletEstimatedEarnings')}
            </style.earningsHeaderTitle>
            <style.earningsInfo>
              <Icon name='info-outline' />
              <div className='tooltip'>
                <style.earningsTooltip>
                  <div>{getString('walletEarningInfoText')}</div>
                  {
                    props.onManageAds &&
                      <style.manageAds>
                        <button onClick={props.onManageAds}>
                          <span>{getString('walletManageAds')}</span>
                          <CaretIcon direction='right' />
                        </button>
                      </style.manageAds>
                  }
                </style.earningsTooltip>
              </div>
            </style.earningsInfo>
          </style.earningsHeader>
          <style.earningsDisplay>
            <style.earningsMonth>
              {monthFormatter.format(new Date())}
            </style.earningsMonth>
            <style.earningsAmount>
              {props.adsReceivedThisMonth}
            </style.earningsAmount>
          </style.earningsDisplay>
        </style.earnings>
      </style.statusPanel>
      {renderBalance()}
      {
        showSummary
          ? <style.summaryBox>
              <RewardsSummary
                data={props.summaryData}
                userType={props.userType}
                providerPayoutStatus={props.providerPayoutStatus}
                autoContributeEnabled={props.autoContributeEnabled}
                hideAdEarnings={Boolean(props.externalWallet)}
                minEarningsLastMonth={props.minEarningsLastMonth}
                maxEarningsLastMonth={props.maxEarningsLastMonth}
                nextPaymentDate={props.nextPaymentDate}
                exchangeRate={props.exchangeRate}
                exchangeCurrency={props.exchangeCurrency}
              />
            </style.summaryBox>
          : <style.pendingBox>
              {
                props.userType === 'connected' &&
                  <PendingRewardsView
                    minEarnings={props.minEarningsLastMonth}
                    maxEarnings={props.maxEarningsLastMonth}
                    nextPaymentDate={props.nextPaymentDate}
                    providerPayoutStatus={props.providerPayoutStatus}
                  />
              }
            </style.pendingBox>
      }
    </style.root>
  )
}
