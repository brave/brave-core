/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { storiesOf } from '@storybook/react'
import * as knobs from '@storybook/addon-knobs'

import { LocaleContext } from '../components/checkout/localeContext'
import { DialogFrame } from '../components/checkout/dialogFrame'
import { PaymentMethodPanel } from '../components/checkout/paymentMethodPanel'
import { AddFundsPanel } from '../components/checkout/addFundsPanel'
import { PaymentProcessing } from '../components/checkout/paymentProcessing'
import { PaymentComplete } from '../components/checkout/paymentComplete'

function onDialogClose () {
  // Empty
}

function actionLogger (type: string) {
  return (data?: any) => { console.log(type, ':', data) }
}

const localeData = {
  strings: {
    bat: 'BAT',
    addFundsTitle: 'Add Funds',
    addFundsSubtitle: 'Add BAT to your wallet using your credit card',
    currentBalance: 'Current Balance',
    batNeeded: 'BAT needed',
    selectAmountToAddStep: '1. Select amount to add',
    enterCreditCardStep: '2. Enter credit card info',
    transactionFee: 'Transaction Fee',
    orderTotal: 'Order Total',
    enterCreditCardInfo: 'Enter credit card info',
    addFundsButtonText: 'Add Funds & Purchase',
    addFundsTermsOfSale: 'By clicking Add Funds & Purchase, you agree to ' +
      '<a href="javascript:void 0">Brave’s Terms of Sale</a>.',
    cardNumber: 'Card number',
    expiration: 'Expiration',
    securityCode: 'Security code',
    saveThisCard: 'Save this card',
    checkout: 'Checkout',
    goBack: 'Go Back',
    itemSelected: 'Item Selected',
    goodToGo: 'You’re good to go!',
    enjoyYourPurchase: 'Enjoy your purchase.',
    paymentMethodTitle: 'Payment Method',
    paymentProcessing: 'Your payment is being processed…',
    confirmButtonText: 'Confirm',
    confirmTermsOfSale: 'By clicking Confirm, you agree ' +
      '<a href="javascript:void 0">Brave’s Terms of Sale</a>.',
    useCreditCard: 'Use credit card',
    continueWithCreditCard: 'Continue with credit card',
    continueWithCreditCardMessage: 'Make a one-time purchase using a credit card instead.',
    payWithBat: 'Pay with BAT',
    payWithBatTermsOfSale: 'By clicking Pay with BAT, you agree to ' +
      '<a href="javascript:void 0">Brave’s Terms of Sale</a>.',
    addFundsLinkText: 'Add Funds',
    notEnoughFunds: 'You don\'t have enough tokens to buy this item.',
    useTokenBalance: 'Use your token balance',
    updated: 'Updated'
  },
  get (key: string) {
    return this.strings[key] || 'MISSING'
  }
}

storiesOf('Rewards/Checkout', module)
  .addDecorator(knobs.withKnobs)
  .add('Payment Method', () => {
    const defaultDescription =
      'Title of the selected item long title long ' +
      'long title wrapped into second line'

    return (
      <LocaleContext.Provider value={localeData}>
        <DialogFrame showBackground={true} showTitle={true} onClose={onDialogClose}>
          <PaymentMethodPanel
            rewardsEnabled={knobs.boolean('rewardsEnabled', true)}
            orderDescription={knobs.text('orderDescription', defaultDescription)}
            orderTotal={knobs.text('orderTotal', '45.0')}
            orderTotalConverted={knobs.text('orderTotalConverted', '$9.00')}
            walletBalance={knobs.text('walletBalance', '100.0')}
            walletBalanceConverted={knobs.text('walletBalanceConverted', '$20.00')}
            walletLastUpdated={knobs.text('walletLastUpdated', 'Today at 11:38 am')}
            walletVerified={knobs.boolean('walletVerified', true)}
            hasSufficientFunds={knobs.boolean('hasSufficientFunds', true)}
            onPayWithCreditCard={actionLogger('onPaymentWithCreditCard')}
            onPayWithWallet={actionLogger('onPaymentWithWallet')}
            onShowAddFunds={actionLogger('onShowAddFunds')}
          />
        </DialogFrame>
      </LocaleContext.Provider>
    )
  })
  .add('Add Funds', () => {
    const defaultAmounts = [
      {
        amount: 30,
        amountConverted: '$6.00',
        transactionFeeRate: '3%',
        transactionFee: '$0.18',
        totalCharge: '$6.18'
      },
      {
        amount: 50,
        amountConverted: '$10.00',
        transactionFeeRate: '3%',
        transactionFee: '$0.18',
        totalCharge: '$10.18'
      },
      {
        amount: 100,
        amountConverted: '$20.00',
        transactionFeeRate: '3%',
        transactionFee: '$0.18',
        totalCharge: '$20.18'
      }
    ]

    return (
      <LocaleContext.Provider value={localeData}>
        <DialogFrame showBackground={true} showTitle={true} onClose={onDialogClose}>
          <AddFundsPanel
            amountNeeded={knobs.text('amountNeeded', '30')}
            walletBalance={knobs.text('walletBalance', '15.0')}
            walletBalanceConverted={knobs.text('walletBalanceConverted', '$3.00')}
            unitValueConverted={knobs.text('unitValueConverted', '$0.1873')}
            amountOptions={knobs.object('amountOptions', defaultAmounts)}
            onCancel={actionLogger('onCancel')}
            onPayWithCreditCard={actionLogger('onPayWithCreditCard')}
          />
        </DialogFrame>
      </LocaleContext.Provider>
    )
  })
  .add('Processing', () => {
    return (
      <LocaleContext.Provider value={localeData}>
        <DialogFrame showTitle={true} onClose={onDialogClose}>
          <PaymentProcessing />
        </DialogFrame>
      </LocaleContext.Provider>
    )
  })
  .add('Complete', () => {
    return (
      <LocaleContext.Provider value={localeData}>
        <DialogFrame onClose={onDialogClose}>
          <PaymentComplete />
        </DialogFrame>
      </LocaleContext.Provider>
    )
  })
