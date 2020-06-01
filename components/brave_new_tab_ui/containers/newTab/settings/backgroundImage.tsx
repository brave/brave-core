// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'

import {
  SettingsRow,
  SettingsText
} from '../../../components/default'
import { Toggle } from '../../../components/toggle'

import { getLocale } from '../../../../common/locale'

interface Props {
  toggleBrandedWallpaperOptIn: () => void
  toggleShowBackgroundImage: () => void
  brandedWallpaperOptIn: boolean
  showBackgroundImage: boolean
}

class BackgroundImageSettings extends React.PureComponent<Props, {}> {
  render () {
    const {
      toggleShowBackgroundImage,
      toggleBrandedWallpaperOptIn,
      brandedWallpaperOptIn,
      showBackgroundImage
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
      </div>
    )
  }
}

export default BackgroundImageSettings
