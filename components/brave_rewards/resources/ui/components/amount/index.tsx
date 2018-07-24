/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { StyledWrapper, StyledAmount, StyledLogo, StyledConverted, StyledTokens } from './style'
import { getLocale } from '../../../helpers'

export interface Props {
  amount: number
  converted: number
  onSelect: (amount: number) => void
  id?: string
  selected?: boolean
  type?: 'big' | 'small'
  currency?: string
}

const logo = require('./assets/logo')

export default class Amount extends React.PureComponent<Props, {}> {
  render () {
    const { id, onSelect, amount, selected, type } = this.props
    const converted = this.props.converted || 0
    const currency = this.props.currency || 'USD'

    return (
      <StyledWrapper id={id} onClick={onSelect.bind(this, amount)}>
        <StyledAmount selected={selected} type={type}>
          <StyledLogo>{logo}</StyledLogo>{amount} <StyledTokens>{type === 'big' ? 'tokens' : null}</StyledTokens>
        </StyledAmount>
        <StyledConverted selected={selected} type={type}>
          {getLocale('about')} {converted.toFixed(2)} {currency}
        </StyledConverted>
      </StyledWrapper>
    )
  }
}
