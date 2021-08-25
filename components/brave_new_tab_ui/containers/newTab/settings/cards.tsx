// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

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
import binanceBanner from './assets/binance.png'
import rewardsBanner from './assets/braverewards.png'
import geminiBanner from './assets/gemini.png'
import cryptoDotComBanner from './assets/crypto-dot-com.png'
import ftxPreviewImageUrl from './assets/ftx-preview.png'
import HideIcon from './assets/hide-icon'
import { Toggle } from '../../../components/toggle'
import { PlusIcon } from 'brave-ui/components/icons'

import { getLocale } from '../../../../common/locale'

interface Props {
  toggleShowBinance: () => void
  showBinance: boolean
  binanceSupported: boolean
  toggleShowBraveTalk: () => void
  showBraveTalk: boolean
  braveTalkSupported: boolean
  toggleShowRewards: () => void
  showRewards: boolean
  toggleShowGemini: () => void
  geminiSupported: boolean
  showGemini: boolean
  toggleShowCryptoDotCom: () => void
  showCryptoDotCom: boolean
  cryptoDotComSupported: boolean
  toggleShowFTX: () => void
  showFTX: boolean
  ftxSupported: boolean
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
      binanceSupported,
      toggleShowBinance,
      showBinance,
      toggleShowBraveTalk,
      showBraveTalk,
      braveTalkSupported,
      toggleShowRewards,
      showRewards,
      geminiSupported,
      toggleShowGemini,
      showGemini,
      cryptoDotComSupported,
      toggleShowCryptoDotCom,
      showCryptoDotCom,
      ftxSupported,
      toggleShowFTX,
      showFTX,
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
          geminiSupported
          ? <SettingsWidget>
              <StyledBannerImage src={geminiBanner} />
              <StyledSettingsInfo>
                <StyledSettingsTitle>
                  {'Gemini'}
                </StyledSettingsTitle>
                <StyledSettingsCopy>
                  {getLocale('geminiWidgetDesc')}
                </StyledSettingsCopy>
              </StyledSettingsInfo>
              {this.renderToggleButton(showGemini, toggleShowGemini, false)}
            </SettingsWidget>
          : null
        }
        {
          binanceSupported
          ? <SettingsWidget>
              <StyledBannerImage src={binanceBanner} />
              <StyledSettingsInfo>
                <StyledSettingsTitle>
                  {'Binance'}
                </StyledSettingsTitle>
                <StyledSettingsCopy>
                  {getLocale('binanceWidgetDesc')}
                </StyledSettingsCopy>
              </StyledSettingsInfo>
              {this.renderToggleButton(showBinance, toggleShowBinance, false)}
            </SettingsWidget>
          : null
        }
        {
          cryptoDotComSupported
          ? <SettingsWidget>
              <StyledBannerImage src={cryptoDotComBanner} />
              <StyledSettingsInfo>
                <StyledSettingsTitle>
                  {'Crypto.com'}
                </StyledSettingsTitle>
                <StyledSettingsCopy>
                  {getLocale('cryptoDotComWidgetDesc')}
                </StyledSettingsCopy>
              </StyledSettingsInfo>
              {this.renderToggleButton(showCryptoDotCom, toggleShowCryptoDotCom, false)}
            </SettingsWidget>
          : null
        }
        {
        ftxSupported &&
        <SettingsWidget>
          <StyledBannerImage src={ftxPreviewImageUrl} />
          <StyledSettingsInfo>
            <StyledSettingsTitle>
              FTX
            </StyledSettingsTitle>
            <StyledSettingsCopy>
              {getLocale('ftxWidgetDescription')}
            </StyledSettingsCopy>
          </StyledSettingsInfo>
          {this.renderToggleButton(showFTX, toggleShowFTX, false)}
        </SettingsWidget>
        }
        <SettingsWidget>
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
                size={'large'}
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
