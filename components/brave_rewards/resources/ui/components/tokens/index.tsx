/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { StyledTokens, StyledContent, StyledTokenValue, StyledTokenCurrency } from './style'

export type Size = 'small' | 'normal'
export type Type = 'contribute' | 'donation' | 'earnings' | 'default' | 'notPaid'

export interface Props {
  value: number
  id?: string
  converted?: number
  currency?: string
  hideText?: boolean
  toFixed?: boolean
  isNegative?: boolean
  size?: Size
  color?: Type
}

export default class Tokens extends React.PureComponent<Props, {}> {
  render () {
    const { id, converted, value, hideText, isNegative, size, color } = this.props
    const currency = this.props.currency || 'USD'
    const toFixed = this.props.toFixed === undefined ? true : this.props.toFixed

    return (
      <span id={id}>
        <StyledTokens>
          <StyledTokenValue size={size} color={color}>
            {isNegative ? '-' : ''}{toFixed ? value.toFixed(1) : value}
          </StyledTokenValue>
          {
            !hideText
            ? <StyledTokenCurrency size={size}>BAT</StyledTokenCurrency>
            : null
          }
        </StyledTokens>
        {
          converted !== undefined
          ? <StyledContent size={size}>
            {toFixed ? converted.toFixed(2) : converted} {currency}
          </StyledContent>
          : null
        }
      </span>
    )
  }
}
