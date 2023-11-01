// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useSelector } from 'react-redux'
import { skipToken } from '@reduxjs/toolkit/query/react'
import Button from '@brave/leo/react/button'

// utils
import { getLocale } from '$web-common/locale'
import {
  emptyNetworksRegistry,
  networkEntityAdapter
} from '../../../common/slices/entities/network.entity'

// types
import { BraveWallet, WalletState } from '../../../constants/types'

// hooks
import { useLib } from '../../../common/hooks/useLib'
import useAssetManagement from '../../../common/hooks/assets-management'
import useTokenInfo from '../../../common/hooks/token'

import {
  useGetCoingeckoIdQuery,
  useGetNetworksRegistryQuery
} from '../../../common/slices/api.slice'
import {
  useGetCombinedTokensListQuery //
} from '../../../common/slices/api.slice.extra'

// components
import { SelectNetworkDropdown } from '../../desktop/select-network-dropdown/index'
import Tooltip from '../tooltip'
import { FormErrorsList } from './form-errors-list'

// styles
import {
  AdvancedButton,
  AdvancedIcon,
  ButtonRow,
  DividerRow,
  DividerText,
  ErrorText,
  FormColumn,
  FormRow,
  FormWrapper,
  Input,
  InputLabel,
  SubDivider,
  AddButtonWrapper
} from './add-custom-token-form-styles'
import { HorizontalSpace } from '../style'

interface Props {
  contractAddress: string
  onHideForm: () => void
  onNftAssetFound: (contractAddress: string) => void
  onChangeContractAddress: (contractAddress: string) => void
}

