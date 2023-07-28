/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import Icon from '@brave/leo/react/icon'

import { LocaleContext, formatMessage } from '../../lib/locale_context'
import { ExternalWallet, getExternalWalletProviderName } from '../../lib/external_wallet'
import { ProviderPayoutStatus } from '../../lib/provider_payout_status'
import { Optional } from '../../../shared/lib/optional'

import { TokenAmount } from '../token_amount'
import { ExchangeAmount } from '../exchange_amount'
import { EarningsRange } from '../earnings_range'
import { NewTabLink } from '../new_tab_link'
import { ExternalWalletView } from './external_wallet_view'
import { ExternalWalletAction } from './external_wallet_action'
import { RewardsSummary, RewardsSummaryData } from './rewards_summary'
import { PendingRewardsView } from './pending_rewards_view'
import { WalletInfoIcon } from './icons/wallet_info_icon'
import { ArrowCircleIcon } from '../icons/arrow_circle_icon'
import { LoadingIcon } from '../../../shared/components/icons/loading_icon'
import { CaretIcon } from '../icons/caret_icon'

import * as urls from '../../lib/rewards_urls'

import * as style from './wallet_card.style'

import * as mojom from '../../../shared/lib/mojom'

const monthFormatter = new Intl.DateTimeFormat(undefined, {
  month: 'short'
})

interface Props {
  balance: Optional<number>
  isGrandfatheredUser: boolean
  externalWallet: ExternalWallet | null
  providerPayoutStatus: ProviderPayoutStatus
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
  onViewStatement?: () => void
  onManageAds?: () => void
}

export function WalletCard (props: Props) {
  const { getString } = React.useContext(LocaleContext)
  const { externalWallet } = props

  const walletDisconnected =
    externalWallet && externalWallet.status === mojom.WalletStatus.kLoggedOut

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
        <style.batAmount data-test-id='rewards-balance-text'>
          {
            !props.balance.hasValue()
              ? <style.balanceSpinner>
                  <LoadingIcon />
                  <style.loading>{getString('loading')}</style.loading>
                </style.balanceSpinner>
              : <>
                  <TokenAmount amount={props.balance.value()} />
                  {
                    props.externalWallet?.provider === 'zebpay' &&
                    props.isGrandfatheredUser &&
                    <style.balanceInfo>
                      <Icon name='help-outline' />
                        <div className='tooltip'>
                          <style.balanceTooltip>
                            {getString('walletBalanceInfoText')}
                          </style.balanceTooltip>
                        </div>
                    </style.balanceInfo>
                  }
              </>
          }
        </style.batAmount>
        <style.exchangeAmount>
        {
          props.balance.hasValue() &&
            <>
              ≈&nbsp;
              <ExchangeAmount
                amount={props.balance.value()}
                rate={props.exchangeRate}
              />
            </>
        }
        </style.exchangeAmount>
      </style.rewardsBalance>
    )
  }

  return (
    <style.root className={props.showSummary ? 'show-summary' : ''}>
      <style.statusPanel>
        <style.statusIndicator>
          <ExternalWalletView
            externalWallet={props.externalWallet}
            onExternalWalletAction={props.onExternalWalletAction}
          />
        </style.statusIndicator>
        <style.earnings>
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
              {
                props.externalWallet
                  ? <EarningsRange
                      minimum={props.minEarningsThisMonth}
                      maximum={props.maxEarningsThisMonth}
                      minimumFractionDigits={3}
                    />
                  : <style.hiddenEarnings>
                      <style.hiddenEarningsValue>
                        - -
                      </style.hiddenEarningsValue>
                      <NewTabLink href={urls.rewardsChangesURL}>
                        {getString('rewardsLearnMore')}
                      </NewTabLink>
                    </style.hiddenEarnings>
              }
            </style.earningsAmount>
          </style.earningsDisplay>
        </style.earnings>
      </style.statusPanel>
      {renderBalance()}
      {
        props.showSummary
          ? <style.summaryBox>
              <RewardsSummary
                data={props.summaryData}
                providerPayoutStatus={props.providerPayoutStatus}
                autoContributeEnabled={props.autoContributeEnabled}
                hideAdEarnings={Boolean(props.externalWallet)}
                minEarningsLastMonth={props.minEarningsLastMonth}
                maxEarningsLastMonth={props.maxEarningsLastMonth}
                nextPaymentDate={props.nextPaymentDate}
                exchangeRate={props.exchangeRate}
                exchangeCurrency={props.exchangeCurrency}
              />
              {
                props.onViewStatement &&
                  <style.viewStatement>
                    <button
                      onClick={props.onViewStatement}
                      data-test-id='view-statement-button'
                    >
                      <WalletInfoIcon />{getString('walletViewStatement')}
                    </button>
                  </style.viewStatement>
              }
            </style.summaryBox>
          : <style.pendingBox>
              <PendingRewardsView
                minEarnings={props.minEarningsLastMonth}
                maxEarnings={props.maxEarningsLastMonth}
                nextPaymentDate={props.nextPaymentDate}
                providerPayoutStatus={props.providerPayoutStatus}
              />
            </style.pendingBox>
      }
    </style.root>
  )
}
