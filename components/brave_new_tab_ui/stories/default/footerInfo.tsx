/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

// Feature-specific components
import { Link, Navigation, IconLink, PhotoName } from '../../components/default'

// Icons
import { SettingsAdvancedIcon, BookmarkBook, HistoryIcon } from 'brave-ui/components/icons'

// Helpers
import { getLocale } from '../fakeLocale'
import Settings from './settings'

interface Props {
  textDirection: string
  onClickOutside: () => void
  backgroundImageInfo: any
  onClickSettings: () => void
  showSettingsMenu: boolean
  showPhotoInfo: boolean
  toggleShowBackgroundImage: () => void
  toggleShowClock: () => void
  toggleShowStats: () => void
  toggleShowTopSites: () => void
  showBackgroundImage: boolean
  showClock: boolean
  showStats: boolean
  showTopSites: boolean
}

export default class FooterInfo extends React.PureComponent<Props, {}> {

  render () {
    const {
      textDirection,
      backgroundImageInfo,
      onClickSettings,
      showSettingsMenu,
      showPhotoInfo,
      onClickOutside,
      toggleShowBackgroundImage,
      toggleShowClock,
      toggleShowStats,
      toggleShowTopSites,
      showBackgroundImage,
      showClock,
      showStats,
      showTopSites
    } = this.props

    return (
      <>
        <div>
          {showPhotoInfo &&
          <PhotoName>
            {`${getLocale('photoBy')} `}
            <Link href={backgroundImageInfo.link} rel='noopener' target='_blank'>
              {backgroundImageInfo.author}
            </Link>
          </PhotoName>}
        </div>
        <Navigation>
          <Settings
            textDirection={textDirection}
            showSettingsMenu={showSettingsMenu}
            onClickOutside={onClickOutside}
            onClick={onClickSettings}
            toggleShowBackgroundImage={toggleShowBackgroundImage}
            toggleShowClock={toggleShowClock}
            toggleShowStats={toggleShowStats}
            toggleShowTopSites={toggleShowTopSites}
            showBackgroundImage={showBackgroundImage}
            showClock={showClock}
            showStats={showStats}
            showTopSites={showTopSites}
          />
          <IconLink><SettingsAdvancedIcon /></IconLink>
          <IconLink><BookmarkBook /></IconLink>
          <IconLink><HistoryIcon /></IconLink>
        </Navigation>
      </>
    )
  }
}
