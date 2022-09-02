// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'

import {
  SettingsRow,
  SettingsText,
  StyledCustomBackgroundOption,
  StyledCustomBackgroundOptionImage,
  StyledCustomBackgroundOptionLabel,
  StyledCustomBackgroundOptionColor,
  StyledCustomBackgroundSettings,
  StyledSelectionBorder,
  StyledUploadIconContainer,
  StyledUploadLabel
} from '../../../components/default'
import braveBackground from './assets/brave-background.png'
import UploadIcon from './assets/upload-icon'
import { Toggle } from '../../../components/toggle'

import { getLocale } from '../../../../common/locale'

import BackgroundChooser from './backgroundChooser'
import { images, defaultSolidBackgroundColor, solidColorsForBackground, gradientColorsForBackground, defaultGradientColor } from '../../../data/backgrounds'
import SponsoredImageToggle from './sponsoredImagesToggle'

interface Props {
  newTabData: NewTab.State
  toggleBrandedWallpaperOptIn: () => void
  toggleShowBackgroundImage: () => void
  useCustomBackgroundImage: () => void
  setBraveBackground: (selectedBackground: string) => void
  setColorBackground: (color: string, useRandomColor: boolean) => void
  brandedWallpaperOptIn: boolean
  showBackgroundImage: boolean
  featureCustomBackgroundEnabled: boolean
  onEnableRewards: () => void
}

enum Location {
  LIST,
  BRAVE_BACKGROUNDS,
  SOLID_COLORS,
  GRADIENT_COLORS
}

interface State {
  location: Location
}

class BackgroundImageSettings extends React.PureComponent<Props, State> {
  constructor (props: Props) {
    super(props)
    this.state = {
      location: Location.LIST
    }
  }

  setLocation = (location: Location) => {
    this.setState({ location })
  }

  onClickCustomBackground = () => {
    this.props.useCustomBackgroundImage()
  }

  onClickBraveBackground = () => {
    this.setState({ location: Location.BRAVE_BACKGROUNDS })
  }

  onClickSolidColorBackground = () => {
    this.setState({ location: Location.SOLID_COLORS })
  }

  onClickGradientColorBackground = () => {
    this.setState({ location: Location.GRADIENT_COLORS })
  }

