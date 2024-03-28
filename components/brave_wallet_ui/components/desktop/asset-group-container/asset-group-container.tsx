// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Constants
import {
  LOCAL_STORAGE_KEYS //
} from '../../../common/constants/local-storage-keys'

// Slices
import {
  networkEntityAdapter //
} from '../../../common/slices/entities/network.entity'

// Utils
import { reduceAddress } from '../../../utils/reduce-address'
import { getLocale } from '../../../../common/locale'
import {
  useLocalStorage,
  useSyncedLocalStorage
} from '../../../common/hooks/use_local_storage'

// Types
import { BraveWallet } from '../../../constants/types'
import {
  ExternalWalletProvider //
} from '../../../../brave_rewards/resources/shared/lib/external_wallet'

// Components
import { CreateNetworkIcon } from '../../shared/create-network-icon/index'
import { LoadingSkeleton } from '../../shared/loading-skeleton/index'
import {
  CreateAccountIcon //
} from '../../shared/create-account-icon/create-account-icon'

// Styled Components
import {
  StyledWrapper,
  CollapseButton,
  CollapseIcon,
  AccountDescriptionWrapper,
  RewardsProviderContainer,
  RewardsText
} from './asset-group-container.style'
import {
  Row,
  Column,
  Text,
  HorizontalSpace,
  BraveRewardsIndicator
} from '../../shared/style'

interface Props {
  externalProvider?: ExternalWalletProvider | null
  network?: BraveWallet.NetworkInfo | undefined
  account?: BraveWallet.AccountInfo | undefined
  isSkeleton?: boolean
  isDisabled?: boolean
  balance: string
  hideBalance?: boolean
  children?: React.ReactNode
}

export const AssetGroupContainer = (props: Props) => {
  const {
    balance,
    hideBalance,
    account,
    isSkeleton,
    isDisabled,
    network,
    children,
    externalProvider
  } = props

  // Local-Storage
  const [hidePortfolioBalances] = useSyncedLocalStorage(
    LOCAL_STORAGE_KEYS.HIDE_PORTFOLIO_BALANCES,
    false
  )
  const [collapsedAccounts, setCollapsedPortfolioAccountIds] = useLocalStorage<
    string[]
  >(LOCAL_STORAGE_KEYS.COLLAPSED_PORTFOLIO_ACCOUNT_IDS, [])
  const [collapsedNetworks, setCollapsedPortfolioNetworkKeys] = useLocalStorage<
    string[]
  >(LOCAL_STORAGE_KEYS.COLLAPSED_PORTFOLIO_NETWORK_KEYS, [])

  // Memos & Computed
  const externalRewardsDescription = network
    ? network.chainName
    : account
    ? account.name
    : ''

  const isCollapsed = React.useMemo(() => {
    if (network) {
      return collapsedNetworks.includes(
        networkEntityAdapter.selectId(network).toString()
      )
    }
    if (account) {
      return collapsedAccounts.includes(account.accountId.uniqueKey)
    }
    return false
  }, [network, account, collapsedAccounts, collapsedNetworks])

  const onToggleCollapsed = React.useCallback(() => {
    if (account) {
      // Construct new list
      const newCollapsedAccounts = isCollapsed
        ? collapsedAccounts.filter(
            (addressKey) => addressKey !== account.accountId.uniqueKey
          )
        : [...collapsedAccounts, account.accountId.uniqueKey]

      setCollapsedPortfolioAccountIds(newCollapsedAccounts)
    }

    if (network) {
      const networksKey = networkEntityAdapter.selectId(network).toString()

      // Construct new list
      const newCollapsedNetworks = isCollapsed
        ? collapsedNetworks.filter((networkKey) => networkKey !== networksKey)
        : [...collapsedNetworks, networksKey]

      setCollapsedPortfolioNetworkKeys(newCollapsedNetworks)
    }
  }, [
    account,
    network,
    isCollapsed,
    collapsedAccounts,
    setCollapsedPortfolioAccountIds,
    collapsedNetworks,
    setCollapsedPortfolioNetworkKeys
  ])

  return (
    <StyledWrapper
      fullWidth={true}
      isCollapsed={isSkeleton || isCollapsed}
    >
      <CollapseButton
        onClick={onToggleCollapsed}
        disabled={isDisabled}
      >
        {isSkeleton && (
          <Row width='unset'>
            <LoadingSkeleton
              width={24}
              height={24}
              circle={!!network}
            />
            <HorizontalSpace space='16px' />
            <LoadingSkeleton
              width={60}
              height={14}
            />
          </Row>
        )}
        {externalProvider && !isSkeleton && (
          <Row width='unset'>
            {network && (
              <CreateNetworkIcon
                network={network}
                marginRight={16}
                size='huge'
              />
            )}
            {account && (
              <CreateAccountIcon
                size='medium'
                externalProvider={externalProvider}
                marginRight={16}
              />
            )}
            <RewardsProviderContainer>
              <RewardsText
                textSize='14px'
                isBold={true}
                textColor='primary'
                textAlign='left'
              >
                {externalRewardsDescription}
              </RewardsText>
              <BraveRewardsIndicator>
                {getLocale('braveWalletBraveRewardsTitle')}
              </BraveRewardsIndicator>
            </RewardsProviderContainer>
          </Row>
        )}

        {network && !externalProvider && !isSkeleton && (
          <Row width='unset'>
            <CreateNetworkIcon
              network={network}
              marginRight={16}
              size='huge'
            />
            <Text
              textSize='14px'
              isBold={true}
              textColor='primary'
              textAlign='left'
            >
              {network.chainName}
            </Text>
          </Row>
        )}

        {account && !externalProvider && !isSkeleton && (
          <Row width='unset'>
            <CreateAccountIcon
              size='medium'
              account={account}
              marginRight={16}
            />
            <AccountDescriptionWrapper width='unset'>
              <Text
                textSize='14px'
                isBold={true}
                textColor='primary'
                textAlign='left'
              >
                {account.name}
              </Text>
              <HorizontalSpace space='8px' />
              <Text
                textSize='12px'
                isBold={false}
                textColor='secondary'
              >
                {reduceAddress(account.address)}
              </Text>
            </AccountDescriptionWrapper>
          </Row>
        )}

        <Row width='unset'>
          {balance !== '' && !hideBalance ? (
            <Text
              textSize='14px'
              isBold={true}
              textColor='primary'
            >
              {hidePortfolioBalances ? '******' : balance}
            </Text>
          ) : (
            <>
              {!hideBalance && (
                <LoadingSkeleton
                  width={60}
                  height={14}
                />
              )}
            </>
          )}

          {!isDisabled && (
            <CollapseIcon
              isCollapsed={isCollapsed}
              name='carat-down'
            />
          )}
        </Row>
      </CollapseButton>

      {!isCollapsed && !isDisabled && (
        <Column fullWidth={true}>{children}</Column>
      )}
    </StyledWrapper>
  )
}
