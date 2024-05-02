// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { EntityId } from '@reduxjs/toolkit'
import Dropdown from '@brave/leo/react/dropdown'

// Types
import {
  LedgerDerivationPath,
  SolDerivationPaths,
  TrezorDerivationPath,
  TrezorDerivationPaths
} from '../../../common/hardware/types'
import { BraveWallet, FilecoinNetwork } from '../../../constants/types'
import {
  HardwareWalletDerivationPathLocaleMapping,
  HardwareWalletDerivationPathsMapping,
  SolHardwareWalletDerivationPathLocaleMapping
} from './hardware_wallet_connect.types'
import { HardwareVendor } from '../../../common/api/hardware_keyrings'

// Utils
import { getLocale } from '../../../../common/locale'

import {
  useGetNetworksRegistryQuery //
} from '../../../common/slices/api.slice'
import { makeNetworkAsset } from '../../../options/asset-options'
import {
  networkEntityAdapter //
} from '../../../common/slices/entities/network.entity'
import {
  getPathForEthLedgerIndex,
  getPathForSolLedgerIndex,
  getPathForTrezorIndex
} from '../../../utils/derivation_path_utils'

// Components
import { SearchBar } from '../../shared/search-bar/index'
import { NetworkFilterSelector } from '../network-filter-selector'
import { AccountListItem } from './account_list_item'

// Styles
import {
  DisclaimerText,
  DisclaimerWrapper
} from '../popup-modals/add-account-modal/style'
import {
  ButtonsContainer,
  HardwareWalletAccountsListContainer,
  SelectRow,
  SelectWrapper,
  LoadingWrapper,
  LoadIcon,
  NoSearchResultText,
  AccountListContainer,
  AccountListHeader,
  AccountListContent,
  DropdownLabel,
  HelpLink
} from './hardware_wallet_connect.styles'
import {
  ContinueButton //
} from '../../../page/screens/onboarding/onboarding.style'
import { Row } from '../../shared/style'

interface Props {
  hardwareWallet: HardwareVendor
  accounts: BraveWallet.HardwareWalletAccount[]
  preAddedHardwareWalletAccounts: BraveWallet.AccountInfo[]
  onLoadMore: () => void
  selectedDerivationPaths: string[]
  setSelectedDerivationPaths: (paths: string[]) => void
  selectedDerivationScheme: string
  setSelectedDerivationScheme: (scheme: string) => void
  onAddAccounts: () => void
  filecoinNetwork: FilecoinNetwork
  onChangeFilecoinNetwork: (network: FilecoinNetwork) => void
  coin: BraveWallet.CoinType
}

