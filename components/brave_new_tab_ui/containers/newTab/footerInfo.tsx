/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

// Feature-specific components
import { Link, Navigation, IconLink, PhotoName } from '../../components/default'
import * as S from '../../components/default/page'

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
  toggleShowBinance: () => void
  toggleBrandedWallpaperOptIn: () => void
  showBackgroundImage: boolean
  showClock: boolean
  showStats: boolean
  showTopSites: boolean
  showRewards: boolean
  showBinance: boolean
  brandedWallpaperOptIn: boolean
  allowSponsoredWallpaperUI: boolean
  binanceSupported: boolean
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
      toggleBrandedWallpaperOptIn,
      showBackgroundImage,
      showClock,
      showStats,
      showTopSites,
      brandedWallpaperOptIn,
      allowSponsoredWallpaperUI,
      toggleShowRewards,
      showRewards,
      toggleShowBinance,
      showBinance,
      binanceSupported
    } = this.props

    return (
      <>
        { showPhotoInfo && backgroundImageInfo &&
          <S.GridItemCredits>
            <PhotoName>
              {`${getLocale('photoBy')} `}
              <Link href={backgroundImageInfo.link} rel='noreferrer noopener' target='_blank'>
                {backgroundImageInfo.author}
              </Link>
            </PhotoName>
          </S.GridItemCredits>
        }
        <S.GridItemNavigation>
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
              toggleBrandedWallpaperOptIn={toggleBrandedWallpaperOptIn}
              showBackgroundImage={showBackgroundImage}
              showClock={showClock}
              showStats={showStats}
              showTopSites={showTopSites}
              brandedWallpaperOptIn={brandedWallpaperOptIn}
              allowSponsoredWallpaperUI={allowSponsoredWallpaperUI}
              toggleShowRewards={toggleShowRewards}
              showRewards={showRewards}
              toggleShowBinance={toggleShowBinance}
              showBinance={showBinance}
              binanceSupported={binanceSupported}
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
        </S.GridItemNavigation>
      </>
    )
  }
}
