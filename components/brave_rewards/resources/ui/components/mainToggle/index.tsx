/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import {
  MainToggleWrapper,
  StyledTitle,
  ToggleHeading,
  StyledTOSWrapper,
  StyledServiceText,
  StyledServiceLink,
  StyledLogotypeWrapper,
  StyledLogoWrapper
} from './style'
import Toggle from 'brave-ui/components/formControls/toggle/index'
import { getLocale } from 'brave-ui/helpers'
import { BatColorIcon } from 'brave-ui/components/icons'

export interface Props {
  enabled: boolean
  onToggle: () => void
  id?: string
  testId?: string
  onTOSClick?: () => void
  onPrivacyClick?: () => void
}

export default class MainToggle extends React.PureComponent<Props, {}> {
  render () {
    const {
      id,
      enabled,
      onToggle,
      testId,
      onTOSClick,
      onPrivacyClick
    } = this.props

    return (
      <MainToggleWrapper id={id}>
        <ToggleHeading>
          <StyledLogotypeWrapper>
            <StyledLogoWrapper>
              <BatColorIcon />
            </StyledLogoWrapper>
            <StyledTitle>
              {getLocale('braveRewards')}
            </StyledTitle>
          </StyledLogotypeWrapper>
          <Toggle checked={enabled} onToggle={onToggle} testId={testId} />
        </ToggleHeading>
        {
          !enabled
            ? <StyledTOSWrapper>
              <StyledServiceText>
                {getLocale('serviceTextToggle')} <StyledServiceLink onClick={onTOSClick}>{getLocale('termsOfService')}</StyledServiceLink> {getLocale('and')} <StyledServiceLink onClick={onPrivacyClick}>{getLocale('privacyPolicy')}</StyledServiceLink>.
              </StyledServiceText>
            </StyledTOSWrapper>
            : null
        }
      </MainToggleWrapper>
    )
  }
}
