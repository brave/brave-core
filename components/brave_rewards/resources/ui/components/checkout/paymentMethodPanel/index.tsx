/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { LocaleContext } from '../localeContext'
import { DialogTitle } from '../dialogTitle'
import { OrderSummary } from '../orderSummary'
import { UseWalletPanel } from '../useWalletPanel'
import { UseCreditCardPanel } from '../useCreditCardPanel'
import { CreditCardDetails } from '../creditCardForm'

interface PaymentMethodPanelProps {
  canUseCreditCard: boolean
  rewardsEnabled: boolean
  orderTotal: string
  orderTotalConverted: string
  hasSufficientFunds: boolean
  walletBalance: string
  walletBalanceConverted: string
  walletVerified: boolean
  walletLastUpdated: string
  onPayWithCreditCard?: (cardDetails: CreditCardDetails) => void
  onPayWithWallet: () => void
  onShowAddFunds?: () => void
}

export function PaymentMethodPanel (props: PaymentMethodPanelProps) {
  const locale = React.useContext(LocaleContext)
  const [continueWithCard, setContinueWithCard] = React.useState(false)

  return (
    <>
      <DialogTitle>{locale.get('paymentMethodTitle')}</DialogTitle>
      <OrderSummary
        orderTotal={props.orderTotal}
        orderTotalConverted={props.orderTotalConverted}
      />
      {
        continueWithCard ? null :
          <UseWalletPanel
            canAddFunds={props.canUseCreditCard}
            balance={props.walletBalance}
            balanceConverted={props.walletBalanceConverted}
            lastUpdated={props.walletLastUpdated}
            hasSufficientFunds={props.hasSufficientFunds}
            rewardsEnabled={props.rewardsEnabled}
            walletVerified={props.walletVerified}
            onShowAddFunds={props.onShowAddFunds}
            onPayWithWallet={props.onPayWithWallet}
          />
      }
      { !props.canUseCreditCard ? null :
          <UseCreditCardPanel
            hasSufficientFunds={props.hasSufficientFunds}
            rewardsEnabled={props.rewardsEnabled}
            walletVerified={props.walletVerified}
            continueWithCard={continueWithCard}
            setContinueWithCard={setContinueWithCard}
            onPayWithCreditCard={props.onPayWithCreditCard}
          />
      }
    </>
  )
}
