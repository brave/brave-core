/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { Toggle } from 'brave-ui/components'

import { LocaleContext } from '../localeContext'
import { Behaviors } from './behaviors'
import { getCreditCardIcon } from './icons'
import { CreditCardDetails, CreditCardError, CreditCardErrorType } from './types'

import {
  Container,
  InputBox,
  CardNumber,
  Expiration,
  SecurityCode,
  SaveThisCard,
  SaveThisCardLabel
} from './style'

export { CreditCardDetails }

function useCardErrorState () {
  return React.useState<CreditCardErrorType>('')
}

export interface CreditCardFormHandle {
  focus: () => void
  validate: () => CreditCardError[]
  details: CreditCardDetails
}

interface CreditCardFormProps {
  handleRef: React.MutableRefObject<CreditCardFormHandle | null>
}

export function CreditCardForm (props: CreditCardFormProps) {
  const locale = React.useContext(LocaleContext)

  // Internal ref for Behavior instance associated with form
  const behaviorsRef = React.useRef<Behaviors | null>(null)

  // Refs for input elements
  const cardNumberRef = React.useRef<HTMLInputElement>(null)
  const expiryRef = React.useRef<HTMLInputElement>(null)
  const securityCodeRef = React.useRef<HTMLInputElement>(null)

  // Form state
  const [saveCardChecked, setSaveCardChecked] = React.useState(true)
  const [cardTypeId, setCardTypeId] = React.useState('')
  const [cardNumberError, setCardNumberError] = useCardErrorState()
  const [expiryError, setExpiryError] = useCardErrorState()
  const [securityCodeError, setSecurityCodeError] = useCardErrorState()

  React.useEffect(() => {
    // Create a Behaviors instance after first render
    behaviorsRef.current = new Behaviors({
      inputs: {
        cardNumber: cardNumberRef.current!,
        expiry: expiryRef.current!,
        securityCode: securityCodeRef.current!
      },
      onCardTypeChange: (cardType) => {
        setCardTypeId(cardType ? cardType.id : '')
      },
      onInputValidation: (error) => {
        switch (error.element) {
          case cardNumberRef.current:
            setCardNumberError(error.type)
            break
          case expiryRef.current:
            setExpiryError(error.type)
            break
          case securityCodeRef.current:
            setSecurityCodeError(error.type)
            break
        }
      }
    })
  }, [])

  React.useEffect(() => {
    // Expose Behaviors instance to callers through
    // the `handleRef` property.
    props.handleRef.current = behaviorsRef.current
  }, [props.handleRef])

  const CardIcon = getCreditCardIcon(cardTypeId)
  const toggleSaveCard = () => { setSaveCardChecked(!saveCardChecked) }

  return (
    <Container>
      <CardNumber>
        <label>
          {locale.get('cardNumber')}
          <InputBox invalid={Boolean(cardNumberError)}>
            <CardIcon />
            <input ref={cardNumberRef} autoComplete='cc-number' />
          </InputBox>
        </label>
      </CardNumber>
      <Expiration>
        <label>
          {locale.get('expiration')}
          <InputBox invalid={Boolean(expiryError)}>
            <input ref={expiryRef} placeholder={'MM/YY'} autoComplete='cc-exp' />
          </InputBox>
        </label>
      </Expiration>
      <SecurityCode>
        <label>
          {locale.get('securityCode')}
          <InputBox invalid={Boolean(securityCodeError)}>
            <input ref={securityCodeRef} autoComplete='cc-cvc' />
          </InputBox>
        </label>
      </SecurityCode>
      <SaveThisCard>
        <SaveThisCardLabel>
          <div>
            {locale.get('saveThisCard')}
          </div>
          <div>
            <Toggle checked={saveCardChecked} onToggle={toggleSaveCard} />
          </div>
        </SaveThisCardLabel>
      </SaveThisCard>
    </Container>
  )
}
