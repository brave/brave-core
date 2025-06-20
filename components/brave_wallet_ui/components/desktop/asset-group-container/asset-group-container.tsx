// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { skipToken } from '@reduxjs/toolkit/query/react'
import Button from '@brave/leo/react/button'

// Selectors
import {
  useSafeUISelector,
  useSafeWalletSelector,
} from '../../../common/hooks/use-safe-selector'
import { UISelectors, WalletSelectors } from '../../../common/selectors'

// Hooks
import {
  useGetChainTipStatusQuery,
  useGetZCashAccountInfoQuery,
  useStartShieldSyncMutation,
  useStopShieldSyncMutation,
} from '../../../common/slices/api.slice'

// Constants
import {
  LOCAL_STORAGE_KEYS, //
} from '../../../common/constants/local-storage-keys'

// Slices
import {
  networkEntityAdapter, //
} from '../../../common/slices/entities/network.entity'

// Utils
import { reduceAddress } from '../../../utils/reduce-address'
import { getLocale } from '../../../../common/locale'
import {
  useLocalStorage,
  useSyncedLocalStorage,
} from '../../../common/hooks/use_local_storage'
import { makeAccountRoute, openTab } from '../../../utils/routes-utils'

// Types
import { AccountPageTabs, BraveWallet } from '../../../constants/types'
import {
  ExternalWalletProvider, //
} from '../../../../brave_rewards/resources/shared/lib/external_wallet'

// Components
import { CreateNetworkIcon } from '../../shared/create-network-icon/index'
import { LoadingSkeleton } from '../../shared/loading-skeleton/index'
import {
  CreateAccountIcon, //
} from '../../shared/create-account-icon/create-account-icon'
import {
  ZCashSyncModal, //
} from '../popup-modals/zcash_sync_modal/zcash_sync_modal'
import {
  AddressActionsMenu, //
} from '../wallet-menus/address_actions_menu'

