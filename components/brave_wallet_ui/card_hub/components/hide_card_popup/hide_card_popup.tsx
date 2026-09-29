// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import Button from '@brave/leo/react/button'

// Hooks
import { useLocalStorage } from '$wallet/common/hooks/use_local_storage'

// Constants
import { LOCAL_STORAGE_KEYS } from '$wallet/common/constants/local-storage-keys'

// Utils
import { getLocale } from '$web-common/locale'

// Types
import { WalletCardIds } from '$wallet/constants/types'

// Options
import { WalletCardOptions } from '$wallet/options/wallet-card-options'

// Styles
import { Dialog } from './hide_card_popup.style'
import { Row } from '$wallet/components/shared/style'

interface Props {
  card: WalletCardIds | undefined
  isOpen: boolean
  onClose: () => void
}

export const HideCardPopup = (props: Props) => {
  const { card, isOpen, onClose } = props

  // Local Storage
  const [, setFilteredOutWalletCards] = useLocalStorage<WalletCardIds[]>(
    LOCAL_STORAGE_KEYS.FILTERED_OUT_WALLET_CARDS,
    [],
  )

  // Computed
  const cardLabelKey = WalletCardOptions.find(
    (option) => option.id === card,
  )?.label
  const cardName = cardLabelKey ? getLocale(cardLabelKey) : ''

  // Methods
  const onHideCard = () => {
    if (!card) {
      return
    }
    setFilteredOutWalletCards((prev) =>
      prev.includes(card) ? prev : [...prev, card],
    )
    onClose()
  }

  return (
    <Dialog
      onClose={onClose}
      isOpen={isOpen}
    >
      <div slot='subtitle'>
        {getLocale(S.BRAVE_WALLET_HIDE_CARD_FROM_HOME).replace('$1', cardName)}
      </div>
      <div>
        {getLocale(S.BRAVE_WALLET_HIDE_CARD_FROM_HOME_DESCRIPTION).replace(
          '$1',
          cardName,
        )}
      </div>
      <Row
        slot='actions'
        gap='8px'
      >
        <Button
          onClick={onClose}
          kind='outline'
        >
          {getLocale(S.BRAVE_WALLET_BUTTON_CANCEL)}
        </Button>
        <Button onClick={onHideCard}>
          {getLocale(S.BRAVE_WALLET_CONFIRM_HIDING_TOKEN)}
        </Button>
      </Row>
    </Dialog>
  )
}
