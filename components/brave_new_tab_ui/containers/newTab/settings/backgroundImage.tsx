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

import ColorChooser from './colorChooser'
import { defaultSolidBackgroundColor, solidColorsForBackground, gradientColorsForBackground, defaultGradientColor } from '../../../data/colors'

interface Props {
  newTabData: NewTab.State
  toggleBrandedWallpaperOptIn: () => void
  toggleShowBackgroundImage: () => void
  useCustomBackgroundImage: (useCustom: boolean) => void
  setColorBackground: (color: string) => void
  brandedWallpaperOptIn: boolean
  showBackgroundImage: boolean
  featureCustomBackgroundEnabled: boolean
}

enum Location {
  LIST,
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
    this.props.useCustomBackgroundImage(true)
  }

  onClickBraveBackground = () => {
    this.props.useCustomBackgroundImage(false)
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
      featureCustomBackgroundEnabled
    } = this.props

    const usingCustomBackground = newTabData.backgroundWallpaper?.type === 'image' &&
        !!newTabData.backgroundWallpaper.wallpaperImageUrl &&
        !newTabData.backgroundWallpaper.author && !newTabData.backgroundWallpaper.link
    const selectedBackgroundColor = newTabData.backgroundWallpaper?.type === 'color' ? newTabData.backgroundWallpaper.wallpaperColor : undefined
    const usingBraveBackground = !usingCustomBackground && !selectedBackgroundColor
    const usingSolidColorBackground = selectedBackgroundColor ? solidColorsForBackground.includes(selectedBackgroundColor) : false
    const usingGradientBackground = selectedBackgroundColor ? gradientColorsForBackground.includes(selectedBackgroundColor) : false

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
            <SettingsRow isChildSetting={true}>
              <SettingsText>{getLocale('brandedWallpaperOptIn')}</SettingsText>
              <Toggle
                onChange={toggleBrandedWallpaperOptIn}
                // This option can only be enabled if
                // users opt in for background images
                checked={showBackgroundImage && brandedWallpaperOptIn}
                disabled={!showBackgroundImage}
                size='small'
              />
            </SettingsRow>
            {showBackgroundImage && featureCustomBackgroundEnabled && (
              <StyledCustomBackgroundSettings>
                <StyledCustomBackgroundOption
                  onClick={this.onClickCustomBackground}
                >
                  <StyledSelectionBorder selected={usingCustomBackground}>
                    <StyledUploadIconContainer selected={usingCustomBackground}>
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
                      colorValue={usingSolidColorBackground ? selectedBackgroundColor as string : defaultSolidBackgroundColor }
                      selected={usingSolidColorBackground}
                    />
                  </StyledSelectionBorder>
                  <StyledCustomBackgroundOptionLabel>
                    {getLocale('solidColorTitle')}
                  </StyledCustomBackgroundOptionLabel>
                </StyledCustomBackgroundOption>
                <StyledCustomBackgroundOption
                  onClick={ this.onClickGradientColorBackground }
                >
                  <StyledSelectionBorder selected={usingGradientBackground}>
                    <StyledCustomBackgroundOptionColor
                      colorValue={usingGradientBackground ? selectedBackgroundColor as string : defaultGradientColor}
                      selected={usingGradientBackground}
                    />
                  </StyledSelectionBorder>
                  <StyledCustomBackgroundOptionLabel>
                    {getLocale('gradientColorTitle')}
                  </StyledCustomBackgroundOptionLabel>
                </StyledCustomBackgroundOption>
              </StyledCustomBackgroundSettings>
            )}
          </div>
        )}
        {this.state.location === Location.SOLID_COLORS &&
          <ColorChooser
            title={getLocale('solidColorTitle')}
            values={solidColorsForBackground}
            currentValue={selectedBackgroundColor}
            onSelectValue={this.props.setColorBackground}
            onBack={() => this.setLocation(Location.LIST)}
          />
        }
        {this.state.location === Location.GRADIENT_COLORS &&
          <ColorChooser
            title={getLocale('gradientColorTitle')}
            values={gradientColorsForBackground}
            currentValue={selectedBackgroundColor}
            onSelectValue={this.props.setColorBackground}
            onBack={() => this.setLocation(Location.LIST)}
          />
        }
      </>
    )
  }
}

export default BackgroundImageSettings
