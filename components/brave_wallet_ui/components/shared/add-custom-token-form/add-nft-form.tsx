// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useDispatch } from 'react-redux'
import { skipToken } from '@reduxjs/toolkit/query/react'
import Button from '@brave/leo/react/button'

// utils
import { BraveWallet } from '../../../constants/types'
import Amount from '../../../utils/amount'
import { getLocale } from '$web-common/locale'
import { WalletActions } from '../../../common/actions'
import {
  networkEntityAdapter,
  emptyNetworksRegistry
} from '../../../common/slices/entities/network.entity'

// hooks
import useAssetManagement from '../../../common/hooks/assets-management'
import useGetTokenInfo from '../../../common/hooks/use-get-token-info'
import { useGetNetworksRegistryQuery } from '../../../common/slices/api.slice'
import { useSafeWalletSelector } from '../../../common/hooks/use-safe-selector'
import { WalletSelectors } from '../../../common/selectors'
import {
  useGetCustomAssetSupportedNetworks //
} from '../../../common/hooks/use_get_custom_asset_supported_networks'

// components
import {
  SelectNetworkDropdown //
} from '../../desktop/select-network-dropdown/index'
import Tooltip from '../tooltip'
import { FormErrorsList } from './form-errors-list'

// styles
import {
  ButtonRow,
  ButtonRowSpacer,
  ErrorText,
  FormWrapper,
  FullWidthFormColumn,
  Input,
  InputLabel,
  AddButtonWrapper
} from './add-custom-token-form-styles'
import { HorizontalSpace } from '../style'

interface Props {
  selectedAsset?: BraveWallet.BlockchainToken
  contractAddress: string
  onHideForm: () => void
  onChangeContractAddress: (contractAddress: string) => void
}

