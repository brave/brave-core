// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import styled from 'styled-components'

// Common components
import { LearnMoreLink, LearnMoreText, SettingsText } from '../../../components/default'
import Icon from '@brave/leo/react/icon'
import Toggle from '@brave/leo/react/toggle'
import Tooltip from '@brave/leo/react/tooltip'

// Utilities
import { getLocale } from '../../../../common/locale'

const sponsoredImagesLearnMoreLink = 'https://support.brave.com/hc/en-us/articles/35182999599501'

interface Props {
  onChange: () => void
  onEnableRewards: () => void

  checked: boolean
  disabled: boolean
  rewardsEnabled: boolean
  isExternalWalletConnected: boolean
}

const ToggleRow = styled.div`
  width: 100%;
  display: flex;
  align-items: center;
  justify-content: space-between;
  padding: 8px 0px;
  background-color: var(--background1);
`

const VerticalContainer = styled.div`
  display: flex;
  flex-direction: column;
`

const ControlsContainer = styled.span`
  display: flex;
  justify-content: space-between;
  align-items: center;
`

const InfoTooltip = styled(Tooltip)`
  --leo-tooltip-padding: 0px;
  --leo-tooltip-background: var(--background1);
  --leo-tooltip-shadow: 0px 0px 24px rgba(99, 105, 110, 0.36);
  margin-right: 18px;

  > div {
    width: 264px;
    border-radius: 6px;
    padding: 24px;
  }

  div {
    letter-spacing: 0.01em;
    font-family: var(--brave-font-family-non-serif);
  }
}
`

const InfoIcon = styled(Icon)`
  --leo-icon-size: 17px;
`

const InfoIconTooltipTitle = styled.div`
  font-weight: 600;
  font-size: 14px;
  line-height: 24px;
  margin-bottom: 16px;
`

const InfoIconTooltipBody = styled.div`
  font-weight: 400;
  font-size: 12px;
  line-height: 18px;
`

const Container = styled.div`
  background-color: var(--info-background);
  border-radius: 8px;
  padding: 24px 24px;
  align-items: center;
  display: flex;
  flex-direction: column;
  margin-top: 8px;
`

const DescriptionRow = styled.div`
  width: 100%;
  color: var(--text1);
  font-family: var(--brave-font-family-non-serif);
  letter-spacing: 0.01em;
  margin-bottom: 16px;
`

const DescriptionTitle = styled.div`
  font-weight: 600;
  font-size: 14px;
  line-height: 24px;
  margin-bottom: 4px;
`

const DescriptionBody = styled.div`
  font-weight: 400;
  font-size: 12px;
  line-height: 18px;
`

const EnableRewardsButton = styled.button`
  height: 36px;
  background: var(--interactive5);
  border-radius: 1000px;
  border-width: 0;
  color: white;
  cursor: pointer;
  padding: 10px 20px;
  font-family: var(--brave-font-family-non-serif);
  font-weight: 600;
  font-size: 12px;
  line-height: 16px;
  align-self: start;
`

function getInfoTooltipText (checked: boolean, rewardsEnabled: boolean) {
  if (!checked) {
    return rewardsEnabled
      ? getLocale('sponsoredImageOffRewardsOnDescription')
      : getLocale('sponsoredImageRewardsOffDescription')
  }

  return getLocale('sponsoredImageOnDescription')
}

function getDescriptionText (rewardsEnabled: boolean) {
  if (!rewardsEnabled) {
    return getLocale('sponsoredImageRewardsOffDescription')
  }

  return getLocale('sponsoredImageOnRewardsOnNoCustodianDescription')
}

function getButtonText (rewardsEnabled: boolean) {
  if (!rewardsEnabled) {
    return getLocale('sponsoredImageEnableRewards')
  }

  return getLocale('braveRewardsTitle')
}

export default function SponsoredImageToggle (
  {
    onChange, onEnableRewards, checked, disabled, rewardsEnabled,
    isExternalWalletConnected
  }: Props) {
  // Info icon is shown when:
  // 1. SI toggle is off
  // 2. Rewards is enabled (with a custodian connected)
  const showInfoIcon =
    !disabled &&
    (!checked || (rewardsEnabled && isExternalWalletConnected))

  // Description is shown when SI toggle is on and:
  // 1. Rewards is not enabled (to show the button to enable Rewards)
  // 2. Rewards custodian is not connected (to show the button to go to Rewards)
  const showDescription =
    !disabled &&
    checked &&
    (!rewardsEnabled || !isExternalWalletConnected)

  return (
    <div>
      <ToggleRow>
        <VerticalContainer>
          <SettingsText>{getLocale('brandedWallpaperOptIn')}</SettingsText>
          <LearnMoreText>
            {getLocale('sponsoredImageOptInDescription')}
            {' '}
            <LearnMoreLink href={sponsoredImagesLearnMoreLink}>
              {getLocale('sponsoredImageOptInLearnMore')}
            </LearnMoreLink>
          </LearnMoreText>
        </VerticalContainer>
        <ControlsContainer>
          {showInfoIcon &&
            <InfoTooltip mode='default' positionStrategy='fixed' tabIndex={0}>
              <InfoIcon name='info-outline' />
              <div slot="content">
                <InfoIconTooltipTitle>
                  {checked
                    ? getLocale('sponsoredImageEarningTitle')
                    : getLocale('sponsoredImageNotEarningTitle')}
                </InfoIconTooltipTitle>
                <InfoIconTooltipBody>
                  {getInfoTooltipText(checked, rewardsEnabled)}
                </InfoIconTooltipBody>
              </div>
            </InfoTooltip>
          }
          <Toggle onChange={onChange} checked={checked} disabled={disabled}
            size='small' />
        </ControlsContainer>
      </ToggleRow>
      {showDescription &&
        <Container>
          <DescriptionRow>
            <DescriptionTitle>
              {getLocale('sponsoredImageNotEarningTitle')}
            </DescriptionTitle>
            <DescriptionBody>
              {getDescriptionText(rewardsEnabled)}
            </DescriptionBody>
          </DescriptionRow>
          <EnableRewardsButton onClick={onEnableRewards} title=''>
            {getButtonText(rewardsEnabled)}
          </EnableRewardsButton>
        </Container>
      }
    </div>
  )
}
