/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { LocaleContext } from '../localeContext'
import { VectorImage } from './vectorImage'
import { Container, ImageContainer, TextContainer, Header, Text } from './style'

export function PaymentComplete () {
  const locale = React.useContext(LocaleContext)
  return (
    <Container>
      <ImageContainer><VectorImage /></ImageContainer>
      <TextContainer>
        <Header>{locale.get('goodToGo')}</Header>
        <Text>{locale.get('enjoyYourPurchase')}</Text>
      </TextContainer>
    </Container>
  )
}
