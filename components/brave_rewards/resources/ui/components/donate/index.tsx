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
import { getLocale } from 'brave-ui/helpers'
import { EmoteSadIcon, SendIcon } from 'brave-ui/components/icons'

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
  isMobile?: boolean
  addFundsLink?: string
  onlyAnonWallet?: boolean
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

  generateMissingFunds = () => {
    const {
      addFundsLink,
      onlyAnonWallet
    } = this.props

    let link = undefined
    let locale = 'notEnoughTokens'
    if (!onlyAnonWallet) {
      link = (<a href={addFundsLink} target={'_blank'}>{getLocale('addFunds')}</a>)
      locale = 'notEnoughTokensLink'
    }

    const tokenString = onlyAnonWallet ? 'points' : 'tokens'
    return (
      <StyledFunds>
        <StyledIconFace>
          <EmoteSadIcon />
        </StyledIconFace>
        <StyledFundsText>
          {getLocale(locale, { currency: getLocale(tokenString) })} {link}
        </StyledFundsText>
      </StyledFunds>)
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
      isMobile,
      onlyAnonWallet
    } = this.props
    const disabled = parseInt(currentAmount, 10) === 0

    return (
      <StyledWrapper donateType={donateType} disabled={disabled} isMobile={isMobile}>
        <StyledContent id={id} isMobile={isMobile}>
          <StyledDonationTitle isMobile={isMobile}>{title}</StyledDonationTitle>
            <StyledAmountsWrapper isMobile={isMobile}>
              {
                donationAmounts && donationAmounts.map((donation: Donation) => {
                  return <div key={`${id}-tip-${donation.tokens}`}>
                    <Amount
                      isMobile={isMobile}
                      amount={donation.tokens}
                      selected={donation.tokens === currentAmount.toString()}
                      onSelect={this.onAmountChange}
                      converted={donation.converted}
                      type={donateType}
                      onlyAnonWallet={onlyAnonWallet}
                    />
                  </div>
                })
              }
            </StyledAmountsWrapper>
          {children}
        </StyledContent>

        <StyledSend onClick={this.validateDonation} data-test-id={'send-tip-button'}>
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
            ? this.generateMissingFunds()
            : null
        }
      </StyledWrapper>
    )
  }
}
