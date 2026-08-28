// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import Icon from '@brave/leo/react/icon'

import { getLocale } from '../../../../../common/locale'

// Styled Components
import { ButtonMenu } from '../../wallet-menus/wellet-menus.style'

interface Props {
  children: React.ReactNode
  isTokenHidden: boolean
  isTokenSpam: boolean
  onEditNft: () => void
  onHideNft: () => void
  onUnHideNft: () => void
  onUnSpam: () => void
  onMarkAsSpam: () => void
  onRemoveNft: () => void
}

export const NftMorePopup = (props: Props) => {
  const {
    children,
    isTokenHidden,
    isTokenSpam,
    onEditNft,
    onHideNft,
    onUnHideNft,
    onUnSpam,
    onRemoveNft,
    onMarkAsSpam,
  } = props

  return (
    <ButtonMenu
      placement='top-end'
      positionStrategy='fixed'
    >
      {children}
      {/* show hide and edit option if a token is not hidden or not spam */}
      {!isTokenHidden && !isTokenSpam && (
        <>
          <leo-menu-item onClick={onEditNft}>
            <Icon name='edit-pencil' />
            {getLocale(S.BRAVE_WALLET_NFTS_TAB_EDIT)}
          </leo-menu-item>
          <leo-menu-item onClick={onHideNft}>
            <Icon name='eye-off' />
            {getLocale(S.BRAVE_WALLET_NFTS_TAB_HIDE)}
          </leo-menu-item>
        </>
      )}

      {isTokenSpam ? (
        // show mark as not junk if a token is junk/spam
        <leo-menu-item onClick={onUnSpam}>
          <Icon name='junk-false' />
          {getLocale(S.BRAVE_WALLET_NFT_UNSPAM)}
        </leo-menu-item>
      ) : (
        // show mark as spam option if a token is not marked as junk
        <leo-menu-item onClick={onMarkAsSpam}>
          <Icon name='junk-true' />
          {getLocale(S.BRAVE_WALLET_NFT_MOVE_TO_SPAM)}
        </leo-menu-item>
      )}

      {/* show unhide option if a token is hidden but not junk */}
      {isTokenHidden && !isTokenSpam && (
        <leo-menu-item onClick={onUnHideNft}>
          <Icon name='eye-on' />
          {getLocale(S.BRAVE_WALLET_NFTS_TAB_UNHIDE)}
        </leo-menu-item>
      )}

      {/* remove option */}
      <leo-menu-item onClick={onRemoveNft}>
        <Icon name='trash' />
        {getLocale(S.BRAVE_WALLET_NFTS_TAB_REMOVE)}
      </leo-menu-item>
    </ButtonMenu>
  )
}
