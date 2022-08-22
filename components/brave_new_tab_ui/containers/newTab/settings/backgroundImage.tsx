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
  StyledCustomBackgroundOptionSolidColor,
  StyledCustomBackgroundSettings,
  StyledSelectionBorder,
  StyledUploadIconContainer,
  StyledUploadLabel
} from '../../../components/default'
import braveBackground from './assets/brave-background.png'
import UploadIcon from './assets/upload-icon'
import { Toggle } from '../../../components/toggle'

import { getLocale } from '../../../../common/locale'

import SolidColorChooser from './solidColorChooser'
import { defaultSolidBackgroundColor } from '../../../data/colors'

interface Props {
  newTabData: NewTab.State
  toggleBrandedWallpaperOptIn: () => void
  toggleShowBackgroundImage: () => void
  useCustomBackgroundImage: (useCustom: boolean) => void
  useSolidColorBackground: (color: string) => void
  brandedWallpaperOptIn: boolean
  showBackgroundImage: boolean
  featureCustomBackgroundEnabled: boolean
}

enum Location {
  LIST,
  SOLID_COLORS
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
    const selectedSolidBackgroundColor = newTabData.backgroundWallpaper?.type === 'solidColor' ? newTabData.backgroundWallpaper.wallpaperSolidColor : undefined 
    const usingBraveBackground = !usingCustomBackground && !selectedSolidBackgroundColor

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
                  <StyledSelectionBorder selected={!!selectedSolidBackgroundColor}>
                    <StyledCustomBackgroundOptionSolidColor
                      color={selectedSolidBackgroundColor ?? defaultSolidBackgroundColor }
                      selected={!!selectedSolidBackgroundColor}
                    />
                  </StyledSelectionBorder>
                  <StyledCustomBackgroundOptionLabel>
                    {getLocale('solidColorTitle')}
                  </StyledCustomBackgroundOptionLabel>
                </StyledCustomBackgroundOption>
              </StyledCustomBackgroundSettings>
            )}
          </div>
        )}
        {this.state.location === Location.SOLID_COLORS &&
          <SolidColorChooser
            currentColor={selectedSolidBackgroundColor}
            useSolidColorBackground={this.props.useSolidColorBackground}
            onBack={() => this.setLocation(Location.LIST)}
          />
        }
      </>
    )
  }
}

export default BackgroundImageSettings
