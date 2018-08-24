/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import {
  StyledWrapper,
  StyledTitle,
  StyledAllowText,
  StyledClose,
  StyledAllowToggle,
  StyledTipWrapper
} from './style'
import Donate from '../donate/index'
import Toggle from '../../../components/formControls/toggle/index'
import { getLocale } from '../../../helpers'

type Donation = {tokens: number, converted: number, selected?: boolean}

export interface Props {
  allow: boolean
  provider: string
  balance: number
  currentAmount: number
  donationAmounts: Donation[]
  onAmountSelection?: (tokens: number) => void
  onAllow: (allow: boolean) => void
  onDonate: (amount: number, allow: boolean) => void
  onClose: () => void
  id?: string
  title?: string
}

const close = require('./assets/close')

export default class Tip extends React.PureComponent<Props, {}> {
  static defaultProps = {
    title: ''
  }

  onDonate = (amount: number) => {
    if (this.props.onDonate) {
      this.props.onDonate(amount, this.props.allow)
    }
  }

  onToggle = () => {
    if (this.props.onAllow) {
      this.props.onAllow(!this.props.allow)
    }
  }

  onAmountChange = (tokens: number) => {
    if (this.props.onAmountSelection) {
      this.props.onAmountSelection(tokens)
    }
  }

  render () {
    const { id, title, balance, donationAmounts, allow, onClose, provider, currentAmount } = this.props

    return (
      <StyledWrapper id={id}>
        <StyledClose onClick={onClose}>{close}</StyledClose>
        <StyledTitle>Send my tip to</StyledTitle>
        <Donate
          title={title || ''}
          actionText={getLocale('sendTip')}
          balance={balance}
          donationAmounts={donationAmounts}
          onAmountSelection={this.onAmountChange}
          onDonate={this.onDonate}
          donateType={'small'}
          currentAmount={currentAmount}
        >
          <StyledTipWrapper>
            <StyledAllowText>{getLocale('allowTip')} {provider}</StyledAllowText>
            <StyledAllowToggle>
              <Toggle
                onToggle={this.onToggle}
                checked={allow}
                size={'small'}
                type={'light'}
              />
            </StyledAllowToggle>
          </StyledTipWrapper>
        </Donate>
      </StyledWrapper>
    )
  }
}
