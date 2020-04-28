/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { WalletInfo, OrderInfo, ExchangeRateInfo, CheckoutHost } from '../interfaces'
import { DialogFrame } from '../../ui/components/checkout/dialogFrame'
import { PaymentMethodPanel } from '../../ui/components/checkout/paymentMethodPanel'
import { AddFundsPanel } from '../../ui/components/checkout/addFundsPanel'
import { PaymentProcessing } from '../../ui/components/checkout/paymentProcessing'
import { PaymentComplete } from '../../ui/components/checkout/paymentComplete'

import {
  createExchangeFormatter,
  formatLastUpdatedDate,
  formatTokenValue
} from '../formatting'

type FlowState =
  'start' |
  'add-funds' |
  'payment-processing' |
  'payment-complete'

interface AppProps {
  host: CheckoutHost
  exchangeCurrency: string
}

export function App (props: AppProps) {
  const [flowState, setFlowState] = React.useState<FlowState>('start')
  const [rateInfo, setRateInfo] = React.useState<ExchangeRateInfo | null>(null)
  const [walletInfo, setWalletInfo] = React.useState<WalletInfo | null>(null)
  const [orderInfo, setOrderInfo] = React.useState<OrderInfo | null>(null)
  const [rewardsEnabled, setRewardsEnabled] = React.useState(false)

  const showTitle =
    flowState !== 'payment-complete'

  const showBackground =
    flowState !== 'payment-complete' &&
    flowState !== 'payment-processing'

  React.useEffect(() => {
    props.host.setListener({
      onWalletUpdated: setWalletInfo,
      onExchangeRatesUpdated: setRateInfo,
      onOrderUpdated: setOrderInfo,
      onRewardsEnabledUpdated: setRewardsEnabled
    })
  }, [props.host])

  const onClose = () => { props.host.closeDialog() }

  if (!rateInfo || !walletInfo || !orderInfo) {
    // TODO(zenparsing): Create a loading screen
    return (
      <DialogFrame
        showTitle={showTitle}
        showBackground={false}
        onClose={onClose}
      >
        Loading...
      </DialogFrame>
    )
  }

  const onShowAddFunds = () => setFlowState('add-funds')
  const onCancelAddFunds = () => setFlowState('start')

  const formatExchange = createExchangeFormatter(
    rateInfo.rates,
    props.exchangeCurrency)

  const amountNeeded = Math.max(0, orderInfo.total - walletInfo.balance)

  function getAddFundsAmounts () {
    // TODO(zenparsing): Calculate three options
    return []
  }

  return (
    <DialogFrame
      showTitle={showTitle}
      showBackground={showBackground}
      onClose={onClose}
    >
      {
        flowState === 'start' ?
          <PaymentMethodPanel
            rewardsEnabled={rewardsEnabled}
            orderDescription={orderInfo.description}
            orderTotal={formatTokenValue(orderInfo.total)}
            orderTotalConverted={formatExchange(orderInfo.total)}
            walletBalance={formatTokenValue(walletInfo.balance)}
            walletBalanceConverted={formatExchange(walletInfo.balance)}
            walletLastUpdated={formatLastUpdatedDate(rateInfo.lastUpdated)}
            walletVerified={walletInfo.verified}
            hasSufficientFunds={amountNeeded <= 0}
            onPayWithCreditCard={props.host.payWithCreditCard}
            onPayWithWallet={props.host.payWithWallet}
            onShowAddFunds={onShowAddFunds}
          /> :
        flowState === 'add-funds' ?
          <AddFundsPanel
            amountNeeded={formatTokenValue(amountNeeded)}
            walletBalance={formatTokenValue(walletInfo.balance)}
            walletBalanceConverted={formatExchange(walletInfo.balance)}
            unitValueConverted={formatExchange(1)}
            amountOptions={getAddFundsAmounts()}
            onCancel={onCancelAddFunds}
            onPayWithCreditCard={props.host.payWithCreditCard}
          /> :
        flowState === 'payment-processing' ?
          <PaymentProcessing /> :
        flowState === 'payment-complete' ?
          <PaymentComplete /> : ''
      }
    </DialogFrame>
  )
}
