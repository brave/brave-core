/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

// Feature-specific components
import { Link, Navigation, IconLink, PhotoName } from '../../components/default'

// Icons
import { SettingsAdvancedIcon, BookmarkBook, HistoryIcon } from 'brave-ui/components/icons'

// Helpers
import { getLocale } from '../../../common/locale'
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
  toggleShowRewards: () => void
  showBackgroundImage: boolean
  showClock: boolean
  showStats: boolean
  showTopSites: boolean
  showRewards: boolean
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
      showTopSites,
      toggleShowRewards,
      showRewards
    } = this.props

    return (
      <>
        <div>
          {showPhotoInfo && backgroundImageInfo &&
          <PhotoName>
            {`${getLocale('photoBy')} `}
            <Link href={backgroundImageInfo.link} rel='noreferrer noopener' target='_blank'>
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
            toggleShowRewards={toggleShowRewards}
            showRewards={showRewards}
          />
          <IconLink title={getLocale('preferencesPageTitle')} href='chrome://settings'>
            <SettingsAdvancedIcon />
          </IconLink>
          <IconLink title={getLocale('bookmarksPageTitle')} href='chrome://bookmarks'>
            <BookmarkBook />
          </IconLink>
          <IconLink title={getLocale('historyPageTitle')} href='chrome://history'>
            <HistoryIcon />
          </IconLink>
        </Navigation>
      </>
    )
  }
}
