// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Styled Components
import {
  StyledWrapper,
  PopupButton,
  PopupButtonText,
  ButtonIcon,
} from './nft-more-popup.styles'
import { getLocale } from '../../../../../../../common/locale'

interface Props {
  isHidden: boolean
  onEditNft: () => void
  onHideNft: () => void
  onUnHideNft: () => void
}

export const NftMorePopup = (props: Props) => {
  const {
    isHidden,
    onEditNft,
    onHideNft,
    onUnHideNft
  } = props

  return (
    <StyledWrapper>
      <PopupButton onClick={onEditNft}>
        <ButtonIcon name='edit-pencil' />
        <PopupButtonText>{getLocale('braveNftsTabEdit')}</PopupButtonText>
      </PopupButton>
      {isHidden
        ? (
          <PopupButton onClick={onUnHideNft}>
            <ButtonIcon name='eye-on' />
            <PopupButtonText>{getLocale('braveNftsTabUnhide')}</PopupButtonText>
          </PopupButton>
        ) : (
          <PopupButton onClick={onHideNft}>
            <ButtonIcon name='eye-off' />
            <PopupButtonText>{getLocale('braveNftsTabHide')}</PopupButtonText>
         </PopupButton>
        )
      }
    </StyledWrapper>
  )
}
