// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useDispatch } from 'react-redux'

// Selectors
import {
  useSafeWalletSelector,
  useUnsafeUISelector,
} from '../../../common/hooks/use-safe-selector'
import {
  UISelectors,
  WalletSelectors
} from '../../../common/selectors'

// Constants
import {
  LOCAL_STORAGE_KEYS
} from '../../../common/constants/local-storage-keys'

// Slices
import {
  UIActions
} from '../../../common/slices/ui.slice'
import {
  networkEntityAdapter
} from '../../../common/slices/entities/network.entity'

// Utils
import { reduceAddress } from '../../../utils/reduce-address'
import { getLocale } from '../../../../common/locale'

// Types
import { BraveWallet } from '../../../constants/types'
import {
  ExternalWalletProvider
} from '../../../../brave_rewards/resources/shared/lib/external_wallet'

// Components
import { CreateNetworkIcon } from '../../shared/create-network-icon/index'
import { LoadingSkeleton } from '../../shared/loading-skeleton/index'
import {
  CreateAccountIcon
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
  VerticalDivider,
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
  children?: React.ReactNode,
  hasBorder?: boolean
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
    hasBorder = true,
    externalProvider
  } = props

  // Redux
  const dispatch = useDispatch()

  // Selectors
  const hidePortfolioBalances =
    useSafeWalletSelector(WalletSelectors.hidePortfolioBalances)
  const collapsedAccounts =
    useUnsafeUISelector(UISelectors.collapsedPortfolioAccountAddresses)
  const collapsedNetworks =
    useUnsafeUISelector(UISelectors.collapsedPortfolioNetworkKeys)

  // Memos & Computed
  const externalRewardsDescription =
    network
      ? network.chainName
      : account
        ? account.name
        : ''

  const isCollapsed = React.useMemo(() => {
    if (network) {
      return collapsedNetworks
        .includes(networkEntityAdapter
          .selectId(network)
          .toString()
        )
    }
    if (account) {
      return collapsedAccounts.includes(account.address)
    }
    return false
  }, [
    network,
    account,
    collapsedAccounts,
    collapsedNetworks
  ])

  const onToggleCollapsed = React.useCallback(() => {
    if (account) {
      // Construct new list
      const newCollapsedAccounts =
        isCollapsed
          ? collapsedAccounts
            .filter((addressKey) => addressKey !== account.address)
          : [...collapsedAccounts, account.address]

      // Update Collapsed Account Addresses in Local Storage
      window.localStorage.setItem(
        LOCAL_STORAGE_KEYS.COLLAPSED_PORTFOLIO_ACCOUNT_ADDRESSES,
        JSON.stringify(newCollapsedAccounts)
      )

      // Update Collapsed Account Addresses in Redux
      dispatch(
        UIActions
          .setCollapsedPortfolioAccountAddresses(
            newCollapsedAccounts
          ))
    }

    if (network) {
      const networksKey = networkEntityAdapter
        .selectId(network)
        .toString()

      // Construct new list
      const newCollapsedNetworks =
        isCollapsed
          ? collapsedNetworks
            .filter((networkKey) => networkKey !== networksKey)
          : [...collapsedNetworks, networksKey]

      // Update Collapsed Network Keys in Local Storage
      window.localStorage.setItem(
        LOCAL_STORAGE_KEYS.COLLAPSED_PORTFOLIO_NETWORK_KEYS,
        JSON.stringify(newCollapsedNetworks)
      )

      // Update Collapsed Network Keys in Redux
      dispatch(
        UIActions
          .setCollapsedPortfolioNetworkKeys(
            newCollapsedNetworks
          ))
    }
  }, [
    account,
    network,
    isCollapsed,
    collapsedAccounts,
    collapsedNetworks
  ])

  return (
    <StyledWrapper
      fullWidth={true}
      hasBorder={hasBorder}
    >
      <CollapseButton
        onClick={onToggleCollapsed}
        disabled={isDisabled}
      >
        {isSkeleton &&
          <Row
            width='unset'
          >
            <LoadingSkeleton width={24} height={24} circle={!!network} />
            <HorizontalSpace space='16px' />
            <LoadingSkeleton width={60} height={14} />
          </Row>
        }
        {externalProvider && !isSkeleton &&
          <Row
            width='unset'
          >
            {network &&
              <CreateNetworkIcon
                network={network}
                marginRight={16}
                size='big'
              />
            }
            {account &&
              <CreateAccountIcon
                size='small'
                externalProvider={externalProvider}
                marginRight={16}
              />
            }
            <RewardsProviderContainer>
              <RewardsText
                textSize='14px'
                isBold={true}
                textColor='text01'
                textAlign='left'
              >
                {externalRewardsDescription}
              </RewardsText>
              <BraveRewardsIndicator>
                {getLocale('braveWalletBraveRewardsTitle')}
              </BraveRewardsIndicator>
            </RewardsProviderContainer>
          </Row>
        }

        {network && !externalProvider && !isSkeleton &&
          <Row
            width='unset'
          >
            <CreateNetworkIcon
              network={network}
              marginRight={16}
              size='big'
            />
            <Text
              textSize='14px'
              isBold={true}
              textColor='text01'
              textAlign='left'
            >
              {network.chainName}
            </Text>
          </Row>
        }

        {account && !externalProvider && !isSkeleton &&
          <Row
            width='unset'
          >
            <CreateAccountIcon
              size='small'
              account={account}
              marginRight={16}
            />
            <AccountDescriptionWrapper
              width='unset'
            >
              <Text
                textSize='14px'
                isBold={true}
                textColor='text01'
                textAlign='left'
              >
                {account.name}
              </Text>
              <HorizontalSpace space='8px' />
              <Text
                textSize='12px'
                isBold={false}
                textColor='text02'
              >
                {reduceAddress(account.address)}
              </Text>
            </AccountDescriptionWrapper>
          </Row>
        }

        <Row
          width='unset'
        >
          {balance !== '' && !hideBalance ? (
            <Text
              textSize='12px'
              isBold={false}
              textColor='text02'
            >
              {hidePortfolioBalances ? '******' : balance}
            </Text>
          ) : (
            <>
              {!hideBalance && <LoadingSkeleton width={60} height={14} />}
            </>
          )}

          {!isDisabled &&
            <CollapseIcon
              isCollapsed={isCollapsed}
              name='carat-down'
            />
          }
        </Row>
      </CollapseButton>

      {!isCollapsed && !isDisabled &&
        <Column
          fullWidth={true}
        >
          <Row
            padding='0px 8px'
          >
            <VerticalDivider />
          </Row>
          {children}
        </Column>
      }
    </StyledWrapper>

  )
}
