import * as React from 'react'
import { getLocale } from 'brave-ui/helpers'

import {
  StyledWrapper,
  StyledTitle,
  StyledInfo,
  StyledInfoItem,
  StyledSubTitle,
  StyledTweet,
  StyledTwitterIcon
} from './style'
import { LogoTwitterIcon } from 'brave-ui/components/icons'

interface Props {
  amount?: string
  monthlyDate?: string
  isMonthly: boolean
  onTweet: () => void
  onlyAnonWallet?: boolean
}

export default class DonateConfirmation extends React.PureComponent<Props, {}> {
  getDonateInfo = () => {
    const {
      amount,
      monthlyDate,
      isMonthly,
      onlyAnonWallet
    } = this.props

    const batString = getLocale(onlyAnonWallet ? 'bap' : 'bat')
    const typeString = getLocale(isMonthly ? 'contributionAmount' : 'donationAmount')

    return (
      <>
        <StyledInfoItem>
          {typeString}: {amount} {batString}
        </StyledInfoItem>
        {
          isMonthly && monthlyDate
          ? <StyledInfoItem>
              {getLocale('contributionNextDate')}: {monthlyDate}
            </StyledInfoItem>
          : null
        }
      </>
    )
  }

  render () {
    const { isMonthly, onTweet } = this.props

    return (
      <StyledWrapper isMonthly={isMonthly}>
        <StyledTitle>
          {getLocale(isMonthly ? 'monthlySet' : 'tipSent')} 🎉
        </StyledTitle>
        <StyledInfo>
          {this.getDonateInfo()}
        </StyledInfo>
        <StyledSubTitle>
          {getLocale('shareText')}
        </StyledSubTitle>
        <StyledTweet onClick={onTweet}>
          <StyledTwitterIcon>
            <LogoTwitterIcon />
          </StyledTwitterIcon>
        </StyledTweet>
      </StyledWrapper>
    )
  }
}
