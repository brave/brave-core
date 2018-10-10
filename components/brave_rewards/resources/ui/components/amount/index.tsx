/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { StyledWrapper, StyledAmount, StyledNumber, StyledLogo, StyledConverted, StyledTokens } from './style'
import { getLocale } from '../../../helpers'
import { BatColorIcon } from '../../../components/icons'

export interface Props {
  amount: string
  converted: string
  onSelect: (amount: number) => void
  id?: string
  selected?: boolean
  type?: 'big' | 'small'
  currency?: string
}

export default class Amount extends React.PureComponent<Props, {}> {
  static defaultProps = {
    type: 'small',
    currency: 'USD',
    converted: 0
  }

  render () {
    const { id, onSelect, amount, selected, type, converted, currency } = this.props

    return (
      <StyledWrapper id={id} onClick={onSelect.bind(this, amount)}>
        <StyledAmount selected={selected} type={type}>
          <StyledLogo><BatColorIcon /></StyledLogo><StyledNumber>{amount}</StyledNumber> <StyledTokens>{type === 'big' ? 'BAT' : null}</StyledTokens>
        </StyledAmount>
        <StyledConverted selected={selected} type={type}>
          {getLocale('about')} {converted} {currency}
        </StyledConverted>
      </StyledWrapper>
    )
  }
}
