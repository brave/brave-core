// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'
import { EntityId } from '@reduxjs/toolkit'
import { Checkbox, Select } from 'brave-ui/components'

// Types
import {
  LedgerDerivationPath,
  SolDerivationPaths,
  TrezorDerivationPath,
  TrezorDerivationPaths
} from '../../../../../common/hardware/types'
import { BraveWallet, FilecoinNetwork } from '../../../../../constants/types'
import {
  HardwareWalletDerivationPathLocaleMapping,
  HardwareWalletDerivationPathsMapping,
  SolHardwareWalletDerivationPathLocaleMapping
} from './types'
import { HardwareVendor } from '../../../../../common/api/hardware_keyrings'

// Utils
import { getLocale } from '../../../../../../common/locale'
import { reduceAddress } from '../../../../../utils/reduce-address'
import Amount from '../../../../../utils/amount'
import {
  useGetHardwareAccountDiscoveryBalanceQuery,
  useGetNetworksRegistryQuery
} from '../../../../../common/slices/api.slice'
import { useAddressOrb } from '../../../../../common/hooks/use-orb'
import { makeNetworkAsset } from '../../../../../options/asset-options'
import {
  networkEntityAdapter //
} from '../../../../../common/slices/entities/network.entity'
import {
  getPathForEthLedgerIndex,
  getPathForSolLedgerIndex,
  getPathForTrezorIndex
} from '../../../../../utils/derivation_path_utils'

// Components
import { NavButton } from '../../../../extension/buttons/nav-button/index'
import { SearchBar } from '../../../../shared/search-bar/index'
import { NetworkFilterSelector } from '../../../network-filter-selector'
import { Skeleton } from '../../../../shared/loading-skeleton/styles'

// Styles
import { DisclaimerText } from '../style'
import {
  ButtonsContainer,
  DisclaimerWrapper,
  HardwareWalletAccountCircle,
  HardwareWalletAccountListItem,
  HardwareWalletAccountListItemRow,
  HardwareWalletAccountsListContainer,
  SelectRow,
  SelectWrapper,
  LoadingWrapper,
  LoadIcon,
  AddressBalanceWrapper,
  NoSearchResultText
} from './style'

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
          />
          {coin === BraveWallet.CoinType.ETH ? (
            <Select
              value={selectedDerivationScheme}
              onChange={setSelectedDerivationScheme}
              showAllContents
            >
              {Object.keys(ethDerivationPathsEnum).map((path) => {
                const pathValue: LedgerDerivationPath | TrezorDerivationPath =
                  ethDerivationPathsEnum[path]

                const pathLocale =
                  HardwareWalletDerivationPathLocaleMapping[pathValue]

                const isTrezorPath = pathValue === TrezorDerivationPaths.Default

                return (
                  <div
                    data-value={pathValue}
                    key={pathValue}
                  >
                    {pathLocale}{' '}
                    {`"${
                      isTrezorPath
                        ? getPathForTrezorIndex(undefined, pathValue)
                        : getPathForEthLedgerIndex(undefined, pathValue)
                    }"`}
                  </div>
                )
              })}
            </Select>
          ) : null}
          {coin === BraveWallet.CoinType.SOL ? (
            <Select
              value={selectedDerivationScheme}
              onChange={setSelectedDerivationScheme}
              showAllContents
            >
              {Object.keys(solDerivationPathsEnum).map((path) => {
                const pathLocale = solDerivationPathsEnum[path]
                return (
                  <div
                    data-value={path}
                    key={path}
                  >
                    {pathLocale}{' '}
                    {`"${getPathForSolLedgerIndex(
                      undefined,
                      path as SolDerivationPaths
                    )}"`}
                  </div>
                )
              })}
            </Select>
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
            <>
              {filteredAccountList.map((account) => {
                return (
                  <AccountListItem
                    key={account.derivationPath}
                    balanceAsset={accountNativeAsset}
                    account={account}
                    selected={
                      selectedDerivationPaths.includes(
                        account.derivationPath
                      ) || isPreAddedAccount(account)
                    }
                    disabled={isPreAddedAccount(account)}
                    onSelect={onSelectAccountCheckbox(account)}
                  />
                )
              })}
            </>
          )}
      </HardwareWalletAccountsListContainer>
      <ButtonsContainer>
        <NavButton
          onSubmit={onClickLoadMore}
          text={
            isLoadingMore
              ? getLocale('braveWalletLoadingMoreAccountsHardwareWallet')
              : getLocale('braveWalletLoadMoreAccountsHardwareWallet')
          }
          buttonType='primary'
          disabled={
            isLoadingMore ||
            accounts.length === 0 ||
            selectedDerivationScheme === SolDerivationPaths.Bip44Root
          }
        />
        <NavButton
          onSubmit={onAddAccounts}
          text={getLocale('braveWalletAddCheckedAccountsHardwareWallet')}
          buttonType='primary'
          disabled={
            accounts.length === 0 || selectedDerivationPaths.length === 0
          }
        />
      </ButtonsContainer>
    </>
  )
}

interface AccountListItemProps {
  account: BraveWallet.HardwareWalletAccount
  onSelect: () => void
  selected: boolean
  disabled: boolean
  balanceAsset: Pick<
    BraveWallet.BlockchainToken,
    | 'coin'
    | 'chainId'
    | 'contractAddress'
    | 'isErc721'
    | 'isNft'
    | 'symbol'
    | 'tokenId'
    | 'decimals'
  >
}

function AccountListItem({
  account,
  onSelect,
  selected,
  disabled,
  balanceAsset
}: AccountListItemProps) {
  // queries
  const { data: balanceResult, isFetching: isLoadingBalance } =
    useGetHardwareAccountDiscoveryBalanceQuery({
      coin: balanceAsset.coin,
      chainId: balanceAsset.chainId,
      address: account.address
    })

  // memos
  const orb = useAddressOrb(account.address)

  const balance = React.useMemo(() => {
    if (
      isLoadingBalance ||
      !balanceResult ||
      balanceAsset.decimals === undefined
    ) {
      return undefined
    }

    return new Amount(balanceResult)
      .divideByDecimals(balanceAsset.decimals)
      .formatAsAsset(undefined, balanceAsset.symbol)
  }, [
    isLoadingBalance,
    balanceResult,
    balanceAsset.decimals,
    balanceAsset.symbol
  ])

  // render
  return (
    <HardwareWalletAccountListItem>
      <HardwareWalletAccountCircle orb={orb} />
      <HardwareWalletAccountListItemRow>
        <AddressBalanceWrapper>
          <div>{reduceAddress(account.address)}</div>
        </AddressBalanceWrapper>
        {isLoadingBalance ? (
          <Skeleton
            width={'140px'}
            height={'100%'}
          />
        ) : (
          <AddressBalanceWrapper>{balance}</AddressBalanceWrapper>
        )}
        <Checkbox
          value={{ selected }}
          onChange={onSelect}
          disabled={disabled}
        >
          <div data-key={'selected'} />
        </Checkbox>
      </HardwareWalletAccountListItemRow>
    </HardwareWalletAccountListItem>
  )
}

export default HardwareWalletAccountsList
