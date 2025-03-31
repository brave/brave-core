// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
// import { skipToken } from '@reduxjs/toolkit/query/react'
import Button from '@brave/leo/react/button'

// Types
import { BraveWallet } from '../../../../../constants/types'

// Queries
import {
  useGetActiveOriginConnectedAccountIdsQuery,
  useGetActiveOriginQuery,
  useGetNetworkForAccountOnActiveOriginQuery,
  useGetNetworksQuery,
  useRemoveSitePermissionMutation,
  useRequestSitePermissionMutation,
  useSetNetworkForAccountOnActiveOriginMutation,
  useSetSelectedAccountMutation
} from '../../../../../common/slices/api.slice'
import {
  useSelectedETHAccountQuery,
  useSelectedSOLAccountQuery
} from '../../../../../common/slices/api.slice.extra'

// Proxies
import getWalletPanelApiProxy from '../../../../../panel/wallet_panel_api_proxy'

// Utils
import { getLocale } from '$web-common/locale'
import { reduceAddress } from '../../../../../utils/reduce-address'

// Components
import {
  CreateAccountIcon //
} from '../../../../shared/create-account-icon/create-account-icon'
import { CreateNetworkIcon } from '../../../../shared/create-network-icon'
import { BottomSheet } from '../../../../shared/bottom_sheet/bottom_sheet'
import {
  DAppConnectionNetworks //
} from '../dapp_connection_networks/dapp_connection_networks'
import {
  DAppConnectionAccounts //
} from '../dapp_connection_accounts/dapp_connection_accounts'

// Styled Components
import {
  ConnectionCard,
  ControlsWrapper,
  StatusText,
  Icon,
  SelectButton,
  SelectButtonIcon,
  TopCardBorder
} from './connection_section.style'
import { Column, Row, Text } from '../../../../shared/style'

interface Props {
  coin: BraveWallet.CoinType
}

