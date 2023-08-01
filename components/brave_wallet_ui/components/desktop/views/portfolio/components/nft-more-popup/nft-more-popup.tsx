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
  isTokenHidden: boolean
  isTokenSpam: boolean
  onEditNft: () => void
  onHideNft: () => void
  onUnHideNft: () => void
  onMoveToSpam: () => void
  onUnSpam: () => void
}

export const NftMorePopup = (props: Props) => {
  const {
    isTokenHidden,
    isTokenSpam,
    onEditNft,
    onHideNft,
    onUnHideNft,
    onUnSpam,
    onMoveToSpam
  } = props

  return (
    <StyledWrapper>
      <PopupButton onClick={onEditNft}>
        <ButtonIcon name='edit-pencil' />
        <PopupButtonText>{getLocale('braveNftsTabEdit')}</PopupButtonText>
      </PopupButton>
      {isTokenHidden ? (
        <PopupButton onClick={onUnHideNft}>
          <ButtonIcon name='eye-on' />
          <PopupButtonText>{getLocale('braveNftsTabUnhide')}</PopupButtonText>
        </PopupButton>
      ) : (
        <>
          <PopupButton onClick={onHideNft}>
            <ButtonIcon name='eye-off' />
            <PopupButtonText>{getLocale('braveNftsTabHide')}</PopupButtonText>
          </PopupButton>
          <PopupButton onClick={isTokenSpam ? onUnSpam : onMoveToSpam}>
            <ButtonIcon name='disable-outline' />
            <PopupButtonText>
              {getLocale(
                isTokenSpam
                  ? 'braveWalletNftUnspam'
                  : 'braveWalletNftMoveToSpam'
              )}
            </PopupButtonText>
          </PopupButton>
        </>
      )}
    </StyledWrapper>
  )
}
