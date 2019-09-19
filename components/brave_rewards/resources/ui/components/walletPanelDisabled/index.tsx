/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
*/

import * as React from 'react'

import {
  StyledWrapper,
  StyledBatLogo,
  StyledTitle,
  StyledSubTitle,
  StyledText,
  StyledServiceText,
  StyledArrowIcon,
  StyledButtonWrapper,
  StyledServiceLink
} from './style'
import { RewardsButton } from '../'
import { ArrowRightIcon, BatColorIcon } from 'brave-ui/components/icons'

import { getLocale } from 'brave-ui/helpers'

export interface Props {
  onEnable: () => void
  onTOSClick: () => void
  onPrivacyClick: () => void
}

export default class WalletPanelDisabled extends React.PureComponent<Props, {}> {

  render () {
    const { onEnable, onTOSClick, onPrivacyClick } = this.props

    return (
      <StyledWrapper>
        <StyledBatLogo>
          <BatColorIcon />
        </StyledBatLogo>
        <StyledTitle>
          {getLocale('welcomeBack')}
        </StyledTitle>
        <StyledSubTitle>
          {getLocale('braveRewardsSubTitle')}
        </StyledSubTitle>
        <StyledText>
          {getLocale('disabledPanelTextTwo')}
        </StyledText>
        <StyledButtonWrapper>
          <RewardsButton
            type={'tip'}
            testId={'optInAction'}
            onClick={onEnable}
            text={getLocale('enableRewards')}
            icon={<StyledArrowIcon><ArrowRightIcon /></StyledArrowIcon>}
          />
        </StyledButtonWrapper>
        <StyledServiceText>
          {getLocale('serviceText')} <StyledServiceLink onClick={onTOSClick}>{getLocale('termsOfService')}</StyledServiceLink> {getLocale('and')} <StyledServiceLink onClick={onPrivacyClick}>{getLocale('privacyPolicy')}</StyledServiceLink>.
        </StyledServiceText>
      </StyledWrapper>
    )
  }
}
