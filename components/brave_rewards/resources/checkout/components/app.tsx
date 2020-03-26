/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { LocaleContext } from '../../ui/components/checkout/localeContext'
import { DialogFrame } from '../../ui/components/checkout/dialogFrame'
import { EnableRewardsPanel } from '../../ui/components/checkout/enableRewardsPanel'
import { PaymentMethodPanel } from '../../ui/components/checkout/paymentMethodPanel'
import { LoadingPanel } from '../../ui/components/checkout/loadingPanel'
import { ErrorPanel } from '../../ui/components/checkout/errorPanel'
import { AddFundsPanel } from '../../ui/components/checkout/addFundsPanel'
import { PaymentProcessing } from '../../ui/components/checkout/paymentProcessing'
import { PaymentComplete } from '../../ui/components/checkout/paymentComplete'

import {
  WalletInfo,
  OrderInfo,
  ExchangeRateInfo,
  Settings,
  ServiceError,
  Host
} from '../interfaces'

import {
  createExchangeFormatter,
  formatLastUpdatedDate,
  formatTokenValue
} from '../formatting'

// A user-defined hook that and an ESC key handler to the
// document body while the component is mounted.
function useEscapeKeyHandler (host: Host) {
  const onEscapePressed = React.useCallback((event: KeyboardEvent) => {
    if (event.key.toLowerCase() === 'escape') {
      host.cancelPayment()
    }
  }, [host])

  React.useEffect(() => {
    document.body.addEventListener('keyup', onEscapePressed)
    return () => {
      document.body.removeEventListener('keyup', onEscapePressed)
    }
  }, [host])
}

type FlowState =
  'start' |
  'add-funds' |
  'payment-processing' |
  'payment-complete'

interface AppProps {
  host: Host
  exchangeCurrency: string
}

export function App (props: AppProps) {
  const locale = React.useContext(LocaleContext)

  useEscapeKeyHandler(props.host)

  const [flowState, setFlowState] = React.useState<FlowState>('start')
  const [rateInfo, setRateInfo] = React.useState<ExchangeRateInfo | undefined>()
  const [walletInfo, setWalletInfo] = React.useState<WalletInfo | undefined>()
  const [orderInfo, setOrderInfo] = React.useState<OrderInfo | undefined>()
  const [settings, setSettings] = React.useState<Settings | undefined>()
  const [serviceError, setServiceError] = React.useState<ServiceError | undefined>()

  React.useEffect(() => {
    return props.host.addListener((state) => {
      setRateInfo(state.exchangeRateInfo)
      setOrderInfo(state.orderInfo)
      setWalletInfo(state.walletInfo)
      setSettings(state.settings)
      setServiceError(state.serviceError)

      switch (state.paymentStatus) {
        case 'processing':
          setFlowState('payment-processing')
          break
        case 'confirmed':
          setFlowState('payment-complete')
          break
      }
    })
  }, [props.host])

  const onClose = () => { props.host.cancelPayment() }

  if (serviceError) {
    return (
      <DialogFrame showTitle={true} onClose={onClose}>
        <ErrorPanel
          text={locale.get('errorHasOccurred')}
          details={`(${ serviceError.type }:${ serviceError.status })`}
        />
      </DialogFrame>
    )
  }

  if (
    !settings ||
    !rateInfo ||
    !orderInfo ||
    !walletInfo ||
    walletInfo.state === 'creating'
  ) {
    return (
      <DialogFrame showTitle={true} onClose={onClose}>
        <LoadingPanel text={''} />
      </DialogFrame>
    )
  }

  if (!settings.rewardsEnabled) {
    return (
      <DialogFrame showBackground={true} onClose={onClose}>
        <EnableRewardsPanel onEnableRewards={props.host.enableRewards} />
      </DialogFrame>
    )
  }

  const showTitle =
    flowState !== 'payment-complete'

  const showBackground =
    flowState !== 'payment-complete' &&
    flowState !== 'payment-processing'

  const onShowAddFunds = () => setFlowState('add-funds')
  const onCancelAddFunds = () => setFlowState('start')

  const formatExchange = createExchangeFormatter(
    rateInfo.rates,
    props.exchangeCurrency)

  const amountNeeded = Math.max(0, orderInfo.total - walletInfo.balance)

  function getAddFundsAmounts () {
    // TODO(zenparsing): Calculate three options. This implementation
    // will need to be completed for credit card payment integration.
    return []
  }

  return (
    <DialogFrame
      showTitle={showTitle}
      showBackground={showBackground}
      onClose={flowState === 'payment-processing' ? undefined : onClose}
    >
      {
        flowState === 'start' ?
          <PaymentMethodPanel
            canUseCreditCard={false}
            rewardsEnabled={settings.rewardsEnabled}
            orderDescription={orderInfo.description}
            orderTotal={formatTokenValue(orderInfo.total)}
            orderTotalConverted={formatExchange(orderInfo.total)}
            walletBalance={formatTokenValue(walletInfo.balance)}
            walletBalanceConverted={formatExchange(walletInfo.balance)}
            walletLastUpdated={formatLastUpdatedDate(rateInfo.lastUpdated)}
            walletVerified={walletInfo.state === 'verified'}
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
