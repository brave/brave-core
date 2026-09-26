// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import Icon from '@brave/leo/react/icon'

// Types
import { WalletCardIds } from '$wallet/constants/types'

// Constants
import { LOCAL_STORAGE_KEYS } from '$wallet/common/constants/local-storage-keys'

// Options
import { WalletCardOptions } from '$wallet/options/wallet-card-options'

// Hooks
import { useLocalStorage } from '$wallet/common/hooks/use_local_storage'

// Components
import { Crypto } from './cards/crypto/crypto'
import { Rewards } from './cards/rewards/rewards'
import { RewardsCard } from './cards/rewards_card/rewards_card'
import { HubMenu } from './components/hub_menu/hub_menu'
import { Settings } from './components/settings/settings'

// Styles
import { CardStack, FabButton, WalletLogo, Wrapper } from './card_hub.style'
import { Column, Row } from '$wallet/components/shared/style'

const walletCardComponents: Record<WalletCardIds, React.ComponentType> = {
  crypto: Crypto,
  'brave-rewards': Rewards,
  'brave-rewards-card': RewardsCard,
}

export const CardHub = () => {
  // State
  const [showSettings, setShowSettings] = React.useState(false)

  const [filteredOutWalletCards] = useLocalStorage<WalletCardIds[]>(
    LOCAL_STORAGE_KEYS.FILTERED_OUT_WALLET_CARDS,
    [],
  )

  // Methods
  const onOpenNotifications = () => {
    // Will nav to notifications in the future
    alert('Open notifications')
  }

  return (
    <Wrapper
      justifyContent='flex-start'
      alignItems='center'
      width='100%'
      height='100%'
    >
      {showSettings ? (
        <Settings onBack={() => setShowSettings(false)} />
      ) : (
        <>
          <Row
            padding='24px'
            justifyContent='space-between'
            alignItems='center'
            width='100%'
          >
            <FabButton onClick={onOpenNotifications}>
              <Icon name='notification-dot' />
            </FabButton>
            <WalletLogo />
            <HubMenu onOpenSettings={() => setShowSettings(true)} />
          </Row>
          <Column
            width='100%'
            padding='0 16px 16px 16px'
          >
            <CardStack>
              {WalletCardOptions.map((option) => {
                if (filteredOutWalletCards.includes(option.id)) {
                  return null
                }
                const Card = walletCardComponents[option.id]
                return <Card key={option.id} />
              })}
            </CardStack>
          </Column>
        </>
      )}
    </Wrapper>
  )
}
