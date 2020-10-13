/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { LocaleContext, getLocaleWithTag } from '../localeContext'
import { DialogTitle } from '../dialogTitle'
import { FormSection } from '../formSection'
import { CreditCardForm, CreditCardFormHandle, CreditCardDetails } from '../creditCardForm'
import { GoBackLink } from '../goBackLink'

import {
  Subtitle,
  CurrentBalance,
  CurrentBalanceNeeded,
  CurrentBalanceBat,
  CurrentBalanceConverted,
  ExchangeRateDisplay,
  BatIcon,
  AmountOptionList,
  AmountOptionContainer,
  AmountOptionButton,
  AmountOptionExchange,
  ChargeSummary,
  ChargeSummaryTotal,
  ChargeSummaryTotalAmount,
  PurchaseButtonRow,
  AddFundsButton,
  TermsOfSale
} from './style'

interface AddFundsAmountOption {
  amount: number
  amountConverted: string
  transactionFeeRate: string
  transactionFee: string
  totalCharge: string
}

interface AmountOpionPanelProps {
  amountOptions: AddFundsAmountOption[]
  selectedAmount: number
  setSelectedAmount: (amount: number) => void
}

function AmountOptionPanel (props: AmountOpionPanelProps) {
  if (props.amountOptions.length === 0) {
    return null
  }

  const locale = React.useContext(LocaleContext)

  let selectedOption = props.amountOptions[0]
  for (const option of props.amountOptions) {
    if (option.amount === props.selectedAmount) {
      selectedOption = option
      break
    }
  }

  return (
    <>
        <AmountOptionList>
        {
          props.amountOptions.map(option => {
            const selectAmount = () => { props.setSelectedAmount(option.amount) }
            return (
              <AmountOptionContainer key={option.amount}>
                <AmountOptionButton
                  text={`${option.amount} ${locale.get('bat')}`}
                  size={'medium'}
                  onClick={selectAmount}
                  selected={option === selectedOption}
                />
                <AmountOptionExchange>
                  {option.amountConverted}
                </AmountOptionExchange>
              </AmountOptionContainer>
            )
          })
        }
      </AmountOptionList>
      <ChargeSummary>
        <div>{locale.get('transactionFee')} ({selectedOption.transactionFeeRate})</div>
        <div>{selectedOption.transactionFee}</div>
        <ChargeSummaryTotal>{locale.get('orderTotal')}</ChargeSummaryTotal>
        <ChargeSummaryTotalAmount>{selectedOption.totalCharge}</ChargeSummaryTotalAmount>
      </ChargeSummary>
    </>
  )
}

interface AddFundsPanelProps {
  amountNeeded: string
  walletBalance: string
  walletBalanceConverted: string
  unitValueConverted: string
  amountOptions: AddFundsAmountOption[]
  onCancel: () => void
  onPayWithCreditCard: (cardDetails: CreditCardDetails) => void
}

export function AddFundsPanel (props: AddFundsPanelProps) {
  const locale = React.useContext(LocaleContext)

  const [selectedAmount, setSelectedAmount] = React.useState<number>(0)
  const creditCardFormRef = React.useRef<CreditCardFormHandle>(null)

  const onPurchaseClick = () => {
    const formHandle = creditCardFormRef.current
    if (formHandle) {
      const errors = formHandle.validate()
      if (errors.length === 0) {
        props.onPayWithCreditCard(formHandle.details)
      } else {
        errors[0].element.focus()
      }
    }
  }

  const tags = getLocaleWithTag(locale.get('addFundsTermsOfSale'))

  return (
    <>
      <DialogTitle>{locale.get('addFundsTitle')}</DialogTitle>
      <Subtitle>
        {locale.get('addFundsSubtitle')}
      </Subtitle>
      <CurrentBalance>
        <div>
          {locale.get('currentBalance')}
          <CurrentBalanceBat>{props.walletBalance} {locale.get('bat')}</CurrentBalanceBat>
          <CurrentBalanceConverted>{props.walletBalanceConverted}</CurrentBalanceConverted>
        </div>
        <CurrentBalanceNeeded>
          {props.amountNeeded} {locale.get('batNeeded')}
        </CurrentBalanceNeeded>
      </CurrentBalance>
      <FormSection
        title={
          <>
            <span>{locale.get('selectAmountToAddStep')}</span>
            <ExchangeRateDisplay>
              <BatIcon /> 1 {locale.get('bat')} = {props.unitValueConverted}
            </ExchangeRateDisplay>
          </>
        }
      >
        <AmountOptionPanel
          amountOptions={props.amountOptions}
          selectedAmount={selectedAmount}
          setSelectedAmount={setSelectedAmount}
        />
      </FormSection>
      <FormSection title={locale.get('enterCreditCardStep')}>
        <CreditCardForm handleRef={creditCardFormRef} />
      </FormSection>
      <PurchaseButtonRow>
        <GoBackLink onClick={props.onCancel} />
        <AddFundsButton
          text={locale.get('addFundsButtonText')}
          size='medium'
          onClick={onPurchaseClick}
          type='accent'
          brand='rewards'
        />
      </PurchaseButtonRow>
      <TermsOfSale>
        <span>
          {tags.beforeTag}
          <a href='javascript:void 0'>{tags.duringTag}</a>
          {tags.afterTag}
        </span>
      </TermsOfSale>
    </>
  )
}
