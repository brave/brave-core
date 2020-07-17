/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

// Feature-specific components
import {
  Link,
  Navigation,
  IconButton,
  IconButtonContainer,
  IconButtonSideText,
  IconLink,
  PhotoName
} from '../../components/default'
import * as S from '../../components/default/page'

// Icons
import {
  SettingsIcon,
  SettingsAdvancedIcon,
  BookmarkBook,
  HistoryIcon
} from 'brave-ui/components/icons'

// Helpers
import { getLocale } from '../../../common/locale'

interface Props {
  textDirection: string
  onClickSettings: (event: React.MouseEvent<HTMLButtonElement>) => void
  backgroundImageInfo: any
  showPhotoInfo: boolean
}

export default class FooterInfo extends React.PureComponent<Props, {}> {
  render () {
    const {
      textDirection,
      onClickSettings,
      backgroundImageInfo,
      showPhotoInfo
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
        <S.GridItemNavigationBraveToday>
          ⬇🟢⬇🟢⬇🟢⬇🟢⬇🟢 scroll to see more ⬇🟢⬇🟢⬇🟢⬇🟢⬇🟢
        </S.GridItemNavigationBraveToday>
        <S.GridItemNavigation>
          <Navigation>
            <IconButtonContainer textDirection={textDirection}>
              <IconButtonSideText textDirection={textDirection}>
                <IconButton onClick={onClickSettings}>
                  <SettingsIcon />
                </IconButton>
                {getLocale('customize')}
              </IconButtonSideText>
            </IconButtonContainer>
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
