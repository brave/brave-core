/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Icon from '@brave/leo/react/icon'

// Feature-specific components
import {
  Label,
  Link,
  Navigation,
  IconButton,
  IconButtonContainer,
  IconButtonSideText,
  PhotoName
} from '..'
import * as S from '../page'

// Helpers
import { getLocale } from '../../../../common/locale'

export interface Props {
  textDirection: string
  backgroundImageInfo: NewTab.BackgroundWallpaper | undefined
  showPhotoInfo: boolean
  onClickSettings: () => any
}

export default class FooterInfo extends React.PureComponent<Props, {}> {
  render () {
    const {
      textDirection,
      backgroundImageInfo,
      showPhotoInfo,
      onClickSettings
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
            <IconButtonContainer textDirection={textDirection}>
              <IconButtonSideText textDirection={textDirection}>
                <IconButton onClick={onClickSettings}>
                  <Icon name='tune' />
                </IconButton>
                {getLocale('customize')}
              </IconButtonSideText>
            </IconButtonContainer>
          </Navigation>
        </S.GridItemNavigation>
      </>
    )
  }
}