export const ConnectionSection = (props: Props) => {
  const { coin } = props

  // State
  const [isPermissionDenied, setIsPermissionDenied] =
    React.useState<boolean>(false)
  const [isSolanaConnected, setIsSolanaConnected] =
    React.useState<boolean>(false)
  const [showAccounts, setShowAccounts] = React.useState<boolean>(false)
  const [showNetworks, setShowNetworks] = React.useState<boolean>(false)
  const [selectedNetworkState, setSelectedNetworkState] = React.useState<
    BraveWallet.NetworkInfo | undefined
  >()

  // Mutations
  const [requestSitePermission] = useRequestSitePermissionMutation()
  const [removeSitePermission] = useRemoveSitePermissionMutation()
  const [setNetworkForAccountOnActiveOrigin] =
    useSetNetworkForAccountOnActiveOriginMutation()
  const [setSelectedAccount] = useSetSelectedAccountMutation()

  // Queries
  const { data: activeOrigin = { eTldPlusOne: '', originSpec: '' } } =
    useGetActiveOriginQuery()
  const { data: selectedSOLAccount } = useSelectedSOLAccountQuery()
  const { data: selectedETHAccount } = useSelectedETHAccountQuery()
  const { data: networkForAccount } =
    useGetNetworkForAccountOnActiveOriginQuery({
      accountId:
        coin === BraveWallet.CoinType.ETH
          ? selectedETHAccount?.accountId
          : selectedSOLAccount?.accountId
    })
  const { data: networks = [] } = useGetNetworksQuery()
  const firstNetworkByCoin = networks.filter(
    (network) => network.coin === coin
  )[0]
  const selectedNetworkForAccount = networkForAccount ?? undefined
  const selectedNetwork =
    selectedNetworkForAccount || selectedNetworkState || firstNetworkByCoin

  const { data: connectedAccounts = [] } =
    useGetActiveOriginConnectedAccountIdsQuery()

  // Computed
  const isChromeOrigin = activeOrigin?.originSpec.startsWith('chrome')
  const selectedAccount =
    coin === BraveWallet.CoinType.SOL ? selectedSOLAccount : selectedETHAccount

  // Memos
  const isConnected = React.useMemo((): boolean => {
    if (!selectedAccount || isPermissionDenied) {
      return false
    }
    if (coin === BraveWallet.CoinType.SOL) {
      return isSolanaConnected
    }
    return connectedAccounts.some(
      (accountId) => accountId.uniqueKey === selectedAccount.accountId.uniqueKey
    )
  }, [
    connectedAccounts,
    selectedAccount,
    coin,
    isSolanaConnected,
    isPermissionDenied
  ])

  const connectionStatusText = React.useMemo((): string => {
    if (isPermissionDenied) {
      return getLocale('braveWalletPanelBlocked')
    }
    return isConnected
      ? getLocale('braveWalletPanelConnected')
      : getLocale('braveWalletNotConnected')
  }, [isConnected, isPermissionDenied])

  // Methods
  const onClickConnect = React.useCallback(() => {
    if (selectedAccount) {
      requestSitePermission(selectedAccount.accountId)
    }
  }, [selectedAccount, requestSitePermission])

  const onClickDisconnect = React.useCallback(async () => {
    if (selectedAccount) {
      await removeSitePermission(selectedAccount.accountId)
    }
  }, [selectedAccount, removeSitePermission])

  const onClickUnblock = React.useCallback(() => {
    chrome.tabs.create(
      {
        url: 'brave://settings/content/all'
      },
      () => {
        if (chrome.runtime.lastError) {
          console.error(
            'tabs.create failed: ' + chrome.runtime.lastError.message
          )
        }
      }
    )
  }, [])

  const onChangeNetwork = React.useCallback(
    async (network: BraveWallet.NetworkInfo) => {
      if (selectedAccount) {
        await setNetworkForAccountOnActiveOrigin({
          accountId: selectedAccount.accountId,
          chainId: network.chainId
        })
        setSelectedNetworkState(network)
        setShowNetworks(false)
      }
    },
    [selectedAccount, setNetworkForAccountOnActiveOrigin]
  )

  const onChangeAccount = React.useCallback(
    (account: BraveWallet.AccountInfo) => {
      setSelectedAccount(account.accountId)
      setShowAccounts(false)
    },
    [setSelectedAccount]
  )

  // Effects
  React.useEffect(() => {
    let subscribed = true

    if (selectedAccount?.address && coin === BraveWallet.CoinType.SOL) {
      ;(async () => {
        const { panelHandler } = getWalletPanelApiProxy()
        await panelHandler
          .isSolanaAccountConnected(selectedAccount?.address)
          .then((result) => {
            if (subscribed) {
              setIsSolanaConnected(result.connected)
            }
          })
          .catch((e) => console.log(e))
      })()
    }

    return () => {
      subscribed = false
    }
  }, [selectedAccount?.address, coin])

  React.useEffect(() => {
    let subscribed = true

    if (coin) {
      ;(async () => {
        await getWalletPanelApiProxy()
          .braveWalletService.isPermissionDenied(coin)
          .then((result) => {
            if (subscribed) {
              setIsPermissionDenied(result.denied)
            }
          })
          .catch((e) => console.log(e))
      })()
    }

    return () => {
      subscribed = false
    }
  }, [coin, activeOrigin])

  return (
    <>
      <ConnectionCard
        connectionStatus={
          isPermissionDenied
            ? 'blocked'
            : isConnected
            ? 'connected'
            : 'not-connected'
        }
        fullWidth={true}
        padding='0px 4px 4px 4px'
      >
        <TopCardBorder marginBottom={4} />
        <Row marginBottom={4}>
          <Icon
            name={
              isPermissionDenied
                ? 'remove-circle-filled'
                : isConnected
                ? 'check-circle-filled'
                : 'close-circle-filled'
            }
          />
          <StatusText>{connectionStatusText}</StatusText>
        </Row>
        <ControlsWrapper
          fullWidth={true}
          gap='8px'
        >
          <Column
            width='100%'
            gap='4px'
            padding='4px'
          >
            <SelectButton onClick={() => setShowAccounts(true)}>
              <Row width='unset'>
                <CreateAccountIcon
                  size='tiny'
                  account={selectedAccount}
                  marginRight={12}
                />
                <Text
                  textSize='14px'
                  isBold={true}
                  textColor='primary'
                  textAlign='left'
                >
                  {selectedAccount?.name ?? ''}
                </Text>
              </Row>
              <Row width='unset'>
                <Text
                  textSize='14px'
                  isBold={false}
                  textColor='tertiary'
                >
                  {reduceAddress(selectedAccount?.address ?? '')}
                </Text>
                <SelectButtonIcon />
              </Row>
            </SelectButton>
            <SelectButton
              onClick={() => setShowNetworks(true)}
              data-test-id={
                coin === BraveWallet.CoinType.ETH ? 'select-network-button' : ''
              }
            >
              <Row width='unset'>
                <CreateNetworkIcon
                  network={selectedNetwork}
                  size='large'
                  marginRight={12}
                />
                <Text
                  textSize='14px'
                  isBold={true}
                  textColor='primary'
                >
                  {selectedNetwork?.chainName ?? ''}
                </Text>
              </Row>
              <SelectButtonIcon />
            </SelectButton>
          </Column>
          <Row
            width='100%'
            padding='0px 16px 16px 16px'
          >
            <Button
              size='small'
              kind={isConnected || isPermissionDenied ? 'outline' : 'filled'}
              isDisabled={isChromeOrigin}
              onClick={
                isPermissionDenied
                  ? onClickUnblock
                  : isConnected
                  ? onClickDisconnect
                  : onClickConnect
              }
            >
              {isPermissionDenied
                ? getLocale('braveWalletUnblock')
                : isConnected
                ? getLocale('braveWalletSitePermissionsDisconnect')
                : getLocale('braveWalletAddAccountConnect')}
            </Button>
          </Row>
        </ControlsWrapper>
      </ConnectionCard>
      <BottomSheet
        isOpen={showAccounts}
        onClose={() => setShowAccounts(false)}
        title={getLocale('braveWalletChangeAccount')}
      >
        <DAppConnectionAccounts
          coin={coin}
          onChangeAccount={onChangeAccount}
        />
      </BottomSheet>
      <BottomSheet
        isOpen={showNetworks}
        onClose={() => setShowNetworks(false)}
        title={getLocale('braveWalletChangeNetwork')}
      >
        <DAppConnectionNetworks
          onChangeNetwork={onChangeNetwork}
          selectedNetwork={selectedNetwork}
          coin={coin}
        />
      </BottomSheet>
    </>
  )
}
