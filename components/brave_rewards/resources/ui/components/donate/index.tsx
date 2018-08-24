/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import {
  StyledWrapper,
  StyledContent,
  StyledDonationTitle,
  StyledSend,
  StyledIconSend,
  StyledIconFace,
  StyledFunds,
  StyledFundsText
} from './style'

import Amount from '../amount/index'
import { getLocale } from '../../../helpers'
import { EmoteSadIcon } from '../../../components/icons'

const send = require('./assets/send')

export type DonateType = 'big' | 'small'

type Donation = {
  tokens: number,
  converted: number
}

export interface Props {
  actionText: string
  title: string
  balance: number
  donationAmounts: Donation[]
  currentAmount: number
  onDonate: (amount: number) => void
  onAmountSelection?: (tokens: number) => void
  id?: string
  donateType: DonateType
  children?: React.ReactNode
}

interface State {
  missingFunds: boolean
}

export default class Donate extends React.PureComponent<Props, State> {
  constructor (props: Props) {
    super(props)
    this.state = {
      missingFunds: false
    }
  }

  componentDidUpdate (prevProps: Props) {
    if (
      this.props.balance !== prevProps.balance ||
      this.props.donationAmounts !== prevProps.donationAmounts ||
      this.props.currentAmount !== prevProps.currentAmount
    ) {
      this.validateAmount(this.props.balance)
    }
  }

  validateDonation = () => {
    if (this.validateAmount(this.props.balance)) {
      return
    }

    if (this.props.onDonate) {
      this.props.onDonate(this.props.currentAmount)
    }
  }

  validateAmount (balance: number, tokens?: number) {
    if (tokens === undefined) {
      tokens = this.props.currentAmount
    }

    const valid = tokens > balance
    this.setState({ missingFunds: valid })
    return valid
  }

  onAmountChange = (tokens: number) => {
    this.validateAmount(this.props.balance, tokens)

    if (this.props.onAmountSelection) {
      this.props.onAmountSelection(tokens)
    }
  }

  render () {
    const { id, donationAmounts, actionText, children, title, currentAmount, donateType } = this.props
    const disabled = currentAmount === 0

    const sendColor = disabled ?
      donateType === 'small'
      ? '#1A22A8'
      : '#3e45b2'
    : '#a1a8f2'

    return (
      <StyledWrapper donateType={donateType} disabled={disabled}>
        <StyledContent id={id} >
          <StyledDonationTitle>{title}</StyledDonationTitle>
          {
            donationAmounts && donationAmounts.map((donation: Donation) => {
              return <div key={`${id}-donate-${donation.tokens}`}>
                <Amount
                  amount={donation.tokens}
                  selected={donation.tokens === currentAmount}
                  onSelect={this.onAmountChange}
                  converted={donation.converted}
                  type={donateType}
                />
              </div>
            })
          }
          {children}
        </StyledContent>
        <StyledSend onClick={this.validateDonation}>
          <StyledIconSend>{send(sendColor)}</StyledIconSend>{actionText}
        </StyledSend>
        {
          this.state.missingFunds
            ? <StyledFunds>
              <StyledIconFace>
                <EmoteSadIcon />
              </StyledIconFace>
              <StyledFundsText>{getLocale('notEnoughTokens')} <a href='#'>{getLocale('addFunds')}</a>.</StyledFundsText>
            </StyledFunds>
            : null
        }
      </StyledWrapper>
    )
  }
}
