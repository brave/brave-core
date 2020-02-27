/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { LocaleContext } from '../localeContext'
import { Container, LeftIcon } from './style'

interface GoBackLinkProps {
  onClick: () => void
}

export function GoBackLink (props: GoBackLinkProps) {
  const locale = React.useContext(LocaleContext)
  const handleClick = (event: React.MouseEvent) => {
    event.preventDefault()
    props.onClick()
  }
  return (
    <Container>
      <a href='#' onClick={handleClick}>
        <LeftIcon />
        {locale.get('goBack')}
      </a>
    </Container>
  )
}
