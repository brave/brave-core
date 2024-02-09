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

// types
import { BraveWallet, WalletState } from '../../../constants/types'

// hooks
import useAssetManagement from '../../../common/hooks/assets-management'
import useGetTokenInfo from '../../../common/hooks/use-get-token-info'
import {
  useGetCustomAssetSupportedNetworks //
} from '../../../common/hooks/use_get_custom_asset_supported_networks'

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
  onChangeContractAddress: (contractAddress: string) => void
}

export const AddCustomTokenForm = (props: Props) => {
  const {
    contractAddress: tokenContractAddress,
    onHideForm,
    onChangeContractAddress
  } = props

  // state
  const [showAdvancedFields, setShowAdvancedFields] =
    React.useState<boolean>(false)
  const [showNetworkDropDown, setShowNetworkDropDown] =
    React.useState<boolean>(false)

  // Form States
  const [customTokenName, setCustomTokenName] = React.useState<string>()
  const [customTokenSymbol, setCustomTokenSymbol] = React.useState<string>()
  const [customTokenDecimals, setCustomTokenDecimals] = React.useState<string>()
  const [customCoingeckoId, setCustomCoingeckoId] = React.useState<string>()
  const [customIconURL, setCustomIconURL] = React.useState<string>()
  const [customAssetsNetwork, setCustomAssetsNetwork] =
    React.useState<BraveWallet.NetworkInfo>()

  // redux
  const addUserAssetError = useSelector(
    ({ wallet }: { wallet: WalletState }) => wallet.addUserAssetError
  )

  // more state
  const [hasError, setHasError] = React.useState<boolean>(addUserAssetError)

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

  const decimals =
    customTokenDecimals ?? matchedTokenInfo?.decimals.toFixed() ?? ''
  const name = customTokenName ?? matchedTokenInfo?.name ?? ''
  const symbol = customTokenSymbol ?? matchedTokenInfo?.symbol ?? ''
  const coingeckoId = customCoingeckoId ?? matchedTokenInfo?.coingeckoId ?? ''
  const iconURL = customIconURL ?? matchedTokenInfo?.logo ?? ''

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
        decimals: Number(decimals),
        coingeckoId,
        logo: iconURL,
        tokenId: '',
        isErc20: customAssetsNetwork.coin !== BraveWallet.CoinType.SOL,
        isErc721: false,
        isErc1155: false,
        isNft: false,
        isSpam: false,
        visible: true
      }
    }, [
      customAssetsNetwork,
      tokenContractAddress,
      name,
      symbol,
      decimals,
      coingeckoId,
      iconURL
    ])

  // Handle Form Input Changes
  const handleTokenNameChanged = React.useCallback(
    (event: React.ChangeEvent<HTMLInputElement>) => {
      setHasError(false)
      setCustomTokenName(event.target.value)
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
    [onChangeContractAddress]
  )

  const handleTokenDecimalsChanged = React.useCallback(
    (event: React.ChangeEvent<HTMLInputElement>) => {
      setHasError(false)
      setCustomTokenDecimals(event.target.value)
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
      setCustomIconURL(event.target.value)
    },
    []
  )

  // methods
  const resetBaseInputFields = React.useCallback(() => {
    setCustomTokenName(undefined)
    setCustomTokenSymbol(undefined)
    setCustomTokenDecimals(undefined)
    setCustomCoingeckoId(undefined)
    setCustomIconURL(undefined)
  }, [])

  const resetInputFields = React.useCallback(() => {
    resetBaseInputFields()
    onChangeContractAddress('')
  }, [resetBaseInputFields, onChangeContractAddress])

  const onClickAddCustomToken = React.useCallback(() => {
    if (!tokenInfo) {
      return
    }
    onAddCustomAsset(tokenInfo)
    onHideForm()
  }, [tokenInfo, onAddCustomAsset, onHideForm])

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
      resetBaseInputFields()
      setCustomAssetsNetwork(network)
      onHideNetworkDropDown()
    },
    [setCustomAssetsNetwork, onHideNetworkDropDown, resetBaseInputFields]
  )

  const onClickCancel = React.useCallback(() => {
    resetInputFields()
    onHideForm()
  }, [resetInputFields, onHideForm])

  // computed
  const isDecimalDisabled =
    isTokenInfoLoading ||
    tokenInfo?.isErc721 ||
    tokenInfo?.isErc1155 ||
    tokenInfo?.isNft
  const tokenNameError = !tokenInfo?.name
  const tokenSymbolError = !tokenInfo?.symbol
  const tokenDecimalsError = decimals === '' || Number(decimals) === 0
  const customAssetsNetworkError = !tokenInfo?.chainId
  const tokenContractAddressError =
    tokenInfo?.contractAddress === '' ||
    (tokenInfo?.coin !== BraveWallet.CoinType.SOL &&
      !tokenContractAddress.toLowerCase().startsWith('0x'))

  const buttonDisabled =
    isTokenInfoLoading ||
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
          networkListSubset={networkList}
        />
        <FormRow>
          <FormColumn>
            <InputLabel>
              {getLocale('braveWalletWatchListTokenName')}
            </InputLabel>
            <Input
              value={name}
              onChange={handleTokenNameChanged}
              disabled={isTokenInfoLoading}
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
              value={symbol}
              onChange={handleTokenSymbolChanged}
              disabled={isTokenInfoLoading}
            />
          </FormColumn>
          <FormColumn>
            <InputLabel>
              {getLocale('braveWalletWatchListTokenDecimals')}
            </InputLabel>
            <Input
              value={decimals}
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
              disabled={isTokenInfoLoading}
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
