/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { StyledWrapper, StyledTitle, StyledContentWrapper } from './style'
import Tokens, { Size, Type } from '../tokens/index'

export interface Props {
  title: React.ReactNode
  value: string
  converted: string
  id?: string
  isNegative?: boolean
  size?: Size
  color?: Type
  border?: 'first' | 'last' | 'default'
  testId?: string
}

export default class ListToken extends React.PureComponent<Props, {}> {
  static defaultProps = {
    border: 'default'
  }

  render () {
    const { id, title, value, converted, isNegative, size, color, border, testId } = this.props

    return (
      <StyledWrapper id={id} border={border} data-test-id={testId}>
        <StyledTitle>{title}</StyledTitle>
        <StyledContentWrapper>
          <Tokens
            value={value}
            converted={converted}
            isNegative={isNegative}
            size={size}
            color={color}
          />
        </StyledContentWrapper>
      </StyledWrapper>
    )
  }
}
