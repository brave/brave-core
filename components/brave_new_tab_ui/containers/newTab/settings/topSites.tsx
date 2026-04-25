// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

import {
  SettingsRow,
  SettingsText,
  StyledTopSitesCustomizationSettings,
  StyledTopSitesCustomizationSettingsOption,
  StyledTopSitesCustomizationImage,
  StyledTopSitesCustomizationOptionTitle,
  StyledTopSitesCustomizationOptionDesc
} from '../../../components/default'
import Toggle from '@brave/leo/react/toggle'

import { getLocale } from '$web-common/locale'

interface Props {
  toggleShowTopSites: () => void
  showTopSites: boolean
  customLinksEnabled: boolean
  setMostVisitedSettings: (show: boolean, customize: boolean) => void
}

function TopSitesSettings({ toggleShowTopSites, showTopSites, customLinksEnabled, setMostVisitedSettings }: Props) {
  const favoritesSelected = showTopSites && customLinksEnabled
  const frecencySelected = showTopSites && !customLinksEnabled
  return <div>
    <SettingsRow>
      <SettingsText>{getLocale('showTopSites')}</SettingsText>
      <Toggle
        onChange={toggleShowTopSites}
        checked={showTopSites}
        size='small' />
    </SettingsRow>
    <StyledTopSitesCustomizationSettings>
      <StyledTopSitesCustomizationSettingsOption
        onClick={() => setMostVisitedSettings(true, true)}>
        <StyledTopSitesCustomizationImage
          isFavorites={true}
          selected={favoritesSelected} />
        <StyledTopSitesCustomizationOptionTitle>
          {getLocale('showFavoritesLabel')}
        </StyledTopSitesCustomizationOptionTitle>
        <StyledTopSitesCustomizationOptionDesc>
          {getLocale('showFavoritesDesc')}
        </StyledTopSitesCustomizationOptionDesc>
      </StyledTopSitesCustomizationSettingsOption>
      <StyledTopSitesCustomizationSettingsOption
        onClick={() => setMostVisitedSettings(true, false)}>
        <StyledTopSitesCustomizationImage
          isFavorites={false}
          selected={frecencySelected} />
        <StyledTopSitesCustomizationOptionTitle>
          {getLocale('showFrecencyLabel')}
        </StyledTopSitesCustomizationOptionTitle>
        <StyledTopSitesCustomizationOptionDesc>
          {getLocale('showFrecencyDesc')}
        </StyledTopSitesCustomizationOptionDesc>
      </StyledTopSitesCustomizationSettingsOption>
    </StyledTopSitesCustomizationSettings>
  </div>
}

export default React.memo(TopSitesSettings)