// Styled Components
import {
  StyledWrapper,
  CollapsedWrapper,
  CollapseIcon,
  AccountDescriptionWrapper,
  RewardsProviderContainer,
  RewardsText,
  InfoBar,
  InfoText,
  WarningIcon,
  BalanceClickArea,
  AccountIconClickArea,
  AccountNameClickArea,
  AddressArea,
  NetworkAndExternalProviderClickArea,
  EmptyClickArea,
} from './asset-group-container.style'
import {
  Row,
  Column,
  Text,
  HorizontalSpace,
  BraveRewardsIndicator,
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
    externalProvider,
  } = props

  // Local-Storage
  const [hidePortfolioBalances] = useSyncedLocalStorage(
    LOCAL_STORAGE_KEYS.HIDE_PORTFOLIO_BALANCES,
    false,
  )
  const [collapsedAccounts, setCollapsedPortfolioAccountIds] = useLocalStorage<
    string[]
  >(LOCAL_STORAGE_KEYS.COLLAPSED_PORTFOLIO_ACCOUNT_IDS, [])
  const [collapsedNetworks, setCollapsedPortfolioNetworkKeys] = useLocalStorage<
    string[]
  >(LOCAL_STORAGE_KEYS.COLLAPSED_PORTFOLIO_NETWORK_KEYS, [])

  // State
  const [showSyncAccountModal, setShowSyncAccountModal] =
    React.useState<boolean>(false)

  // Selectors
  const isZCashShieldedTransactionsEnabled = useSafeWalletSelector(
    WalletSelectors.isZCashShieldedTransactionsEnabled,
  )
  const isPanel = useSafeUISelector(UISelectors.isPanel)

  // mutations
  const [startShieldSync] = useStartShieldSyncMutation()
  const [stopShieldSync] = useStopShieldSyncMutation()

  // Queries & Computed
  const { data: zcashAccountInfo } = useGetZCashAccountInfoQuery(
    isZCashShieldedTransactionsEnabled
      && account
      && account.accountId.coin === BraveWallet.CoinType.ZEC
      ? account.accountId
      : skipToken,
  )

  const isShieldedAccount =
    isZCashShieldedTransactionsEnabled
    && !!zcashAccountInfo
    && !!zcashAccountInfo.accountShieldBirthday

  const { data: chainTipStatus } = useGetChainTipStatusQuery(
    account && isShieldedAccount ? account.accountId : skipToken,
  )

  const blocksBehind = chainTipStatus
    ? chainTipStatus.chainTip - chainTipStatus.latestScannedBlock
    : 0

  const showSyncWarning =
    isShieldedAccount && (blocksBehind > 1000 || chainTipStatus === null)

  // Memos & Computed
  const externalRewardsDescription = network
    ? network.chainName
    : account
      ? account.name
      : ''

  const isCollapsed = React.useMemo(() => {
    if (network) {
      return collapsedNetworks.includes(
        networkEntityAdapter.selectId(network).toString(),
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
            (addressKey) => addressKey !== account.accountId.uniqueKey,
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
    setCollapsedPortfolioNetworkKeys,
  ])

  const onStartShieldSync = React.useCallback(async () => {
    if (isPanel && account) {
      openTab(
        'brave://wallet'
          + makeAccountRoute(account, AccountPageTabs.AccountAssetsSub),
      )
      return
    }
    if (account) {
      await startShieldSync(account.accountId)
      setShowSyncAccountModal(true)
    }
  }, [startShieldSync, account, isPanel])

  const onStopAndCloseShieldSync = React.useCallback(async () => {
    if (account) {
      await stopShieldSync(account.accountId)
      setShowSyncAccountModal(false)
    }
  }, [stopShieldSync, account])

  return (
    <StyledWrapper
      fullWidth={true}
      isCollapsed={isSkeleton || isCollapsed}
    >
      <CollapsedWrapper isCollapsed={isSkeleton || isCollapsed}>
        {isSkeleton && (
          <Row
            width='unset'
            padding='12px'
          >
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
          <NetworkAndExternalProviderClickArea
            onClick={onToggleCollapsed}
            disabled={isDisabled}
          >
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
          </NetworkAndExternalProviderClickArea>
        )}

        {network && !externalProvider && !isSkeleton && (
          <NetworkAndExternalProviderClickArea
            onClick={onToggleCollapsed}
            disabled={isDisabled}
          >
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
          </NetworkAndExternalProviderClickArea>
        )}

        {account && !externalProvider && !isSkeleton && (
          <Row
            width='unset'
            height='100%'
          >
            <AccountIconClickArea
              onClick={onToggleCollapsed}
              disabled={isDisabled}
            >
              <CreateAccountIcon
                size='medium'
                account={account}
                marginRight={16}
              />
            </AccountIconClickArea>
            <AccountDescriptionWrapper width='unset'>
              <AccountNameClickArea
                onClick={onToggleCollapsed}
                disabled={isDisabled}
                hasAddress={account.address !== ''}
              >
                <Text
                  textSize='14px'
                  isBold={true}
                  textColor='primary'
                  textAlign='left'
                >
                  {account.name}
                </Text>
                <HorizontalSpace space='8px' />
              </AccountNameClickArea>
              {account.address !== '' && (
                <AddressArea width='unset'>
                  <AddressActionsMenu account={account}>
                    <Text
                      textSize='12px'
                      isBold={false}
                      textColor='secondary'
                    >
                      {reduceAddress(account.address)}
                    </Text>
                  </AddressActionsMenu>
                  <EmptyClickArea
                    onClick={onToggleCollapsed}
                    disabled={isDisabled}
                  />
                </AddressArea>
              )}
            </AccountDescriptionWrapper>
          </Row>
        )}

        <BalanceClickArea
          onClick={onToggleCollapsed}
          disabled={isDisabled}
        >
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
        </BalanceClickArea>
      </CollapsedWrapper>

      {!isCollapsed && !isDisabled && (
        <Column fullWidth={true}>
          {children}
          {showSyncWarning && (
            <Row padding='8px'>
              <InfoBar justifyContent='space-between'>
                <Row
                  width='unset'
                  gap='16px'
                >
                  <WarningIcon />
                  <InfoText
                    textSize='14px'
                    isBold={false}
                    textAlign='left'
                  >
                    {!chainTipStatus
                      ? getLocale('braveWalletOutOfSyncTitle')
                      : getLocale(
                          'braveWalletOutOfSyncBlocksBehindTitle',
                        ).replace('$1', blocksBehind.toLocaleString())}
                  </InfoText>
                </Row>
                <div>
                  <Button
                    kind='plain'
                    size='tiny'
                    onClick={onStartShieldSync}
                  >
                    {getLocale('braveWalletSyncAccountButton')}
                  </Button>
                </div>
              </InfoBar>
            </Row>
          )}
        </Column>
      )}
      {showSyncAccountModal && account && (
        <ZCashSyncModal
          account={account}
          onClose={onStopAndCloseShieldSync}
        />
      )}
    </StyledWrapper>
  )
}
