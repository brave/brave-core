/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { LocaleContext } from '../localeContext'
import { Container, LoadIcon } from './style'

interface PaymentProcessingProps {
  paymentDone: () => void
}

export function PaymentProcessing (props: PaymentProcessingProps) {
  const locale = React.useContext(LocaleContext)
  const TIMEOUT_SECONDS = 1

  window.setTimeout(props.paymentDone, TIMEOUT_SECONDS * 1000)

  return (
    <Container>
      <LoadIcon />
      <div>
        {locale.get('paymentProcessing')}
      </div>
    </Container>
  )
}