export const AddCustomTokenForm = (props: Props) => {
  const {
    contractAddress: tokenContractAddress,
    onHideForm,
    onNftAssetFound,
    onChangeContractAddress
  } = props

  // queries
  const { data: networksRegistry = emptyNetworksRegistry } =
    useGetNetworksRegistryQuery()

  // state
  const [showAdvancedFields, setShowAdvancedFields] =
    React.useState<boolean>(false)
  const [showNetworkDropDown, setShowNetworkDropDown] =
    React.useState<boolean>(false)

  // Form States
  const [tokenName, setTokenName] = React.useState<string>('')
  const [tokenSymbol, setTokenSymbol] = React.useState<string>('')
  const [tokenDecimals, setTokenDecimals] = React.useState<string>('')
  const [customCoingeckoId, setCustomCoingeckoId] = React.useState<
    string | undefined
  >(undefined)
  const [iconURL, setIconURL] = React.useState<string>('')
  const [customAssetsNetwork, setCustomAssetsNetwork] =
    React.useState<BraveWallet.NetworkInfo>()

  // redux
  const addUserAssetError = useSelector(
    ({ wallet }: { wallet: WalletState }) => wallet.addUserAssetError
  )

  // more state
  const [hasError, setHasError] = React.useState<boolean>(addUserAssetError)

  // queries
  const { data: combinedTokensList } = useGetCombinedTokensListQuery()

  // custom hooks
  const { getBlockchainTokenInfo } = useLib()
  const { onFindTokenInfoByContractAddress, foundTokenInfoByContractAddress } =
    useTokenInfo(
      getBlockchainTokenInfo,
      combinedTokensList,
      customAssetsNetwork
    )
  const { onAddCustomAsset } = useAssetManagement()

  const { data: matchedCoingeckoId } = useGetCoingeckoIdQuery(
    customAssetsNetwork && tokenContractAddress
      ? {
          chainId: customAssetsNetwork.chainId,
          contractAddress: tokenContractAddress
        }
      : skipToken
  )

  // If user has customized the coingecko id, use that even if it's an empty
  // string.
  const coingeckoId =
    customCoingeckoId ??
    (foundTokenInfoByContractAddress?.coingeckoId || tokenContractAddress
      ? matchedCoingeckoId || ''
      : '')

  // Handle Form Input Changes
  const handleTokenNameChanged = React.useCallback(
    (event: React.ChangeEvent<HTMLInputElement>) => {
      setHasError(false)
      setTokenName(event.target.value)
    },
    []
  )

  const handleTokenSymbolChanged = React.useCallback(
    (event: React.ChangeEvent<HTMLInputElement>) => {
      setHasError(false)
      setTokenSymbol(event.target.value)
    },
    []
  )

  const handleTokenAddressChanged = React.useCallback(
    (event: React.ChangeEvent<HTMLInputElement>) => {
      setHasError(false)
      onChangeContractAddress(event.target.value)
    },
    [onChangeContractAddress]
  )

  const handleTokenDecimalsChanged = React.useCallback(
    (event: React.ChangeEvent<HTMLInputElement>) => {
      setHasError(false)
      setTokenDecimals(event.target.value)
    },
    []
  )

  const handleCoingeckoIDChanged = React.useCallback(
    (event: React.ChangeEvent<HTMLInputElement>) => {
      setHasError(false)
      setCustomCoingeckoId(event.target.value)
    },
    []
  )

  const handleIconURLChanged = React.useCallback(
    (event: React.ChangeEvent<HTMLInputElement>) => {
      setHasError(false)
      setIconURL(event.target.value)
    },
    []
  )

  // methods
  const resetInputFields = React.useCallback(() => {
    setTokenName('')
    onChangeContractAddress('')
    setTokenSymbol('')
    setTokenDecimals('')
    setCustomCoingeckoId(undefined)
    setIconURL('')
  }, [onChangeContractAddress])

  const onClickAddCustomToken = React.useCallback(() => {
    if (!customAssetsNetwork) return

    if (foundTokenInfoByContractAddress) {
      if (foundTokenInfoByContractAddress.isErc721) {
        onNftAssetFound(foundTokenInfoByContractAddress.contractAddress)
      }
      let foundToken = { ...foundTokenInfoByContractAddress }
      foundToken.coingeckoId = coingeckoId
      foundToken.logo = foundToken.logo ? foundToken.logo : iconURL
      foundToken.chainId = customAssetsNetwork.chainId
      onAddCustomAsset(foundToken)
    } else {
      const newToken: BraveWallet.BlockchainToken = {
        contractAddress: tokenContractAddress,
        decimals: Number(tokenDecimals),
        isErc20: customAssetsNetwork.coin !== BraveWallet.CoinType.SOL,
        isErc721: false,
        isErc1155: false,
        isNft: false,
        isSpam: false,
        name: tokenName,
        symbol: tokenSymbol,
        tokenId: '',
        logo: iconURL,
        visible: true,
        coingeckoId,
        chainId: customAssetsNetwork.chainId,
        coin: customAssetsNetwork.coin
      }
      onAddCustomAsset(newToken)
    }
    onHideForm()
  }, [
    tokenContractAddress,
    foundTokenInfoByContractAddress,
    customAssetsNetwork,
    iconURL,
    tokenDecimals,
    tokenName,
    tokenSymbol,
    coingeckoId,
    onAddCustomAsset,
    onHideForm,
    onNftAssetFound
  ])

  const onToggleShowAdvancedFields = () =>
    setShowAdvancedFields((prev) => !prev)

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
      setCustomCoingeckoId(undefined)
    },
    [setCustomAssetsNetwork, onHideNetworkDropDown, setCustomCoingeckoId]
  )

  const onClickCancel = React.useCallback(() => {
    resetInputFields()
    onHideForm()
  }, [resetInputFields, onHideForm])

  // computed
  const isDecimalDisabled = foundTokenInfoByContractAddress?.isErc721 || false
  const tokenNameError = tokenName === ''
  const tokenSymbolError = tokenSymbol === ''
  const tokenDecimalsError = tokenDecimals === ''
  const customAssetsNetworkError = !customAssetsNetwork?.chainId
  const tokenContractAddressError =
    tokenContractAddress === '' ||
    (customAssetsNetwork?.coin !== BraveWallet.CoinType.SOL &&
      !tokenContractAddress.toLowerCase().startsWith('0x'))

  const buttonDisabled =
    tokenNameError ||
    tokenSymbolError ||
    tokenDecimalsError ||
    tokenContractAddressError ||
    customAssetsNetworkError

  // memos
  const formErrors = React.useMemo(() => {
    return [
      customAssetsNetworkError &&
        getLocale('braveWalletNetworkIsRequiredError'),
      tokenNameError && getLocale('braveWalletTokenNameIsRequiredError'),
      tokenContractAddressError &&
        getLocale('braveWalletInvalidTokenContractAddressError'),
      tokenSymbolError && getLocale('braveWalletTokenSymbolIsRequiredError'),
      tokenDecimalsError && getLocale('braveWalletTokenDecimalsIsRequiredError')
    ]
  }, [
    customAssetsNetworkError,
    tokenNameError,
    tokenContractAddressError,
    tokenSymbolError,
    tokenDecimalsError
  ])

  const tokenAlreadyExists = React.useMemo(() => {
    if (tokenContractAddress !== '' && customAssetsNetwork) {
      return combinedTokensList.some(
        (t) =>
          t.contractAddress.toLocaleLowerCase() ===
            tokenContractAddress.toLowerCase() &&
          t.chainId === customAssetsNetwork.chainId &&
          t.coin === customAssetsNetwork.coin &&
          t.visible
      )
    }
    return false
  }, [tokenContractAddress, combinedTokensList, customAssetsNetwork])

  // effects
  React.useEffect(() => {
    if (tokenContractAddress === '') {
      resetInputFields()
      return
    }
    onFindTokenInfoByContractAddress(tokenContractAddress)
    if (foundTokenInfoByContractAddress) {
      setTokenName(foundTokenInfoByContractAddress.name)
      setTokenSymbol(foundTokenInfoByContractAddress.symbol)
      setTokenDecimals(foundTokenInfoByContractAddress.decimals.toString())
      const network =
        networksRegistry.entities[
          networkEntityAdapter.selectId(foundTokenInfoByContractAddress)
        ]
      if (network) setCustomAssetsNetwork(network)
    }
    if (foundTokenInfoByContractAddress?.isErc721) {
      onNftAssetFound(foundTokenInfoByContractAddress.contractAddress)
    }
  }, [
    foundTokenInfoByContractAddress,
    tokenContractAddress,
    onFindTokenInfoByContractAddress,
    resetInputFields,
    networksRegistry,
    onNftAssetFound
  ])

  // render
  return (
    <>
      <FormWrapper onClick={onHideNetworkDropDown}>
        <InputLabel>{getLocale('braveWalletSelectNetwork')}</InputLabel>
        <SelectNetworkDropdown
          selectedNetwork={customAssetsNetwork}
          onClick={onShowNetworkDropDown}
          showNetworkDropDown={showNetworkDropDown}
          onSelectCustomNetwork={onSelectCustomNetwork}
        />
        <FormRow>
          <FormColumn>
            <InputLabel>
              {getLocale('braveWalletWatchListTokenName')}
            </InputLabel>
            <Input
              value={tokenName}
              onChange={handleTokenNameChanged}
            />
          </FormColumn>
          <FormColumn>
            <InputLabel>
              {customAssetsNetwork?.coin === BraveWallet.CoinType.SOL
                ? getLocale('braveWalletTokenMintAddress')
                : getLocale('braveWalletWatchListTokenAddress')}
            </InputLabel>
            <Input
              value={tokenContractAddress}
              onChange={handleTokenAddressChanged}
            />
          </FormColumn>
        </FormRow>
        <FormRow>
          <FormColumn>
            <InputLabel>
              {getLocale('braveWalletWatchListTokenSymbol')}
            </InputLabel>
            <Input
              value={tokenSymbol}
              onChange={handleTokenSymbolChanged}
            />
          </FormColumn>
          <FormColumn>
            <InputLabel>
              {getLocale('braveWalletWatchListTokenDecimals')}
            </InputLabel>
            <Input
              value={tokenDecimals}
              onChange={handleTokenDecimalsChanged}
              disabled={isDecimalDisabled}
              type='number'
            />
          </FormColumn>
        </FormRow>
        <DividerRow>
          <AdvancedButton onClick={onToggleShowAdvancedFields}>
            <DividerText>
              {getLocale('braveWalletWatchListAdvanced')}
            </DividerText>
          </AdvancedButton>
          <AdvancedButton onClick={onToggleShowAdvancedFields}>
            <AdvancedIcon rotated={showAdvancedFields} />
          </AdvancedButton>
        </DividerRow>
        <SubDivider />
        {showAdvancedFields && (
          <>
            <InputLabel>{getLocale('braveWalletIconURL')}</InputLabel>
            <Input
              value={iconURL}
              onChange={handleIconURLChanged}
            />
            <InputLabel>
              {getLocale('braveWalletWatchListCoingeckoId')}
            </InputLabel>
            <Input
              value={coingeckoId}
              onChange={handleCoingeckoIDChanged}
            />
          </>
        )}
        {hasError && (
          <ErrorText>{getLocale('braveWalletWatchListError')}</ErrorText>
        )}
        {tokenAlreadyExists && (
          <ErrorText>
            {getLocale('braveWalletCustomTokenExistsError')}
          </ErrorText>
        )}
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
          maxWidth={120}
          verticalPosition='above'
        >
          <AddButtonWrapper>
            <Button
              onClick={onClickAddCustomToken}
              isDisabled={buttonDisabled || tokenAlreadyExists}
            >
              {getLocale('braveWalletWatchListAdd')}
            </Button>
          </AddButtonWrapper>
        </Tooltip>
      </ButtonRow>
    </>
  )
}
