/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import styled from 'styled-components'
import Button from '@brave/leo/react/button'
import { color, font } from '@brave/leo/tokens/css/variables'

import SecureLink from '$web-common/SecureLink'
import { getLocale, formatLocale } from '$web-common/locale'
import { useBraveNews } from './shared/Context'

import optInImage from './braveNewsLogo.svg'

const Container = styled.div`
  background: rgba(53, 53, 53, 0.47);
  backdrop-filter: blur(62px);
  border-radius: 16px;
  padding: 44px 82px 36px;
  width: 680px;

  color: ${color.text.primary};
  font: ${font.default.regular};
  text-align: center;

  display: flex;
  flex-direction: column;
  gap: 32px;

  > * {
    display: flex;
    flex-direction: column;
    gap: 16px;
  }

  h3 {
    font: ${font.heading.h3};
    margin: 0;
  }

  p {
    margin: 0;
  }

  a {
    color: inherit;
  }

  .graphic {
    background-image: url(${optInImage});
    background-size: contain;
    background-position: center;
    background-repeat: no-repeat;
    height: 60px;
  }
`

const description = formatLocale('braveNewsIntroDescriptionTwo', {
  $1: (content) => (
    <SecureLink href='https://brave.com/privacy/browser/'>{content}</SecureLink>
  )
})

export default function OptIn() {
  const { toggleBraveNewsOnNTP } = useBraveNews()

  return (
    <Container data-theme='dark'>
      <div className='graphic' />
      <h3>{getLocale('braveNewsIntroTitle')}</h3>
      <div>
        <p>{getLocale('braveNewsIntroDescription')}</p>
        <p>{description}</p>
      </div>
      <div>
        <Button
          kind='filled'
          onClick={() => toggleBraveNewsOnNTP(true)}
        >
          {getLocale('braveNewsOptInActionLabel')}
        </Button>
        <Button
          kind='plain-faint'
          onClick={() => toggleBraveNewsOnNTP(false)}
        >
          {getLocale('braveNewsOptOutActionLabel')}
        </Button>
      </div>
    </Container>
  )
}
