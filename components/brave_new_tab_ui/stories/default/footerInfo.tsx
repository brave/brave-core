/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

// Feature-specific components
import { Link, Navigation, IconLink, PhotoName } from '../../components/default'

// Icons
import { SettingsAdvancedIcon, BookmarkBook, HistoryIcon, SettingsIcon } from 'brave-ui/components/icons'

// Helpers
import { getLocale } from '../fakeLocale'

interface Props {
  backgroundImageInfo: any
  onClickSettings: () => void
  isSettingsMenuOpen: boolean
  showPhotoInfo: boolean
}

export default class FooterInfo extends React.PureComponent<Props, {}> {
  render () {
    const {
      backgroundImageInfo,
      onClickSettings,
      isSettingsMenuOpen,
      showPhotoInfo
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
          <IconLink onClick={onClickSettings} disabled={isSettingsMenuOpen}><SettingsIcon /></IconLink>
          <IconLink><SettingsAdvancedIcon /></IconLink>
          <IconLink><BookmarkBook /></IconLink>
          <IconLink><HistoryIcon /></IconLink>
        </Navigation>
      </>
    )
  }
}
