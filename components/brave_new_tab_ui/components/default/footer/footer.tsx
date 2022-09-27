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
import BraveTalkItem from './braveTalkItem'

// Helpers
import { getLocale } from '../../../../common/locale'
import { DismissBraveTalkPromptPayload } from '../../../actions/new_tab_actions'

export type OnDismissBraveTalkPrompt = (payload: DismissBraveTalkPromptPayload) => unknown
export interface Props {
  textDirection: string
  supportsBraveTalk: boolean
  showBraveTalkPrompt: boolean
  backgroundImageInfo: NewTab.BackgroundWallpaper | undefined
  showPhotoInfo: boolean
  onClickSettings: () => any
  onDismissBraveTalkPrompt: OnDismissBraveTalkPrompt
  color: string
}

export default class FooterInfo extends React.PureComponent<Props, {}> {
  render () {
    const {
      textDirection,
      supportsBraveTalk,
      backgroundImageInfo,
      showPhotoInfo,
      onClickSettings,
      color
    } = this.props

    return (
      <>
        { showPhotoInfo && backgroundImageInfo?.type === 'brave' &&
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
            <IconButtonContainer textDirection={textDirection} color={color}>
              <IconButtonSideText textDirection={textDirection}>
                <IconButton onClick={onClickSettings} color={color}>
                  <SettingsIcon />
                </IconButton>
                {getLocale('customize')}
              </IconButtonSideText>
            </IconButtonContainer>
            <IconLink title={getLocale('preferencesPageTitle')} href='chrome://settings' color={color}>
              <SettingsAdvancedIcon />
            </IconLink>
            <IconLink title={getLocale('bookmarksPageTitle')} href='chrome://bookmarks' color={color}>
              <BookmarkBook />
            </IconLink>
            <IconLink title={getLocale('historyPageTitle')} href='chrome://history' color={color}>
              <HistoryIcon />
            </IconLink>
            {supportsBraveTalk &&
              <BraveTalkItem {...this.props} />
            }
          </Navigation>
        </S.GridItemNavigation>
      </>
    )
  }
}
