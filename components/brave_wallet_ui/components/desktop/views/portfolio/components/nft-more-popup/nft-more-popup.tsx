// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Styled Components
import {
  ButtonIcon,
  Popup,
  PopupButton,
  PopupButtonText,
} from './nft-more-popup.styles'
import { getLocale } from '../../../../../../../common/locale'

interface Props {
  isOpen: boolean
  isTokenHidden: boolean
  isTokenSpam: boolean
  onEditNft: () => void
  onHideNft: () => void
  onUnHideNft: () => void
  onUnSpam: () => void
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
    onClose
  } = props

  const popupRef = React.useRef<HTMLDivElement>(null)

  React.useEffect(() => {
    const handleKeyPress = (e: KeyboardEvent) => {
      if (e.key === 'Escape') onClose()
    }

    const handleClickOutside = (e: Event) => {
      if (popupRef.current && !popupRef.current.contains(e.target as Node)) {
        onClose()
      }
    }

    document.addEventListener('keydown', handleKeyPress)
    document.addEventListener('mousedown', handleClickOutside)

    return () => {
      document.removeEventListener('keydown', handleKeyPress)
      document.removeEventListener('mousedown', handleClickOutside)
    }
  }, [popupRef, onClose])

  return (
    <Popup isOpen={isOpen} ref={popupRef}>
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

      {/* show mark as not junk if a token is junk/spam */}
      {isTokenSpam && (
        <PopupButton onClick={onUnSpam}>
          <ButtonIcon name='disable-outline' />
          <PopupButtonText>{getLocale('braveWalletNftUnspam')}</PopupButtonText>
        </PopupButton>
      )}

      {/* show unhide option if a token is hidden but not junk */}
      {isTokenHidden && !isTokenSpam && (
        <PopupButton onClick={onUnHideNft}>
          <ButtonIcon name='eye-on' />
          <PopupButtonText>{getLocale('braveNftsTabUnhide')}</PopupButtonText>
        </PopupButton>
      )}

      {/* remove option */}
      <PopupButton onClick={onRemoveNft}>
        <ButtonIcon name='trash' />
        <PopupButtonText>{getLocale('braveNdftsTabRemove')}</PopupButtonText>
      </PopupButton>
    </Popup>
  )
}
