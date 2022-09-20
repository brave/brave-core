// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'
import styled from 'styled-components'

// Common components
import { SettingsText } from '../../../components/default'
import { Toggle } from '../../../components/toggle'

// Assets
import InfoIcon from '../../../components/default/settings/assets/info-icon.svg'

// Utilities
import { getLocale } from '../../../../common/locale'

interface Props {
  onChange: () => void
  onEnableRewards: () => void

  checked: boolean
  disabled: boolean
  rewardsEnabled: boolean
  adsEnabled: boolean
  canSupportAds: boolean
}

const Container = styled.div`
  background-color: var(--info-background);
  border-radius: 8px;
  padding: 8px;
  align-items: center;
  display: flex;
  flex-direction: column;
  gap: 10px;
`

const ToggleRow = styled.div`
  width: 100%;
  display: flex;
  align-items: center;
  justify-content: space-between;
  padding: 12px 14px;
  border-radius: 10px;
  background-color: var(--background1);
`

const DescriptionRow = styled.div`
  width: 100%;
  display: grid; 
  grid-template-rows: 16px 1fr;
  grid-template-columns: 16px 1fr;
  padding: 0 8px;
  gap: 8px;
  color: #339AF0;
  font-family: var(--brave-font-family-non-serif);
  letter-spacing: 0.01em;
  margin-bottom: 7px;
`

const DescriptionIcon = styled.div`
  background: url(${InfoIcon}) no-repeat;
`

const DescriptionTitle = styled.div`
  font-weight: 600;
  font-size: 12px;
  line-height: 18px;
`

const DescriptionBody = styled.div`
  grid-column: 2;
  font-weight: 500;
  font-size: 11px;
  line-height: 17px;
`

const EnableRewardsButton = styled.button`
  width: 236px;
  height: 40px;
  background: var(--interactive5);
  border-radius: 48px;
  border-width: 0;
  color: white;
  cursor: pointer;
  margin-bottom: 8px;
`

export default function SponsoredImageToggle ({ onChange, onEnableRewards, checked, disabled, rewardsEnabled: rewardEnabled, adsEnabled, canSupportAds }: Props) {
  const showRewardButton = checked && !disabled && (!rewardEnabled || (!adsEnabled && canSupportAds))
  return (
    <Container>
      <ToggleRow>
        <SettingsText>{getLocale('brandedWallpaperOptIn')}</SettingsText>
        <Toggle onChange={onChange} checked={checked} disabled={disabled} size='small' />
      </ToggleRow>
      <DescriptionRow>
        <DescriptionIcon/>
        <DescriptionTitle>
          {!checked || showRewardButton ? getLocale('sponsoredImageOn') : getLocale('sponsoredImageOff')}
        </DescriptionTitle>
        <DescriptionBody>
          {!checked || showRewardButton ? getLocale('sponsoredImageOnDescription') : getLocale('sponsoredImageOffDescription')}
        </DescriptionBody>
      </DescriptionRow>
      {showRewardButton &&
        <EnableRewardsButton onClick={onEnableRewards}>
            {getLocale('rewardsStartUsingRewards')}
        </EnableRewardsButton>}
    </Container>
  )
}
