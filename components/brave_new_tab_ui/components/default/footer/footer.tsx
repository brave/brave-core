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
  supportsTogether: boolean
  togetherPromptDismissed: boolean
  backgroundImageInfo: any
  showPhotoInfo: boolean
  onClickSettings: () => any
  onDismissTogetherPrompt: () => any
}

function TogetherItem (props: Props) {
  if (!props.togetherPromptDismissed) {
    return (
      <TogetherTooltip onClose={props.onDismissTogetherPrompt}>
        <IconLink title={getLocale('togetherPromptTitle')} href='https://talk.brave.com/widget'>
          <TogetherIcon />
        </IconLink>
      </TogetherTooltip>
    )
  }

  return (
    <IconLink title={getLocale('togetherPromptTitle')} href='https://talk.brave.com/widget'>
      <TogetherIcon />
    </IconLink>
  )
}

export default class FooterInfo extends React.PureComponent<Props, {}> {
  render () {
    const {
      textDirection,
      supportsTogether,
      backgroundImageInfo,
      showPhotoInfo,
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
            <IconLink title={getLocale('preferencesPageTitle')} href='chrome://settings'>
              <SettingsAdvancedIcon />
            </IconLink>
            <IconLink title={getLocale('bookmarksPageTitle')} href='chrome://bookmarks'>
              <BookmarkBook />
            </IconLink>
            <IconLink title={getLocale('historyPageTitle')} href='chrome://history'>
              <HistoryIcon />
            </IconLink>
            {supportsTogether &&
              <TogetherItem {...this.props} />
            }
          </Navigation>
        </S.GridItemNavigation>
      </>
    )
  }
}
