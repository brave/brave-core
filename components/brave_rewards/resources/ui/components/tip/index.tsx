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
import Donate from '../donate'
import Toggle from '../../../components/formControls/toggle'
import { getLocale } from '../../../helpers'
import { CloseCircleOIcon } from '../../../components/icons'

type Donation = { tokens: string, converted: string, selected?: boolean }

export interface Props {
  allow: boolean
  provider: string
  balance: string
  currentAmount: string
  donationAmounts: Donation[]
  onAmountSelection?: (tokens: string) => void
  onAllow: (allow: boolean) => void
  onDonate: (amount: string, allow: boolean) => void
  onClose: () => void
  id?: string
  title?: string
  addFundsLink?: string
}

export default class Tip extends React.PureComponent<Props, {}> {
  static defaultProps = {
    title: ''
  }

  onDonate = (amount: string) => {
    if (this.props.onDonate) {
      this.props.onDonate(amount, this.props.allow)
    }
  }

  onToggle = () => {
    if (this.props.onAllow) {
      this.props.onAllow(!this.props.allow)
    }
  }

  onAmountChange = (tokens: string) => {
    if (this.props.onAmountSelection) {
      this.props.onAmountSelection(tokens)
    }
  }

  render () {
    const { id, title, balance, donationAmounts, allow, onClose, provider, currentAmount, addFundsLink } = this.props

    return (
      <StyledWrapper id={id}>
        <StyledClose onClick={onClose}>
          <CloseCircleOIcon />
        </StyledClose>
        <StyledTitle>Send my tip to</StyledTitle>
        <Donate
          title={title || ''}
          actionText={getLocale('sendTip')}
          balance={parseFloat(balance)}
          donationAmounts={donationAmounts}
          onAmountSelection={this.onAmountChange}
          onDonate={this.onDonate}
          donateType={'small'}
          currentAmount={currentAmount}
          addFundsLink={addFundsLink}
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
