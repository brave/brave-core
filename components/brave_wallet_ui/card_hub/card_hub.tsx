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

// Utils
import { getLocale } from '$web-common/locale'

// Hooks
import { useLocalStorage } from '$wallet/common/hooks/use_local_storage'
import { useCardHubTransition } from './hooks/use_card_hub_transition'

// Components
import { Crypto } from './cards/crypto/crypto'
import { Rewards } from './cards/rewards/rewards'
import { RewardsCard } from './cards/rewards_card/rewards_card'
import { HubMenu } from './components/hub_menu/hub_menu'
import { Settings } from './components/settings/settings'
import { HideCardPopup } from './components/hide_card_popup/hide_card_popup'
import { CardDetails } from './components/card_details/card_details'

// Styles
import {
  CardStack,
  FabButton,
  HubChrome,
  ScreenPane,
  StackItem,
  WalletLogo,
  Wrapper,
} from './card_hub.style'
import { Column, Row } from '$wallet/components/shared/style'

const walletCardComponents: Record<
  WalletCardIds,
  React.ComponentType<{
    onHide?: () => void
    onClick?: () => void
    locked?: boolean
  }>
> = {
  crypto: Crypto,
  'brave-rewards': Rewards,
  'brave-rewards-card': RewardsCard,
}

export const CardHub = () => {
  // State
  const [showSettings, setShowSettings] = React.useState(false)
  const [hidingCard, setHidingCard] = React.useState<WalletCardIds>()

  // Local Storage
  const [filteredOutWalletCards] = useLocalStorage<WalletCardIds[]>(
    LOCAL_STORAGE_KEYS.FILTERED_OUT_WALLET_CARDS,
    [],
  )

  // Computed
  const visibleCards = WalletCardOptions.filter((option) => {
    return !filteredOutWalletCards.includes(option.id)
  })
  const visibleCardIds = React.useMemo(() => {
    return WalletCardOptions.filter((option) => {
      return !filteredOutWalletCards.includes(option.id)
    }).map((option) => option.id)
  }, [filteredOutWalletCards])

  // Hooks
  const {
    selectedCardId,
    detailsOpen,
    isClosingDetails,
    isEnteringHub,
    isStackAnimating,
    wrapperRef,
    detailsSlotRef,
    setStackItemRef,
    onOpenCardDetails,
    onBackFromDetails,
    onEnterHub,
  } = useCardHubTransition(visibleCardIds)

  const selectedCardTitle = selectedCardId
    ? getLocale(
        WalletCardOptions.find((option) => option.id === selectedCardId)?.label
          ?? '',
      )
    : ''

  // Methods
  const onOpenNotifications = () => {
    // Will nav to notifications in the future
    alert('Open notifications')
  }

  const onBackFromScreen = () => {
    onEnterHub()
    setShowSettings(false)
  }

  return (
    <Wrapper
      ref={wrapperRef}
      $clip={isStackAnimating}
      justifyContent='flex-start'
      alignItems='center'
      width='100%'
      height='100%'
    >
      {showSettings && !selectedCardId && (
        <ScreenPane>
          <Settings onBack={onBackFromScreen} />
        </ScreenPane>
      )}
      <HubChrome $inactive={detailsOpen || showSettings}>
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
      </HubChrome>
      <Column
        width='100%'
        padding='0 16px 16px 16px'
      >
        <CardStack $frozen={!!selectedCardId || isEnteringHub}>
          {visibleCards.map((option) => {
            const Card = walletCardComponents[option.id]
            const isSelected = selectedCardId === option.id
            return (
              <StackItem
                key={option.id}
                ref={setStackItemRef(option.id)}
                $faded={detailsOpen && !isSelected}
                data-selected={detailsOpen && isSelected}
                data-returning={
                  isEnteringHub || (isClosingDetails && !isSelected)
                }
              >
                <Card
                  locked={!!selectedCardId || isEnteringHub}
                  onHide={
                    selectedCardId || isEnteringHub
                      ? undefined
                      : () => setHidingCard(option.id)
                  }
                  onClick={
                    selectedCardId || isEnteringHub
                      ? undefined
                      : () => onOpenCardDetails(option.id)
                  }
                />
              </StackItem>
            )
          })}
        </CardStack>
      </Column>
      <CardDetails
        title={selectedCardTitle}
        visible={detailsOpen}
        slotRef={detailsSlotRef}
        onBack={onBackFromDetails}
      />
      <HideCardPopup
        card={hidingCard}
        isOpen={!!hidingCard}
        onClose={() => setHidingCard(undefined)}
      />
    </Wrapper>
  )
}
