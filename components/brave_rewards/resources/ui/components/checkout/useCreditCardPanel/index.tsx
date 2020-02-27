/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { LocaleContext } from '../localeContext'
import { FormSection } from '../formSection'
import { CreditCardForm, CreditCardDetails, CreditCardFormHandle } from '../creditCardForm'
import { GoBackLink } from '../goBackLink'

import {
  ContinueBox,
  ContinueBoxLink,
  ContinueBoxText,
  ConfirmButtonRow,
  ConfirmButton,
  TermsOfSale,
  RightIcon
} from './style'

interface UseCreditCardPanelProps {
  hasSufficientFunds?: boolean
  rewardsEnabled?: boolean
  walletVerified?: boolean
  continueWithCard?: boolean
  setContinueWithCard: (value: boolean) => void
  onPayWithCreditCard: (cardDetails: CreditCardDetails) => void
}

export function UseCreditCardPanel (props: UseCreditCardPanelProps) {
  const locale = React.useContext(LocaleContext)
  const creditCardFormRef = React.useRef<CreditCardFormHandle>(null)

  const onConfirmClick = () => {
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

  const onBack = () => {
    props.setContinueWithCard(false)
  }

  const title = props.rewardsEnabled && !props.continueWithCard
    ? locale.get('useCreditCard')
    : locale.get('enterCreditCardInfo')

  const showForm =
    props.continueWithCard ||
    !props.rewardsEnabled ||
    (!props.hasSufficientFunds && !props.walletVerified)

  React.useEffect(() => {
    if (showForm && creditCardFormRef.current) {
      creditCardFormRef.current.focus()
    }
  }, [showForm])

  if (showForm) {
    return (
      <div>
        <FormSection title={title}>
          <CreditCardForm handleRef={creditCardFormRef} />
        </FormSection>
        <ConfirmButtonRow showBackLink={props.continueWithCard}>
          {props.continueWithCard && <GoBackLink onClick={onBack} />}
          <div>
            <ConfirmButton
              showBackLink={props.continueWithCard}
              text={locale.get('confirmButtonText')}
              size='medium'
              onClick={onConfirmClick}
              type='accent'
              brand='rewards'
            />
          </div>
        </ConfirmButtonRow>
        <TermsOfSale>
          <span dangerouslySetInnerHTML={{ __html: locale.get('confirmTermsOfSale') }} />
        </TermsOfSale>
      </div>
    )
  }

  const onContinueClick = (event: React.MouseEvent) => {
    event.preventDefault()
    props.setContinueWithCard(true)
  }

  return (
    <FormSection title={title}>
      <ContinueBox>
        <ContinueBoxText>
          {locale.get('continueWithCreditCardMessage')}
        </ContinueBoxText>
        <ContinueBoxLink>
          <a href='#' onClick={onContinueClick}>
            {locale.get('continueWithCreditCard')}
            <RightIcon />
          </a>
        </ContinueBoxLink>
      </ContinueBox>
    </FormSection>
  )
}
