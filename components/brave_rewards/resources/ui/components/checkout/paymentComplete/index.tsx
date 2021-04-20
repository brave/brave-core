/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { LocaleContext } from '../localeContext'
import { VectorImage } from './vectorImage'
import { Container, ImageContainer, TextContainer, Header, Text } from './style'

interface PaymentCompleteProps {
  onClose: (reason: number) => void
  reason: number
}

export function PaymentComplete (props: PaymentCompleteProps) {
  const locale = React.useContext(LocaleContext)
  const TIMEOUT_SECONDS = 1

  window.setTimeout(function () {
    props.onClose(props.reason)
  }, TIMEOUT_SECONDS * 1000)

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
