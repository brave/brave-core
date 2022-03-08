/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { getLocale } from 'brave-ui/helpers'
import { StyledWrapper, StyledTokens, StyledContent, StyledTokenValue, StyledTokenCurrency } from './style'

export type Size = 'mini' | 'small' | 'normal'
export type Type = 'contribute' | 'earning' | 'default'

export interface Props {
  value: string
  id?: string
  converted?: string
  currency?: string
  hideText?: boolean
  isNegative?: boolean
  size?: Size
  color?: Type
  showApproxSign?: boolean
}

export default class Tokens extends React.PureComponent<Props, {}> {
  static defaultProps = {
    size: 'normal',
    color: 'default',
    currency: 'USD',
    showApproxSign: false
  }

  render () {
    const { id, converted, value, hideText, isNegative, size, color, currency, showApproxSign } = this.props
    const batFormatString = getLocale('bat')
    const approxSign = showApproxSign ? '≈' : ''

    return (
      <StyledWrapper id={id} size={size} color={color}>
        <StyledTokens>
          <StyledTokenValue>
            {isNegative ? '-' : ''}{value}
          </StyledTokenValue>
          {
            !hideText
            ? <StyledTokenCurrency>
                {batFormatString}
              </StyledTokenCurrency>
            : null
          }
        </StyledTokens>
        {
          converted !== undefined
          ? <StyledContent showApproxSign={showApproxSign}>
            {approxSign} {converted} {currency}
          </StyledContent>
          : null
        }
      </StyledWrapper>
    )
  }
}
