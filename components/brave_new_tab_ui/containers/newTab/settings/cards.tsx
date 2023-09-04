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
  StyledWidgetToggle,
  SettingsWidget,
  StyledAddButtonIcon,
  StyledHideButtonIcon,
  StyledWidgetSettings,
  StyledButtonLabel,
  ToggleCardsWrapper,
  ToggleCardsTitle,
  ToggleCardsCopy,
  ToggleCardsSwitch,
  ToggleCardsText
} from '../../../components/default'
import braveTalkBanner from './assets/brave-talk.png'
import rewardsBanner from './assets/braverewards.png'
import HideIcon from './assets/hide-icon'
import Toggle from '@brave/leo/react/toggle'
import { PlusIcon } from 'brave-ui/components/icons'

import { getLocale } from '../../../../common/locale'

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

class CardsSettings extends React.PureComponent<Props, {}> {
  renderToggleButton = (on: boolean, toggleFunc: any, float: boolean = true) => {
    const ButtonContainer = on ? StyledHideButtonIcon : StyledAddButtonIcon
    const ButtonIcon = on ? HideIcon : PlusIcon

    return (
      <StyledWidgetToggle
        isAdd={!on}
        float={float}
        onClick={toggleFunc}
      >
        <ButtonContainer>
          <ButtonIcon />
        </ButtonContainer>
        <StyledButtonLabel>
          {
            on
            ? getLocale('hideWidget')
            : getLocale('addWidget')
          }
        </StyledButtonLabel>
      </StyledWidgetToggle>
    )
  }

  render () {
    const {
      toggleShowBraveTalk,
      showBraveTalk,
      braveTalkSupported,
      toggleShowRewards,
      showRewards,
      braveRewardsSupported,
      toggleCards,
      cardsHidden
    } = this.props
    return (
      <StyledWidgetSettings>
        {
          braveTalkSupported
          ? <FeaturedSettingsWidget>
              <StyledBannerImage src={braveTalkBanner} />
              <StyledSettingsInfo>
                <StyledSettingsTitle>
                  {getLocale('braveTalkWidgetTitle')}
                </StyledSettingsTitle>
                <StyledSettingsCopy>
                  {getLocale('braveTalkWidgetWelcomeTitle')}
                </StyledSettingsCopy>
              </StyledSettingsInfo>
              {this.renderToggleButton(showBraveTalk, toggleShowBraveTalk)}
            </FeaturedSettingsWidget>
          : null
        }
        {
          braveRewardsSupported
            ? <SettingsWidget>
              <StyledBannerImage src={rewardsBanner} />
              <StyledSettingsInfo>
                <StyledSettingsTitle>
                  {getLocale('braveRewardsTitle')}
                </StyledSettingsTitle>
                <StyledSettingsCopy>
                  {getLocale('rewardsWidgetDesc')}
                </StyledSettingsCopy>
              </StyledSettingsInfo>
              {this.renderToggleButton(showRewards, toggleShowRewards, false)}
            </SettingsWidget>
            : null
        }
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
                onChange={toggleCards.bind(this, cardsHidden)}
                checked={!cardsHidden}
              />
            </ToggleCardsSwitch>
          </ToggleCardsWrapper>
        </FeaturedSettingsWidget>
      </StyledWidgetSettings>
    )
  }
}

export default CardsSettings
