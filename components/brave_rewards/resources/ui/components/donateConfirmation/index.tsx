import * as React from 'react'
import { getLocale } from 'brave-ui/helpers'

import {
  StyledWrapper,
  StyledTitle,
  StyledSubTitle,
  StyledTweet,
  StyledTwitterIcon
} from './style'
import { LogoTwitterIcon } from 'brave-ui/components/icons'

interface Props {
  isMonthly: boolean
  onTweet: () => void
}

export default class DonateConfirmation extends React.PureComponent<Props, {}> {

  render () {
    const { isMonthly, onTweet } = this.props

    return (
      <StyledWrapper isMonthly={isMonthly}>
        <StyledTitle>
          {getLocale('tipSent')} 🎉
        </StyledTitle>
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
