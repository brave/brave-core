// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'

import {
  SettingsRow,
  SettingsText,
  StyledCustomBackgroundSettings,
  StyledCustomBackgroundOption,
  StyledCustomBackgroundOptionImage,
  StyledCustomBackgroundOptionLabel,
  StyledUploadIconContainer,
  StyledUploadLabel
} from '../../../components/default'
import braveBackground from './assets/brave-background.png'
import UploadIcon from './assets/upload-icon'
import { Toggle } from '../../../components/toggle'

import { getLocale } from '../../../../common/locale'

interface Props {
  toggleBrandedWallpaperOptIn: () => void
  toggleShowBackgroundImage: () => void
  useCustomBackgroundImage: (useCustom: boolean) => void
  brandedWallpaperOptIn: boolean
  showBackgroundImage: boolean
  featureCustomBackgroundEnabled: boolean
}

class BackgroundImageSettings extends React.PureComponent<Props, {}> {
  onClickCustomBackground = () => {
    this.props.useCustomBackgroundImage(true)
  }

  onClickBraveBackground = () => {
    this.props.useCustomBackgroundImage(false)
  }

  render () {
    const {
      toggleShowBackgroundImage,
      toggleBrandedWallpaperOptIn,
      brandedWallpaperOptIn,
      showBackgroundImage,
      featureCustomBackgroundEnabled
    } = this.props

    return (
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
        {
          showBackgroundImage &&
          featureCustomBackgroundEnabled &&
          <StyledCustomBackgroundSettings>
            <StyledCustomBackgroundOption
              onClick={this.onClickCustomBackground}
            >
              <StyledUploadIconContainer>
                <UploadIcon />
                <StyledUploadLabel>
                  {getLocale('customBackgroundImageOptionUploadLabel')}
                </StyledUploadLabel>
              </StyledUploadIconContainer>
              <StyledCustomBackgroundOptionLabel>
                {getLocale('customBackgroundImageOptionTitle')}
              </StyledCustomBackgroundOptionLabel>
            </StyledCustomBackgroundOption>
            <StyledCustomBackgroundOption
              onClick={this.onClickBraveBackground}
            >
              <StyledCustomBackgroundOptionImage src={braveBackground} />
              <StyledCustomBackgroundOptionLabel>
                {getLocale('braveBackgroundImageOptionTitle')}
              </StyledCustomBackgroundOptionLabel>
            </StyledCustomBackgroundOption>
          </StyledCustomBackgroundSettings>
        }
      </div>
    )
  }
}

export default BackgroundImageSettings
