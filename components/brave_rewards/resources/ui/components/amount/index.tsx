/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { StyledWrapper, StyledAmount, StyledNumber, StyledLogo, StyledConverted, StyledTokens } from './style'
import { getLocale } from 'brave-ui/helpers'
import { BatColorIcon } from 'brave-ui/components/icons'

export interface Props {
  amount: string
  converted: string
  onSelect: (amount: string) => void
  id?: string
  selected?: boolean
  type?: 'big' | 'small'
  currency?: string
  isMobile?: boolean,
  onlyAnonWallet?: boolean
}

export default class Amount extends React.PureComponent<Props, {}> {
  static defaultProps = {
    type: 'small',
    currency: 'USD',
    converted: 0
  }

  getAboutText = (isMobile?: boolean) => {
    return isMobile ? '' : getLocale('about')
  }

  getBatString = () => {
    const { onlyAnonWallet, type } = this.props

    if (type !== 'big') {
      return null
    }

    return getLocale(onlyAnonWallet ? 'bap' : 'bat')
  }

  render () {
    const { id, onSelect, amount, selected, type, converted, currency, isMobile } = this.props

    return (
      <StyledWrapper id={id} onClick={onSelect.bind(this, amount)} isMobile={isMobile} data-test-id={'amount-wrapper'}>
        <StyledAmount selected={selected} type={type} isMobile={isMobile}>
          <StyledLogo isMobile={isMobile}><BatColorIcon /></StyledLogo><StyledNumber>{amount}</StyledNumber> <StyledTokens>{this.getBatString()}</StyledTokens>
        </StyledAmount>
        <StyledConverted selected={selected} type={type} isMobile={isMobile}>
          {this.getAboutText(isMobile)} {converted} {currency}
        </StyledConverted>
      </StyledWrapper>
    )
  }
}