  render () {
    const {
      newTabData,
      toggleShowBackgroundImage,
      toggleBrandedWallpaperOptIn,
      brandedWallpaperOptIn,
      showBackgroundImage,
      featureCustomBackgroundEnabled,
      onEnableRewards
    } = this.props

    const usingCustomImageBackground = newTabData.backgroundWallpaper?.type === 'image'
    const selectedBackgroundColor = newTabData.backgroundWallpaper?.type === 'color' ? newTabData.backgroundWallpaper.wallpaperColor : undefined
    const usingBraveBackground = !usingCustomImageBackground && !selectedBackgroundColor
    const usingSolidColorBackground = !!selectedBackgroundColor && !!solidColorsForBackground.find(element => element.wallpaperColor === selectedBackgroundColor)
    const usingGradientBackground = !!selectedBackgroundColor && !!gradientColorsForBackground.find(element => element.wallpaperColor === selectedBackgroundColor)

    const usingRandomColor = newTabData.backgroundWallpaper?.type === 'color' && !!newTabData.backgroundWallpaper?.random
    const usingRandomBraveBackground = newTabData.backgroundWallpaper?.type === 'brave' && !!newTabData.backgroundWallpaper.random
    const selectedBraveBackground = newTabData.backgroundWallpaper?.type === 'brave' ? newTabData.backgroundWallpaper.wallpaperImageUrl : undefined

    return (
      <>
        {this.state.location === Location.LIST && (
          <div>
            <SettingsRow>
              <SettingsText>{getLocale('showBackgroundImage')}</SettingsText>
              <Toggle
                onChange={toggleShowBackgroundImage}
                checked={showBackgroundImage}
                size='large'
              />
            </SettingsRow>
            {showBackgroundImage && featureCustomBackgroundEnabled && (
              <StyledCustomBackgroundSettings>
                <StyledCustomBackgroundOption
                  onClick={this.onClickCustomBackground}
                >
                  <StyledSelectionBorder selected={usingCustomImageBackground}>
                    <StyledUploadIconContainer selected={usingCustomImageBackground}>
                      <UploadIcon />
                      <StyledUploadLabel>
                        {getLocale('customBackgroundImageOptionUploadLabel')}
                      </StyledUploadLabel>
                    </StyledUploadIconContainer>
                  </StyledSelectionBorder>
                  <StyledCustomBackgroundOptionLabel>
                    {getLocale('customBackgroundImageOptionTitle')}
                  </StyledCustomBackgroundOptionLabel>
                </StyledCustomBackgroundOption>
                <StyledCustomBackgroundOption
                  onClick={this.onClickBraveBackground}
                >
                  <StyledSelectionBorder selected={usingBraveBackground}>
                    <StyledCustomBackgroundOptionImage image={braveBackground} selected={usingBraveBackground}/>
                  </StyledSelectionBorder>
                  <StyledCustomBackgroundOptionLabel>
                    {getLocale('braveBackgroundImageOptionTitle')}
                  </StyledCustomBackgroundOptionLabel>
                </StyledCustomBackgroundOption>
                <StyledCustomBackgroundOption
                  onClick={this.onClickSolidColorBackground}
                >
                  <StyledSelectionBorder selected={usingSolidColorBackground}>
                    <StyledCustomBackgroundOptionColor
                      colorValue={usingSolidColorBackground ? selectedBackgroundColor : defaultSolidBackgroundColor }
                      selected={usingSolidColorBackground}
                    />
                  </StyledSelectionBorder>
                  <StyledCustomBackgroundOptionLabel>
                    {getLocale('solidColorTitle')}
                  </StyledCustomBackgroundOptionLabel>
                </StyledCustomBackgroundOption>
                <StyledCustomBackgroundOption
                  onClick={this.onClickGradientColorBackground}
                >
                  <StyledSelectionBorder selected={usingGradientBackground}>
                    <StyledCustomBackgroundOptionColor
                      colorValue={usingGradientBackground ? selectedBackgroundColor : defaultGradientColor}
                      selected={usingGradientBackground}
                    />
                  </StyledSelectionBorder>
                  <StyledCustomBackgroundOptionLabel>
                    {getLocale('gradientColorTitle')}
                  </StyledCustomBackgroundOptionLabel>
                </StyledCustomBackgroundOption>
              </StyledCustomBackgroundSettings>
            )}
            <div style={{ height: '16px' }}/>
            <SettingsRow>
              <SponsoredImageToggle
                onChange={toggleBrandedWallpaperOptIn}
                onEnableRewards={onEnableRewards}
                checked={showBackgroundImage && brandedWallpaperOptIn}
                disabled={!showBackgroundImage /* This option can only be enabled if users opt in for background images */}
                rewardEnabled={this.props.newTabData.rewardsState.rewardsEnabled}
                adsEnabled={this.props.newTabData.rewardsState.enabledAds}
                canSupportAds={!!this.props.newTabData.rewardsState.adsSupported}/>
            </SettingsRow>
          </div>
        )}
        {this.state.location === Location.BRAVE_BACKGROUNDS &&
          <BackgroundChooser
            title={getLocale('braveBackgroundsTitle')}
            backgrounds={images}
            currentValue={selectedBraveBackground}
            usingRandomColor={usingRandomBraveBackground}
            onToggleRandomColor={on => this.props.setBraveBackground('')}
            onSelectValue={ value => this.props.setBraveBackground(value)}
            onBack={() => this.setLocation(Location.LIST)}
          />
        }
        {this.state.location === Location.SOLID_COLORS &&
          <BackgroundChooser
            title={getLocale('solidColorTitle')}
            backgrounds={solidColorsForBackground}
            currentValue={selectedBackgroundColor}
            usingRandomColor={usingSolidColorBackground && usingRandomColor}
            onToggleRandomColor={on => this.props.setColorBackground(on ? 'solid' : (selectedBackgroundColor ?? defaultSolidBackgroundColor), on)}
            onSelectValue={this.props.setColorBackground}
            onBack={() => this.setLocation(Location.LIST)}
          />
        }
        {this.state.location === Location.GRADIENT_COLORS &&
          <BackgroundChooser
            title={getLocale('gradientColorTitle')}
            backgrounds={gradientColorsForBackground}
            currentValue={selectedBackgroundColor}
            usingRandomColor={usingGradientBackground && usingRandomColor}
            onToggleRandomColor={on => this.props.setColorBackground(on ? 'gradient' : (selectedBackgroundColor ?? defaultGradientColor), on)}
            onSelectValue={this.props.setColorBackground}
            onBack={() => this.setLocation(Location.LIST)}
          />
        }
      </>
    )
  }
}

export default BackgroundImageSettings
