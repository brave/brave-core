// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.*/

import * as React from 'react'

import {
  SettingsRow,
  SettingsText,
  StyledTopSitesCustomizationSettings,
  StyledTopSitesCustomizationSettingsOption,
  StyledTopSitesCustomizationImageBorder,
  StyledTopSitesCustomizationImage,
  StyledTopSitesCustomizationOptionTitle,
  StyledTopSitesCustomizationOptionDesc
} from '../../../components/default'
import { Toggle } from '../../../components/toggle'

import { getLocale } from '../../../../common/locale'

interface Props {
  toggleShowTopSites: () => void
  showTopSites: boolean
  customLinksEnabled: boolean
  setMostVisitedSettings: (show: boolean, customize: boolean) => void
}

class TopSitesSettings extends React.PureComponent<Props, {}> {
  onClickFavorites = () => {
    this.props.setMostVisitedSettings(true, true)
  }

  onClickFrecency = () => {
    this.props.setMostVisitedSettings(true, false)
  }

  render () {
    const {
      toggleShowTopSites,
      showTopSites,
      customLinksEnabled
    } = this.props

    const favoritesSelected = showTopSites && customLinksEnabled
    const frecencySelected = showTopSites && !customLinksEnabled
    return (
      <div>
        <SettingsRow>
          <SettingsText>{getLocale('showTopSites')}</SettingsText>
          <Toggle
            onChange={toggleShowTopSites}
            checked={showTopSites}
            size='large'
          />
        </SettingsRow>
        <StyledTopSitesCustomizationSettings>
          <StyledTopSitesCustomizationSettingsOption
            onClick={this.onClickFavorites}
          >
            <StyledTopSitesCustomizationImageBorder
              selected={favoritesSelected}
            >
              <StyledTopSitesCustomizationImage
                isFavorites={true}
                selected={favoritesSelected}
              />
            </StyledTopSitesCustomizationImageBorder>
            <StyledTopSitesCustomizationOptionTitle>
              {getLocale('showFavoritesLabel')}
            </StyledTopSitesCustomizationOptionTitle>
            <StyledTopSitesCustomizationOptionDesc>
              {getLocale('showFavoritesDesc')}
            </StyledTopSitesCustomizationOptionDesc>
          </StyledTopSitesCustomizationSettingsOption>
          <StyledTopSitesCustomizationSettingsOption
            onClick={this.onClickFrecency}
          >
            <StyledTopSitesCustomizationImageBorder
              selected={frecencySelected}
            >
              <StyledTopSitesCustomizationImage
                isFavorites={false}
                selected={frecencySelected}
              />
            </StyledTopSitesCustomizationImageBorder>
            <StyledTopSitesCustomizationOptionTitle>
              {getLocale('showFrecencyLabel')}
            </StyledTopSitesCustomizationOptionTitle>
            <StyledTopSitesCustomizationOptionDesc>
              {getLocale('showFrecencyDesc')}
            </StyledTopSitesCustomizationOptionDesc>
          </StyledTopSitesCustomizationSettingsOption>
        </StyledTopSitesCustomizationSettings>
      </div>
    )
  }
}

export default TopSitesSettings