export const AddNftForm = (props: Props) => {
  const {
    selectedAsset,
    contractAddress: tokenContractAddress,
    onHideForm,
    onChangeContractAddress
  } = props

  // redux
  const addUserAssetError = useSafeWalletSelector(
    WalletSelectors.addUserAssetError
  )
  const dispatch = useDispatch()

  const { data: networksRegistry = emptyNetworksRegistry } =
    useGetNetworksRegistryQuery()
  const selectedAssetNetwork = selectedAsset
    ? networksRegistry.entities[networkEntityAdapter.selectId(selectedAsset)]
    : undefined

  // state
  const [showTokenIDRequired, setShowTokenIDRequired] =
    React.useState<boolean>(false)
  const [showNetworkDropDown, setShowNetworkDropDown] =
    React.useState<boolean>(false)
  const [hasError, setHasError] = React.useState<boolean>(addUserAssetError)

  // Form States
  const [customTokenName, setCustomTokenName] = React.useState<
    string | undefined
  >(selectedAsset?.name)
  const [customTokenID, setCustomTokenID] = React.useState<string | undefined>(
    selectedAsset?.tokenId
  )
  const [customTokenSymbol, setCustomTokenSymbol] = React.useState<
    string | undefined
  >(selectedAsset?.symbol)
  const [customAssetsNetwork, setCustomAssetsNetwork] = React.useState<
    BraveWallet.NetworkInfo | undefined
  >(selectedAssetNetwork)

  // custom hooks
  const { onAddCustomAsset } = useAssetManagement()

  const {
    tokenInfo: matchedTokenInfo,
    isVisible: tokenAlreadyExists,
    isLoading: isTokenInfoLoading
  } = useGetTokenInfo(
    customAssetsNetwork && tokenContractAddress
      ? {
          contractAddress: tokenContractAddress,
          network: {
            chainId: customAssetsNetwork.chainId,
            coin: customAssetsNetwork.coin
          }
        }
      : skipToken
  )

  const networkList = useGetCustomAssetSupportedNetworks()

  const name = customTokenName ?? matchedTokenInfo?.name ?? ''
  const symbol = customTokenSymbol ?? matchedTokenInfo?.symbol ?? ''
  const tokenId = customTokenID ?? matchedTokenInfo?.tokenId ?? ''

  const tokenInfo: BraveWallet.BlockchainToken | undefined =
    React.useMemo(() => {
      if (!customAssetsNetwork || !tokenContractAddress) {
        return undefined
      }

      return {
        chainId: customAssetsNetwork.chainId,
        coin: customAssetsNetwork.coin,
        contractAddress: tokenContractAddress,
        name,
        symbol,
        decimals: 0,
        coingeckoId: '',
        logo: '',
        tokenId: tokenId ? new Amount(tokenId).toHex() : '',
        isErc20: false,
        isErc721:
          customAssetsNetwork.coin !== BraveWallet.CoinType.SOL && !!tokenId,
        isErc1155: false,
        isNft: true,
        isSpam: false,
        visible: true
      }
    }, [customAssetsNetwork, tokenContractAddress, name, symbol, tokenId])

  const resetBaseInputFields = React.useCallback(() => {
    setCustomTokenName(undefined)
    setCustomTokenSymbol(undefined)
    setCustomTokenID(undefined)
  }, [])

  const resetInputFields = React.useCallback(() => {
    resetBaseInputFields()
    onChangeContractAddress('')
  }, [resetBaseInputFields, onChangeContractAddress])

  // Handle Form Input Changes
  const handleTokenNameChanged = React.useCallback(
    (event: React.ChangeEvent<HTMLInputElement>) => {
      setHasError(false)
      setCustomTokenName(event.target.value)
    },
    []
  )

  const handleTokenIDChanged = React.useCallback(
    (event: React.ChangeEvent<HTMLInputElement>) => {
      setHasError(false)
      setShowTokenIDRequired(false)
      setCustomTokenID(event.target.value)
    },
    []
  )

  const handleTokenSymbolChanged = React.useCallback(
    (event: React.ChangeEvent<HTMLInputElement>) => {
      setHasError(false)
      setCustomTokenSymbol(event.target.value)
    },
    []
  )

  const handleTokenAddressChanged = React.useCallback(
    (event: React.ChangeEvent<HTMLInputElement>) => {
      setHasError(false)
      if (event.target.value === '') {
        resetInputFields()
        return
      }

      onChangeContractAddress(event.target.value)
    },
    [onChangeContractAddress, resetInputFields]
  )

  // methods
  const addOrUpdateToken = React.useCallback(async () => {
    if (!tokenInfo) {
      return
    }

    if (tokenAlreadyExists && selectedAsset) {
      dispatch(
        WalletActions.updateUserAsset({
          existing: selectedAsset,
          updated: tokenInfo
        })
      )
    } else {
      onAddCustomAsset(tokenInfo)
    }

    onHideForm()
  }, [
    tokenInfo,
    selectedAsset,
    tokenAlreadyExists,
    onAddCustomAsset,
    onHideForm
  ])

  const onHideNetworkDropDown = React.useCallback(() => {
    if (showNetworkDropDown) {
      setShowNetworkDropDown(false)
    }
  }, [showNetworkDropDown])

  const onShowNetworkDropDown = React.useCallback(() => {
    setShowNetworkDropDown(true)
  }, [])

  const onSelectCustomNetwork = React.useCallback(
    (network: BraveWallet.NetworkInfo) => {
      setCustomAssetsNetwork(network)
      onHideNetworkDropDown()
    },
    [onHideNetworkDropDown]
  )

  const onClickCancel = React.useCallback(() => {
    resetInputFields()
    onHideForm()
  }, [resetInputFields, onHideForm])

  // computed
  const customAssetsNetworkError = !customAssetsNetwork?.chainId
  const tokenNameError = !tokenInfo?.name
  const tokenSymbolError = !tokenInfo?.symbol
  const tokenContractAddressError =
    tokenContractAddress === '' ||
    (customAssetsNetwork?.coin !== BraveWallet.CoinType.SOL &&
      !tokenContractAddress.toLowerCase().startsWith('0x')) ||
    (customAssetsNetwork?.coin === BraveWallet.CoinType.ETH && !customTokenID)

  const buttonDisabled =
    isTokenInfoLoading ||
    tokenNameError ||
    tokenSymbolError ||
    tokenContractAddressError ||
    customAssetsNetworkError

  // memos
  const formErrors = React.useMemo(() => {
    return [
      tokenContractAddressError &&
        getLocale('braveWalletInvalidTokenContractAddressError'),
      customAssetsNetworkError &&
        getLocale('braveWalletNetworkIsRequiredError'),
      tokenNameError && getLocale('braveWalletTokenNameIsRequiredError'),
      tokenSymbolError && getLocale('braveWalletTokenSymbolIsRequiredError')
    ]
  }, [
    customAssetsNetworkError,
    tokenNameError,
    tokenContractAddressError,
    tokenSymbolError
  ])

  return (
    <>
      <FormWrapper onClick={onHideNetworkDropDown}>
        <FullWidthFormColumn>
          <InputLabel>
            {customAssetsNetwork?.coin === BraveWallet.CoinType.SOL
              ? getLocale('braveWalletTokenMintAddress')
              : getLocale('braveWalletWatchListTokenAddress')}
          </InputLabel>
          <Input
            value={tokenContractAddress}
            onChange={handleTokenAddressChanged}
            width='100%'
          />
        </FullWidthFormColumn>
        <FullWidthFormColumn>
          <InputLabel>{getLocale('braveWalletSelectNetwork')}</InputLabel>
          <SelectNetworkDropdown
            selectedNetwork={customAssetsNetwork}
            onClick={onShowNetworkDropDown}
            showNetworkDropDown={showNetworkDropDown}
            onSelectCustomNetwork={onSelectCustomNetwork}
            useWithSearch={false}
            networkListSubset={networkList}
          />
        </FullWidthFormColumn>
        <FullWidthFormColumn>
          <InputLabel>{getLocale('braveWalletWatchListTokenName')}</InputLabel>
          <Input
            value={name}
            onChange={handleTokenNameChanged}
            disabled={isTokenInfoLoading}
            width='100%'
          />
        </FullWidthFormColumn>
        <FullWidthFormColumn>
          <InputLabel>
            {getLocale('braveWalletWatchListTokenSymbol')}
          </InputLabel>
          <Input
            value={symbol}
            onChange={handleTokenSymbolChanged}
            disabled={isTokenInfoLoading}
            width='100%'
          />
        </FullWidthFormColumn>
        <FullWidthFormColumn>
          {customAssetsNetwork?.coin !== BraveWallet.CoinType.SOL && (
            <>
              <InputLabel>
                {getLocale('braveWalletWatchListTokenId')}
              </InputLabel>
              <Input
                value={tokenId ? new Amount(tokenId).format() : ''}
                onChange={handleTokenIDChanged}
                type='number'
                width='100%'
              />
            </>
          )}
        </FullWidthFormColumn>
        <>
          {showTokenIDRequired && (
            <ErrorText>
              {getLocale('braveWalletWatchListTokenIdError')}
            </ErrorText>
          )}
        </>
        {hasError && (
          <ErrorText>{getLocale('braveWalletWatchListError')}</ErrorText>
        )}
        <ButtonRowSpacer />
      </FormWrapper>
      <ButtonRow>
        <Button
          onClick={onClickCancel}
          kind='outline'
        >
          {getLocale('braveWalletButtonCancel')}
        </Button>
        <HorizontalSpace space='16px' />
        <Tooltip
          text={<FormErrorsList errors={formErrors} />}
          isVisible={buttonDisabled}
          verticalPosition='above'
          maxWidth={120}
        >
          <AddButtonWrapper>
            <Button
              onClick={addOrUpdateToken}
              isDisabled={buttonDisabled}
            >
              {selectedAsset
                ? getLocale('braveWalletButtonSaveChanges')
                : getLocale('braveWalletWatchListAdd')}
            </Button>
          </AddButtonWrapper>
        </Tooltip>
      </ButtonRow>
    </>
  )
}