export const HardwareWalletAccountsList = ({
  accounts,
  preAddedHardwareWalletAccounts,
  hardwareWallet,
  selectedDerivationScheme,
  setSelectedDerivationScheme,
  setSelectedDerivationPaths,
  selectedDerivationPaths,
  onLoadMore,
  onAddAccounts,
  filecoinNetwork,
  onChangeFilecoinNetwork,
  coin
}: Props) => {
  // queries
  const { data: networksRegistry } = useGetNetworksRegistryQuery()

  // state
  const [filteredAccountList, setFilteredAccountList] = React.useState<
    BraveWallet.HardwareWalletAccount[]
  >([])
  const [isLoadingMore, setIsLoadingMore] = React.useState<boolean>(false)
  const [selectedNetworkId, setSelectedNetworkId] = React.useState<EntityId>(
    coin === BraveWallet.CoinType.ETH
      ? BraveWallet.MAINNET_CHAIN_ID
      : coin === BraveWallet.CoinType.SOL
      ? BraveWallet.SOLANA_MAINNET
      : BraveWallet.FILECOIN_MAINNET
  )

  // memos
  const accountNativeAsset = React.useMemo(() => {
    if (!networksRegistry) {
      return undefined
    }
    return makeNetworkAsset(networksRegistry.entities[selectedNetworkId])
  }, [networksRegistry, selectedNetworkId])

  const networksSubset = React.useMemo(() => {
    if (!networksRegistry) {
      return []
    }
    return networksRegistry.idsByCoinType[coin].map(
      (id) => networksRegistry.entities[id] as BraveWallet.NetworkInfo
    )
  }, [networksRegistry, coin])

  // computed
  const ethDerivationPathsEnum =
    HardwareWalletDerivationPathsMapping[hardwareWallet]
  const solDerivationPathsEnum = SolHardwareWalletDerivationPathLocaleMapping

  // methods
  const onSelectAccountCheckbox =
    (account: BraveWallet.HardwareWalletAccount) => () => {
      const { derivationPath } = account
      const isSelected = selectedDerivationPaths.includes(derivationPath)
      const updatedPaths = isSelected
        ? selectedDerivationPaths.filter((path) => path !== derivationPath)
        : [...selectedDerivationPaths, derivationPath]
      setSelectedDerivationPaths(updatedPaths)
    }

  const filterAccountList = (event: React.ChangeEvent<HTMLInputElement>) => {
    const search = event?.target?.value || ''
    if (search === '') {
      setFilteredAccountList(accounts)
    } else {
      const filteredList = accounts.filter((account) => {
        return (
          account.address.toLowerCase() === search.toLowerCase() ||
          account.address.toLowerCase().startsWith(search.toLowerCase())
        )
      })
      setFilteredAccountList(filteredList)
    }
  }

  const onClickLoadMore = () => {
    setIsLoadingMore(true)
    onLoadMore()
  }

  const isPreAddedAccount = React.useCallback(
    (account: BraveWallet.HardwareWalletAccount) => {
      return preAddedHardwareWalletAccounts.some(
        (e) => e.address === account.address
      )
    },
    [preAddedHardwareWalletAccounts]
  )

  const onSelectNetwork = React.useCallback(
    (n: BraveWallet.NetworkInfo): void => {
      setSelectedNetworkId(networkEntityAdapter.selectId(n))
      if (coin === BraveWallet.CoinType.FIL) {
        onChangeFilecoinNetwork(n.chainId as FilecoinNetwork)
      }
    },
    [coin, onChangeFilecoinNetwork]
  )

  const getPathValue = (
    path: string
  ): LedgerDerivationPath | TrezorDerivationPath => {
    return ethDerivationPathsEnum[path]
  }

  const getDerivationPathLabel = (
    pathValue: LedgerDerivationPath | TrezorDerivationPath
  ): string => {
    const pathLocale = HardwareWalletDerivationPathLocaleMapping[pathValue]
    const isTrezorPath = pathValue === TrezorDerivationPaths.Default
    const devicePath = isTrezorPath
      ? getPathForTrezorIndex(undefined, pathValue)
      : getPathForEthLedgerIndex(undefined, pathValue)

    return `${pathLocale} "${devicePath}"`
  }

  const onChangeDerivationScheme = (value?: string) => {
    if (value) {
      setSelectedDerivationScheme(value)
    }
  }

  // effects
  React.useEffect(() => {
    setFilteredAccountList(accounts)
    setIsLoadingMore(false)
  }, [accounts])

  React.useEffect(() => {
    if (selectedNetworkId) {
      return
    }
    if (!networksRegistry) {
      return
    }

    // set network dropdown default value
    setSelectedNetworkId(networksRegistry.idsByCoinType[coin][0])
  }, [networksRegistry, coin, selectedNetworkId])

  // render
  return (
    <>
      <SelectRow>
        <SelectWrapper>
          <NetworkFilterSelector
            networkListSubset={networksSubset}
            selectedNetwork={networksRegistry?.entities[selectedNetworkId]}
            onSelectNetwork={onSelectNetwork}
            disableAllAccountsOption
            isV2
          />
          {coin === BraveWallet.CoinType.ETH ? (
            <Dropdown
              value={selectedDerivationScheme}
              onChange={(e) => onChangeDerivationScheme(e.value)}
            >
              <Row
                width='100%'
                justifyContent='space-between'
                slot='label'
              >
                <DropdownLabel>{getLocale('braveWalletHDPath')}</DropdownLabel>
                <HelpLink
                  href='https://support.brave.com/hc/en-us/categories/360001059151-Brave-Wallet'
                  target='_blank'
                  rel='noopener noreferrer'
                >
                  {getLocale('braveWalletHelpCenter')}
                </HelpLink>
              </Row>
              <div slot='value'>
                {getDerivationPathLabel(
                  selectedDerivationScheme as
                    | LedgerDerivationPath
                    | TrezorDerivationPath
                )}
              </div>
              {Object.keys(ethDerivationPathsEnum).map((path) => {
                const pathValue = getPathValue(path)

                return (
                  <leo-option
                    value={pathValue}
                    key={pathValue}
                  >
                    {getDerivationPathLabel(pathValue)}
                  </leo-option>
                )
              })}
            </Dropdown>
          ) : null}
          {coin === BraveWallet.CoinType.SOL ? (
            <Dropdown
              value={selectedDerivationScheme}
              onChange={(e) => onChangeDerivationScheme(e.value)}
            >
              <div slot='value'>{selectedDerivationScheme}</div>
              {Object.keys(solDerivationPathsEnum).map((path) => {
                const pathLocale = solDerivationPathsEnum[path]
                return (
                  <leo-option
                    data-value={path}
                    key={path}
                  >
                    {pathLocale}{' '}
                    {`"${getPathForSolLedgerIndex(
                      undefined,
                      path as SolDerivationPaths
                    )}"`}
                  </leo-option>
                )
              })}
            </Dropdown>
          ) : null}
        </SelectWrapper>
      </SelectRow>
      {coin !== BraveWallet.CoinType.FIL && (
        <DisclaimerWrapper>
          <DisclaimerText>
            {getLocale('braveWalletSwitchHDPathTextHardwareWallet')}
          </DisclaimerText>
        </DisclaimerWrapper>
      )}
      <SearchBar
        placeholder={getLocale('braveWalletSearchScannedAccounts')}
        action={filterAccountList}
        isV2
      />
      <HardwareWalletAccountsListContainer>
        {accounts.length === 0 && (
          <LoadingWrapper>
            <LoadIcon size={'big'} />
          </LoadingWrapper>
        )}

        {accounts.length > 0 && filteredAccountList.length === 0 && (
          <NoSearchResultText>
            {getLocale('braveWalletConnectHardwareSearchNothingFound')}
          </NoSearchResultText>
        )}

        {accountNativeAsset &&
          accounts.length > 0 &&
          filteredAccountList.length > 0 && (
            <AccountListContainer>
              <AccountListHeader>
                <div>{getLocale('braveWalletSubviewAccount')}</div>
                <div>{getLocale('braveWalletBalance')}</div>
                <div>{getLocale('braveWalletAddAccountConnect')}</div>
              </AccountListHeader>
              <AccountListContent>
                {filteredAccountList.map((account) => {
                  const isPreAdded = isPreAddedAccount(account)

                  return (
                    <AccountListItem
                      key={account.derivationPath}
                      balanceAsset={accountNativeAsset}
                      account={account}
                      selected={
                        selectedDerivationPaths.includes(
                          account.derivationPath
                        ) || isPreAdded
                      }
                      disabled={isPreAdded}
                      onSelect={onSelectAccountCheckbox(account)}
                    />
                  )
                })}
              </AccountListContent>
            </AccountListContainer>
          )}
      </HardwareWalletAccountsListContainer>
      <ButtonsContainer>
        <ContinueButton
          onClick={onClickLoadMore}
          isLoading={isLoadingMore}
          isDisabled={
            isLoadingMore ||
            accounts.length === 0 ||
            selectedDerivationScheme === SolDerivationPaths.Bip44Root
          }
        >
          {isLoadingMore
            ? getLocale('braveWalletLoadingMoreAccountsHardwareWallet')
            : getLocale('braveWalletLoadMoreAccountsHardwareWallet')}
        </ContinueButton>
        <ContinueButton
          onClick={onAddAccounts}
          isDisabled={
            accounts.length === 0 || selectedDerivationPaths.length === 0
          }
        >
          {getLocale('braveWalletButtonContinue')}
        </ContinueButton>
      </ButtonsContainer>
    </>
  )
}

export default HardwareWalletAccountsList
