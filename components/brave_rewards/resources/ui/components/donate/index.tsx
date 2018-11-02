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
  StyledFundsText,
  StyledAmountsWrapper,
  StyledSendButton,
  StyledButtonWrapper
} from './style'

import Amount from '../amount/index'
import { getLocale } from '../../../helpers'
import { EmoteSadIcon, SendIcon } from '../../../components/icons'

export type DonateType = 'big' | 'small'

type Donation = {
  tokens: string,
  converted: string
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
  isMobile?: boolean
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
    const { id, donationAmounts, actionText, children, title, currentAmount, donateType, isMobile } = this.props
    const disabled = currentAmount === 0

    return (
      <StyledWrapper donateType={donateType} disabled={disabled} isMobile={isMobile}>
        <StyledContent id={id} isMobile={isMobile}>
          <StyledDonationTitle isMobile={isMobile}>{title}</StyledDonationTitle>
            <StyledAmountsWrapper isMobile={isMobile}>
              {
                donationAmounts && donationAmounts.map((donation: Donation) => {
                  return <div key={`${id}-donate-${donation.tokens}`}>
                    <Amount
                      isMobile={isMobile}
                      amount={donation.tokens}
                      selected={donation.tokens === currentAmount.toString()}
                      onSelect={this.onAmountChange}
                      converted={donation.converted}
                      type={donateType}
                    />
                  </div>
                })
              }
            </StyledAmountsWrapper>
          {children}
        </StyledContent>

        <StyledSend onClick={this.validateDonation}>
          <StyledButtonWrapper isMobile={isMobile}>
            <StyledSendButton>
              <StyledIconSend disabled={disabled} donateType={donateType}>
                <SendIcon />
              </StyledIconSend>{actionText}
            </StyledSendButton>
          </StyledButtonWrapper>
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
