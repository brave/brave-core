/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import * as CSS from 'csstype'
import { StyledTokens, StyledContent, StyledTokenValue, StyledTokenCurrency } from './style'

interface Theme {
  color?: {
    token?: CSS.Color
    tokenNum?: CSS.Color
    text?: CSS.Color
  }
  size?: {
    token?: CSS.FontSizeProperty<1>
    tokenNum?: CSS.FontSizeProperty<1>
    text?: CSS.FontSizeProperty<1>
  }
  display?: CSS.DisplayProperty
}

export interface Props {
  value: number
  id?: string
  converted?: number
  currency?: string
  hideText?: boolean
  toFixed?: boolean
  isNegative?: boolean
  theme?: Theme
}

export default class Tokens extends React.PureComponent<Props, {}> {
  render () {
    const { id, converted, value, hideText, isNegative, theme } = this.props
    const currency = this.props.currency || 'USD'
    const toFixed = this.props.toFixed === undefined ? true : this.props.toFixed

    return (
      <span id={id}>
        <StyledTokens theme={theme}>
          <StyledTokenValue theme={theme}>
            {isNegative ? '-' : ''}{toFixed ? value.toFixed(1) : value}
          </StyledTokenValue>
          {
            !hideText
            ? <StyledTokenCurrency theme={theme}>BAT</StyledTokenCurrency>
            : null
          }
        </StyledTokens>
        {
          converted !== undefined
          ? <StyledContent theme={theme}>
            {toFixed ? converted.toFixed(2) : converted} {currency}
          </StyledContent>
          : null
        }
      </span>
    )
  }
}
