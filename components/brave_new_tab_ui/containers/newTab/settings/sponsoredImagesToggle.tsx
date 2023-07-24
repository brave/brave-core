// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import styled from 'styled-components'

// Common components
import { SettingsText } from '../../../components/default'
import Toggle from '@brave/leo/react/toggle'

// Assets
import { SponsoredImageInfoIcon } from './icons/sponsoredImageInfo'

// Utilities
import { getLocale } from '../../../../common/locale'

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

const ControlsContainer = styled.span`
  display: flex;
  justify-content: space-between;
  align-items: center;
`

const InfoIcon = styled.span`
  position: relative;
  margin: 0px 18px;

  > .icon {
    height: 17px;
    width: auto;
    vertical-align: middle;
  }

  .tooltip {
    position: absolute;
    bottom: 100%;
    inset-inline-start: -200px;
    width: 264px;
    padding-bottom: 12px;
    display: none;
  }

  &:hover .tooltip {
    display: initial;
  }

  &:focus .tooltip {
    display: initial;
  }

  &:focus {
    outline-style: solid;
    outline-color: ${p => p.theme.color.brandBrave};
    outline-width: 1px;
  }
`

const InfoIconTooltip = styled.div`
  position: relative;
  background: var(--background1);
  box-shadow: 0px 0px 24px rgba(99, 105, 110, 0.36);
  border-radius: 6px;
  font-family: var(--brave-font-family-non-serif);
  letter-spacing: 0.01em;
  padding: 24px;

  &:before {
    content: '';
    position: absolute;
    bottom: -7px;
    inset-inline-start: 201px;
    background: inherit;
    height: 15px;
    width: 15px;
    transform: rotate(45deg);
  }
`

const InfoIconTooltipTitle = styled.div`
  font-weight: 600;
  font-size: 14px;
  line-height: 24px;
  margin-bottom: 8px;
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
        <SettingsText>{getLocale('brandedWallpaperOptIn')}</SettingsText>
        <ControlsContainer>
          {showInfoIcon &&
            <InfoIcon title='' tabIndex={0}>
              {SponsoredImageInfoIcon}
              <div className='tooltip'>
                <InfoIconTooltip>
                  <InfoIconTooltipTitle>
                    {checked
                      ? getLocale('sponsoredImageEarningTitle')
                      : getLocale('sponsoredImageNotEarningTitle')}
                  </InfoIconTooltipTitle>
                  <InfoIconTooltipBody>
                    {getInfoTooltipText(checked, rewardsEnabled)}
                  </InfoIconTooltipBody>
                </InfoIconTooltip>
              </div>
            </InfoIcon>
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
