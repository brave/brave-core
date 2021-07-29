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
import BraveTalkTooltip from './braveTalkTooltip'
import BraveTalkIcon from './braveTalkTooltip/braveTalkIcon'

// Helpers
import { getLocale } from '../../../../common/locale'

interface Props {
  textDirection: string
  supportsBraveTalk: boolean
  braveTalkPromptDismissed: boolean
  backgroundImageInfo: any
  showPhotoInfo: boolean
  onClickSettings: () => any
  onDismissBraveTalkPrompt: () => any
}

function BraveTalkItem (props: Props) {
  if (!props.braveTalkPromptDismissed) {
    return (
      <BraveTalkTooltip onClose={props.onDismissBraveTalkPrompt}>
        <IconLink title={getLocale('braveTalkPromptTitle')} href='https://talk.brave.com/widget'>
          <BraveTalkIcon />
        </IconLink>
      </BraveTalkTooltip>
    )
  }

  return (
    <IconLink title={getLocale('braveTalkPromptTitle')} href='https://talk.brave.com/widget'>
      <BraveTalkIcon />
    </IconLink>
  )
}

export default class FooterInfo extends React.PureComponent<Props, {}> {
  render () {
    const {
      textDirection,
      supportsBraveTalk,
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
            {supportsBraveTalk &&
              <BraveTalkItem {...this.props} />
            }
          </Navigation>
        </S.GridItemNavigation>
      </>
    )
  }
}
