/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { StyledWrapper, StyledTitle, StyledContent, StyledContentWrapper } from './style'

export interface Props {
  id?: string
  title?: string
  children?: React.ReactNode
  theme?: {[key: string]: string}
}

export default class List extends React.PureComponent<Props, {}> {
  render () {
    const { id, title, children, theme } = this.props

    return (
      <StyledWrapper id={id}>
        <StyledTitle>{title}</StyledTitle>
        <StyledContentWrapper>
          <StyledContent theme={theme}>{children}</StyledContent>
        </StyledContentWrapper>
      </StyledWrapper>
    )
  }
}
