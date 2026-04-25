// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import styled from 'styled-components'

// Common components
import {
  LearnMoreLink,
  LearnMoreText,
  SettingsText } from '../../../components/default'
import Toggle from '@brave/leo/react/toggle'
import Flex from '$web-common/Flex'

// Utilities
import { getLocale } from '../../../../common/locale'
import { loadTimeData } from '$web-common/loadTimeData'
import { color } from '@brave/leo/tokens/css/variables'

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
  background-color: ${color.container.background};
`

const ControlsContainer = styled.span`
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-left: 24px;
`

const Container = styled.div`
  background-color: ${color.systemfeedback.infoBackground};
  border-radius: 8px;
  padding: 24px 24px;
  align-items: center;
  display: flex;
  flex-direction: column;
  margin-top: 8px;
`

const DescriptionRow = styled.div`
  width: 100%;
  color: ${color.text.primary};
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
  background: ${color.button.background};
  border-radius: 1000px;
  border-width: 0;
  color: white;
  cursor: pointer;
  padding: 10px 20px;
  font-weight: 600;
  font-size: 12px;
  line-height: 16px;
  align-self: start;
`

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
        <Flex direction="column">
          <SettingsText>{getLocale('brandedWallpaperOptIn')}</SettingsText>
          <LearnMoreText>
            {
              checked && rewardsEnabled && isExternalWalletConnected
                ? getLocale('sponsoredImageEarningDescription')
                : getLocale('sponsoredImageCanEarnDescription')
            }
            {' '}
            <LearnMoreLink
              href={loadTimeData.getString('newTabTakeoverLearnMoreLinkUrl')}
              target='_blank'
              rel='noopener noreferrer'>
              {getLocale('sponsoredImageLearnMore')}
            </LearnMoreLink>
          </LearnMoreText>
        </Flex>
        <ControlsContainer>
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
