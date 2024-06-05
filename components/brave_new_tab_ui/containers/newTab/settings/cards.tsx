// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

import {
  FeaturedSettingsWidget,
  StyledBannerImage,
  StyledSettingsInfo,
  StyledSettingsTitle,
  StyledSettingsCopy,
  SettingsWidget,
  StyledWidgetSettings,
  ToggleCardsWrapper,
  ToggleCardsTitle,
  ToggleCardsCopy,
  ToggleCardsSwitch,
  ToggleCardsText
} from '../../../components/default'
import braveTalkBanner from './assets/brave-talk.png'
import rewardsBanner from './assets/braverewards.png'
import Toggle from '@brave/leo/react/toggle'
import Button from '@brave/leo/react/button'

import { getLocale } from '$web-common/locale'
import Icon from '@brave/leo/react/icon'
import styled, { css } from 'styled-components'
import { spacing } from '@brave/leo/tokens/css/variables'

const StyledButton = styled(Button) <{ float: boolean }>`
  margin-top: ${spacing.xl};
  width: fit-content;
  ${p => p.float && css`
    float: right;
    margin-right: ${spacing.m};
  `}
`

interface Props {
  toggleShowBraveTalk: () => void
  showBraveTalk: boolean
  braveTalkSupported: boolean
  toggleShowRewards: () => void
  showRewards: boolean
  braveRewardsSupported: boolean
  toggleCards: (show: boolean) => void
  cardsHidden: boolean
}

const ToggleButton = ({ on, toggleFunc, float }: { on: boolean, toggleFunc: any, float?: boolean }) => {
  return <StyledButton onClick={toggleFunc} kind={on ? 'outline' : 'filled'} float={!!float}>
    <div slot="icon-before">
      <Icon name={on ? 'eye-off' : 'plus-add'} />
    </div>
    {getLocale(on ? 'hideWidget' : 'addWidget')}
  </StyledButton>
}

function CardSettings({ toggleShowBraveTalk, showBraveTalk, braveTalkSupported, toggleShowRewards, showRewards, braveRewardsSupported, toggleCards, cardsHidden }: Props) {
  return <StyledWidgetSettings>
    {braveTalkSupported && <FeaturedSettingsWidget>
      <StyledBannerImage src={braveTalkBanner} />
      <StyledSettingsInfo>
        <StyledSettingsTitle>
          {getLocale('braveTalkWidgetTitle')}
        </StyledSettingsTitle>
        <StyledSettingsCopy>
          {getLocale('braveTalkWidgetWelcomeTitle')}
        </StyledSettingsCopy>
      </StyledSettingsInfo>
      <ToggleButton on={showBraveTalk} toggleFunc={toggleShowBraveTalk} float />
    </FeaturedSettingsWidget>}
    {braveRewardsSupported && <SettingsWidget>
      <StyledBannerImage src={rewardsBanner} />
      <StyledSettingsInfo>
        <StyledSettingsTitle>
          {getLocale('braveRewardsTitle')}
        </StyledSettingsTitle>
        <StyledSettingsCopy>
          {getLocale('rewardsWidgetDesc')}
        </StyledSettingsCopy>
      </StyledSettingsInfo>
      <ToggleButton on={showRewards} toggleFunc={toggleShowRewards} />
    </SettingsWidget>}
    <FeaturedSettingsWidget>
      <ToggleCardsWrapper>
        <ToggleCardsText>
          <ToggleCardsTitle>
            {getLocale('cardsToggleTitle')}
          </ToggleCardsTitle>
          <ToggleCardsCopy>
            {getLocale('cardsToggleDesc')}
          </ToggleCardsCopy>
        </ToggleCardsText>
        <ToggleCardsSwitch>
          <Toggle
            size='small'
            onChange={() => toggleCards(cardsHidden)}
            checked={!cardsHidden}
          />
        </ToggleCardsSwitch>
      </ToggleCardsWrapper>
    </FeaturedSettingsWidget>
  </StyledWidgetSettings>
}

export default React.memo(CardSettings)
