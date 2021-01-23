/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

// Feature-specific components
import {
  Label,
  Link,
  Navigation,
  IconButton,
  IconButtonContainer,
  IconButtonSideText,
  IconLink,
  PhotoName
} from '..'
import * as S from '../page'

// Items
import {
  SettingsIcon,
  SettingsAdvancedIcon,
  BookmarkBook,
  HistoryIcon
} from 'brave-ui/components/icons'
import TogetherTooltip from './togetherTooltip'
import TogetherIcon from './togetherTooltip/togetherIcon'

// Helpers
import { getLocale } from '../../../../common/locale'

interface Props {
  textDirection: string
  togetherPrmoptDismissed: boolean
  backgroundImageInfo: any
  showPhotoInfo: boolean
  onClickSettings: () => any
  onDismissTogetherPrompt: () => any
}

export default class FooterInfo extends React.PureComponent<Props, {}> {
  render () {
    const {
      textDirection,
      togetherPrmoptDismissed,
      backgroundImageInfo,
      showPhotoInfo,
      onDismissTogetherPrompt,
      onClickSettings
    } = this.props

    return (
      <>
        { showPhotoInfo && backgroundImageInfo &&
          <S.GridItemCredits>
            <PhotoName>
              {`${getLocale('photoBy')} `}
              { backgroundImageInfo.link
                  ? <Link href={backgroundImageInfo.link} rel='noreferrer noopener' target='_blank'>
                      {backgroundImageInfo.author}
                    </Link>
                  : <Label> {backgroundImageInfo.author} </Label>
              }
            </PhotoName>
          </S.GridItemCredits>
        }
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
            <IconLink title={getLocale('preferencesPageTitle')} href='brave://settings'>
              <SettingsAdvancedIcon />
            </IconLink>
            <IconLink title={getLocale('bookmarksPageTitle')} href='brave://bookmarks'>
              <BookmarkBook />
            </IconLink>
            <IconLink title={getLocale('historyPageTitle')} href='brave://history'>
              <HistoryIcon />
            </IconLink>
            { !togetherPrmoptDismissed
              ? <TogetherTooltip onClose={onDismissTogetherPrompt}>
                  <IconLink title={getLocale('togetherPageTitle')} href='https://together.brave.com/widget'>
                    <TogetherIcon />
                  </IconLink>
                </TogetherTooltip>
              : <IconLink title={getLocale('togetherPageTitle')} href='https://together.brave.com/widget'>
                  <TogetherIcon />
                </IconLink>
            }
          </Navigation>
        </S.GridItemNavigation>
      </>
    )
  }
}
