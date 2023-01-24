/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { LocaleContext, formatMessage } from '../../lib/locale_context'
import { ExternalWallet, getExternalWalletProviderName } from '../../lib/external_wallet'
import { ProviderPayoutStatus } from '../../lib/provider_payout_status'

import { TokenAmount } from '../token_amount'
import { ExchangeAmount } from '../exchange_amount'
import { NewTabLink } from '../new_tab_link'
import { ExternalWalletView } from './external_wallet_view'
import { ExternalWalletAction } from './external_wallet_action'
import { RewardsSummary, RewardsSummaryData } from './rewards_summary'
import { PendingRewardsView } from './pending_rewards_view'
import { WalletInfoIcon } from './icons/wallet_info_icon'
import { ArrowCircleIcon } from '../icons/arrow_circle_icon'

import * as urls from '../../lib/rewards_urls'

import * as style from './wallet_card.style'

import * as mojom from '../../../shared/lib/mojom'

const rangeFormatter = new Intl.DateTimeFormat(undefined, {
  month: 'short',
  day: 'numeric'
})

interface Props {
  balance: number
  externalWallet: ExternalWallet | null
  providerPayoutStatus: ProviderPayoutStatus
  earningsThisMonth: number
  earningsLastMonth: number
  nextPaymentDate: number
  exchangeRate: number
  exchangeCurrency: string
  showSummary: boolean
  summaryData: RewardsSummaryData
  autoContributeEnabled: boolean
  onExternalWalletAction: (action: ExternalWalletAction) => void
  onViewPendingTips?: () => void
  onViewStatement?: () => void
}

export function getCurrentMonthRange () {
  const now = new Date(Date.now())
  const start = new Date(now.getFullYear(), now.getMonth(), 1)
  const end = new Date(now.getFullYear(), now.getMonth() + 1, 0)
  return rangeFormatter.format(start) + ' – ' + rangeFormatter.format(end)
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

      const coverActions = props.showSummary && props.onViewStatement

      return (
        <style.disconnectedBalance
          className={coverActions ? 'cover-actions' : ''}
          onClick={onReconnectClick}
        >
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
          {getString('walletYourBalance')}
        </style.balanceHeader>
        <style.batAmount data-test-id='rewards-balance-text'>
          <TokenAmount amount={props.balance} />
        </style.batAmount>
        <style.exchangeAmount>
          <ExchangeAmount amount={props.balance} rate={props.exchangeRate} />
        </style.exchangeAmount>
      </style.rewardsBalance>
    )
  }

  function renderEstimatedEarnings () {
    if (!props.externalWallet) {
      return (
        <style.hiddenEarnings>
          -&nbsp;-
          <NewTabLink href={urls.rewardsChangesURL}>
            {getString('rewardsLearnMore')}
          </NewTabLink>
        </style.hiddenEarnings>
      )
    }

    return (
      <>
        <style.batAmount>
          <TokenAmount amount={props.earningsThisMonth} />
        </style.batAmount>
        <style.exchangeAmount>
          ≈ &nbsp;
          <ExchangeAmount
            amount={props.earningsThisMonth}
            rate={props.exchangeRate}
            currency={props.exchangeCurrency}
          />
        </style.exchangeAmount>
      </>
    )
  }

  return (
    <style.root className={props.showSummary ? 'show-summary' : ''}>
      <style.grid>
        <style.statusIndicator>
          <ExternalWalletView
            externalWallet={props.externalWallet}
            onExternalWalletAction={props.onExternalWalletAction}
          />
        </style.statusIndicator>
        {renderBalance()}
        <style.earningsPanel>
          <style.dateRange>
            {getCurrentMonthRange()}
          </style.dateRange>
          <style.earningsHeader>
            {getString('walletEstimatedEarnings')}
          </style.earningsHeader>
          {renderEstimatedEarnings()}
        </style.earningsPanel>
        {
          props.showSummary && props.onViewStatement &&
            <style.viewStatement>
              <button
                onClick={props.onViewStatement}
                data-test-id='view-statement-button'
              >
                <WalletInfoIcon />{getString('walletViewStatement')}
              </button>
            </style.viewStatement>
        }
      </style.grid>
      {
        props.showSummary
          ? <style.summaryBox>
              <RewardsSummary
                data={props.summaryData}
                providerPayoutStatus={props.providerPayoutStatus}
                autoContributeEnabled={props.autoContributeEnabled}
                hideAdEarnings={Boolean(props.externalWallet)}
                earningsLastMonth={props.earningsLastMonth}
                nextPaymentDate={props.nextPaymentDate}
                exchangeRate={props.exchangeRate}
                exchangeCurrency={props.exchangeCurrency}
                onViewPendingTips={props.onViewPendingTips}
              />
            </style.summaryBox>
          : <style.pendingBox>
              <PendingRewardsView
                earningsLastMonth={props.earningsLastMonth}
                nextPaymentDate={props.nextPaymentDate}
                providerPayoutStatus={props.providerPayoutStatus}
              />
            </style.pendingBox>
      }
    </style.root>
  )
}
