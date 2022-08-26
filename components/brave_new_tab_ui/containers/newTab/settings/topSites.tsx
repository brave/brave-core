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
import { useNewTabPref } from '../../../hooks/usePref'

interface Props {
  customLinksEnabled: boolean
  setMostVisitedSettings: (show: boolean, customize: boolean) => void
}

export default function TopSitesSettings ({ customLinksEnabled, setMostVisitedSettings }: Props) {
    const [showTopSites, setShowTopSites] = useNewTabPref('showTopSites')
    const favoritesSelected = showTopSites && customLinksEnabled
    const frecencySelected = showTopSites && !customLinksEnabled

    return <div>
      <SettingsRow>
        <SettingsText>{getLocale('showTopSites')}</SettingsText>
        <Toggle
          onChange={() => setShowTopSites(!showTopSites)}
          checked={showTopSites}
          size='large'
        />
      </SettingsRow>
      <StyledTopSitesCustomizationSettings>
        <StyledTopSitesCustomizationSettingsOption
          onClick={() => setMostVisitedSettings(true, true)}
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
          onClick={() => setMostVisitedSettings(true, false)}>

          <StyledTopSitesCustomizationImageBorder selected={frecencySelected}>
            <StyledTopSitesCustomizationImage
              isFavorites={false}
              selected={frecencySelected} />
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
}
