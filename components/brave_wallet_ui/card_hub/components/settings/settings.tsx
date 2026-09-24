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
import {
  useLocalStorage,
  useSyncedLocalStorage,
} from '$wallet/common/hooks/use_local_storage'
import { useLockWalletMutation } from '$wallet/common/slices/api.slice'

// Components
import { SectionToggle } from './section_toggle/section_toggle'

// Styles
import {
  BackButton,
  BackIcon,
  Header,
  SectionLabel,
  IconBubble,
  SectionButton,
} from './settings.style'
import { Column, Text } from '$wallet/components/shared/style'

interface Props {
  onBack: () => void
}

export const Settings = (props: Props) => {
  const { onBack } = props

  // Local Storage
  const [hidePortfolioBalances, setHidePortfolioBalances] =
    useSyncedLocalStorage(LOCAL_STORAGE_KEYS.HIDE_PORTFOLIO_BALANCES, false)

  const [filteredOutWalletCards, setFilteredOutWalletCards] = useLocalStorage<
    WalletCardIds[]
  >(LOCAL_STORAGE_KEYS.FILTERED_OUT_WALLET_CARDS, [])

  // Mutations
  const [lockWallet] = useLockWalletMutation()

  // Methods
  const onToggleHideBalances = React.useCallback(() => {
    setHidePortfolioBalances((prev) => !prev)
  }, [setHidePortfolioBalances])

  const onLockWallet = React.useCallback(async () => {
    await lockWallet()
  }, [lockWallet])

  const onToggleWalletCard = React.useCallback(
    (cardId: WalletCardIds) => {
      setFilteredOutWalletCards((prev) => {
        if (prev.includes(cardId)) {
          return prev.filter((id) => id !== cardId)
        }
        return [...prev, cardId]
      })
    },
    [setFilteredOutWalletCards],
  )

  return (
    <Column width='100%'>
      <Header
        width='100%'
        padding='16px 12px'
        alignItems='center'
      >
        <BackButton
          fab
          kind='plain-faint'
          onClick={onBack}
        >
          <BackIcon name='arrow-left' />
        </BackButton>
        <Text
          variant='default.semibold'
          textColor='primary'
        >
          {getLocale(S.BRAVE_WALLET_WALLET_POPUP_SETTINGS)}
        </Text>
      </Header>
      <Column
        width='100%'
        padding='16px'
        gap='16px'
      >
        {/* Appearance */}
        <Column
          width='100%'
          justifyContent='flex-start'
          alignItems='flex-start'
        >
          <SectionLabel
            variant='xSmall.regular'
            textColor='tertiary'
          >
            {getLocale(S.BRAVE_WALLET_APPEARANCE)}
          </SectionLabel>
          <SectionToggle
            label={getLocale(S.BRAVE_WALLET_HIDE_BALANCES)}
            icon={hidePortfolioBalances ? 'eye-off' : 'eye-on'}
            checked={hidePortfolioBalances}
            onChange={onToggleHideBalances}
          />
        </Column>

        {/* Security */}
        <Column
          width='100%'
          justifyContent='flex-start'
          alignItems='flex-start'
        >
          <SectionLabel
            variant='xSmall.regular'
            textColor='tertiary'
          >
            {getLocale(S.BRAVE_WALLET_SECURITY)}
          </SectionLabel>
          <SectionButton onClick={onLockWallet}>
            <IconBubble>
              <Icon name='lock' />
            </IconBubble>
            <Column
              justifyContent='center'
              alignItems='flex-start'
            >
              <Text
                variant='default.semibold'
                textColor='primary'
              >
                {getLocale(S.BRAVE_WALLET_WALLET_POPUP_LOCK)}
              </Text>
              <Text
                variant='xSmall.regular'
                textColor='tertiary'
              >
                {getLocale(S.BRAVE_WALLET_REQUIRE_UNLOCK_NEXT_TIME)}
              </Text>
            </Column>
          </SectionButton>
        </Column>

        {/* Wallet */}
        <Column
          width='100%'
          justifyContent='flex-start'
          alignItems='flex-start'
        >
          <SectionLabel
            variant='xSmall.regular'
            textColor='tertiary'
          >
            {getLocale(S.BRAVE_WALLET_TITLE)}
          </SectionLabel>
          {WalletCardOptions.map((option) => (
            <SectionToggle
              key={option.id}
              label={getLocale(option.label)}
              icon={option.icon}
              checked={!filteredOutWalletCards.includes(option.id)}
              onChange={() => onToggleWalletCard(option.id)}
            />
          ))}
        </Column>
      </Column>
    </Column>
  )
}
