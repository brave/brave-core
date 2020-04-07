/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { LocaleContext } from '../localeContext'
import { Container, LoadIcon } from './style'

export function PaymentProcessing (props: {}) {
  const locale = React.useContext(LocaleContext)
  return (
    <Container>
      <LoadIcon />
      <div>
        {locale.get('paymentProcessing')}
      </div>
    </Container>
  )
}
