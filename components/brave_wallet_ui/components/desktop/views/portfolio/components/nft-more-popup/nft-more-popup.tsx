// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

import { getLocale } from '../../../../../../../common/locale'
import { useOnClickOutside } from '../../../../../../common/hooks/useOnClickOutside'

// Styled Components
import {
  ButtonIcon,
  Popup,
  PopupButton,
  PopupButtonText
} from './nft-more-popup.styles'

interface Props {
  isOpen: boolean
  isTokenHidden: boolean
  isTokenSpam: boolean
  onEditNft: () => void
  onHideNft: () => void
  onUnHideNft: () => void
  onUnSpam: () => void
  onMarkAsSpam: () => void
  onRemoveNft: () => void
  onClose: () => void
}

export const NftMorePopup = (props: Props) => {
  const {
    isOpen,
    isTokenHidden,
    isTokenSpam,
    onEditNft,
    onHideNft,
    onUnHideNft,
    onUnSpam,
    onRemoveNft,
    onClose,
    onMarkAsSpam
  } = props

  const popupRef = React.useRef<HTMLDivElement>(null)

  // hooks
  useOnClickOutside(popupRef, onClose, isOpen)

  return (
    <Popup
      isOpen={isOpen}
      ref={popupRef}
    >
      {/* show hide and edit option if a token is not hidden or not spam */}
      {!isTokenHidden && !isTokenSpam && (
        <>
          <PopupButton onClick={onEditNft}>
            <ButtonIcon name='edit-pencil' />
            <PopupButtonText>{getLocale('braveNftsTabEdit')}</PopupButtonText>
          </PopupButton>
          <PopupButton onClick={onHideNft}>
            <ButtonIcon name='eye-off' />
            <PopupButtonText>{getLocale('braveNftsTabHide')}</PopupButtonText>
          </PopupButton>
        </>
      )}

      {isTokenSpam ? (
        // show mark as not junk if a token is junk/spam
        <PopupButton onClick={onUnSpam}>
          <ButtonIcon name='disable-outline' />
          <PopupButtonText>{getLocale('braveWalletNftUnspam')}</PopupButtonText>
        </PopupButton>
      ) : (
        // show mark as spam option if a token is not marked as junk
        <PopupButton onClick={onMarkAsSpam}>
          <ButtonIcon name='trash' />
          <PopupButtonText>
            {getLocale('braveWalletNftMoveToSpam')}
          </PopupButtonText>
        </PopupButton>
      )}

      {/* show unhide option if a token is hidden but not junk */}
      {isTokenHidden && !isTokenSpam ? (
        <PopupButton onClick={onUnHideNft}>
          <ButtonIcon name='eye-on' />
          <PopupButtonText>{getLocale('braveNftsTabUnhide')}</PopupButtonText>
        </PopupButton>
      ) : null}

      {/* remove option */}
      <PopupButton onClick={onRemoveNft}>
        <ButtonIcon name='trash' />
        <PopupButtonText>{getLocale('braveNftsTabRemove')}</PopupButtonText>
      </PopupButton>
    </Popup>
  )
}
