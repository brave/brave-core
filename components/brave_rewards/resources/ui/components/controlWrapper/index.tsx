/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import * as CSS from 'csstype'
import { StyledWrapper, StyledTitle } from './style'

export interface Props {
  id?: string
  title?: React.ReactNode
  children?: React.ReactNode
  theme?: Theme
}

interface Theme {
  maxWidth?: CSS.MaxWidthProperty<1>
}

export default class ControlWrapper extends React.PureComponent<Props, {}> {
  render () {
    const { id, title, children, theme } = this.props

    return (
      <StyledWrapper id={id} theme={theme}>
        {title ? <StyledTitle>{title}</StyledTitle> : null}
        {children}
      </StyledWrapper>
    )
  }
}
