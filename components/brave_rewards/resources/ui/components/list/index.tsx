/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { StyledWrapper, StyledTitle, StyledContentWrapper } from './style'

export interface Props {
  id?: string
  title?: string | React.ReactNode
  children?: React.ReactNode
}

export default class List extends React.PureComponent<Props, {}> {
  render () {
    const { id, title, children } = this.props

    return (
      <StyledWrapper id={id}>
        <StyledTitle>{title}</StyledTitle>
        <StyledContentWrapper>
          {children}
        </StyledContentWrapper>
      </StyledWrapper>
    )
  }
}
