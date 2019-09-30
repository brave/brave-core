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
  StyledButtonWrapper,
  StyledContributionWrapper,
  StyledContributionText,
  StyledMonthlySendButton
} from './style'

import Amount from '../amount/index'
import { getLocale } from 'brave-ui/helpers'
import { EmoteSadIcon, SendIcon } from 'brave-ui/components/icons'
import { BannerType } from '../siteBanner'

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
  currentAmount: string
  onDonate: (amount: string) => void
  onAmountSelection?: (tokens: string) => void
  id?: string
  donateType: DonateType
  children?: React.ReactNode
  addFundsLink?: string
  type: BannerType
  nextContribution?: string
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

  validateAmount (balance: number, tokens?: string) {
    if (tokens === undefined) {
      tokens = this.props.currentAmount
    }

    const valid = parseInt(tokens, 10) > balance
    this.setState({ missingFunds: valid })
    return valid
  }

  onAmountChange = (tokens: string) => {
    this.validateAmount(this.props.balance, tokens)

    if (this.props.onAmountSelection) {
      this.props.onAmountSelection(tokens)
    }
  }

  render () {
    const {
      id,
      donationAmounts,
      actionText,
      children,
      title,
      currentAmount,
      donateType,
      addFundsLink,
      type,
      nextContribution
    } = this.props

    const isMonthly = type === 'monthly'
    const disabled = parseInt(currentAmount, 10) === 0
    const SendButton = isMonthly ? StyledMonthlySendButton : StyledSendButton

    return (
      <StyledWrapper donateType={donateType} disabled={disabled}>
        <StyledContent id={id}>
          <StyledDonationTitle>{title}</StyledDonationTitle>
            <StyledAmountsWrapper>
              {
                donationAmounts && donationAmounts.map((donation: Donation) => {
                  return <div key={`${id}-tip-${donation.tokens}`}>
                    <Amount
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

        <StyledSend onClick={this.validateDonation} data-test-id={'send-tip-button'} monthly={isMonthly}>
          <StyledButtonWrapper>
            <SendButton>
              <StyledIconSend disabled={disabled} donateType={donateType} monthly={isMonthly}>
                <SendIcon />
              </StyledIconSend>{actionText}
            </SendButton>
            {
              nextContribution && isMonthly
              ? <StyledContributionWrapper>
                  <StyledContributionText>
                    {getLocale('nextContribution')}
                  </StyledContributionText>
                  <StyledContributionText>
                    {nextContribution}
                  </StyledContributionText>
                </StyledContributionWrapper>
              : null
            }
          </StyledButtonWrapper>
        </StyledSend>
        {
          this.state.missingFunds
            ? <StyledFunds>
              <StyledIconFace>
                <EmoteSadIcon />
              </StyledIconFace>
              <StyledFundsText>
                {getLocale('notEnoughTokens')} <a href={addFundsLink} target={'_blank'}>{getLocale('addFunds')}</a>.
              </StyledFundsText>
            </StyledFunds>
            : null
        }
      </StyledWrapper>
    )
  }
}
