// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { skipToken } from '@reduxjs/toolkit/query/react'
import Input, { InputEventDetail } from '@brave/leo/react/input'

// utils
import { getLocale } from '$web-common/locale'

// types
import { BraveWallet } from '../../../constants/types'

// hooks
import useGetTokenInfo from '../../../common/hooks/use-get-token-info'
import {
  useGetCustomAssetSupportedNetworks //
} from '../../../common/hooks/use_get_custom_asset_supported_networks'
import {
  useAddUserTokenMutation,
  useGetNetworksRegistryQuery,
  useUpdateUserTokenMutation
} from '../../../common/slices/api.slice'
import {
  emptyNetworksRegistry,
  networkEntityAdapter
} from '../../../common/slices/entities/network.entity'

// components
import Tooltip from '../tooltip'
import { FormErrorsList } from './form-errors-list'
import { NetworksDropdown } from '../dropdowns/networks_dropdown'

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
  FullWidthFormColumn,
  InputLabel,
  SubDivider
} from './add-custom-token-form-styles'
import { Column, LeoSquaredButton, Row } from '../style'

interface Props {
  selectedAsset?: BraveWallet.BlockchainToken
  contractAddress?: string
  onHideForm: () => void
  onChangeContractAddress?: (contractAddress: string) => void
}

export const AddCustomTokenForm = (props: Props) => {
  const {
    selectedAsset,
    contractAddress,
    onHideForm,
    onChangeContractAddress
  } = props

  // queries
  const { data: networksRegistry = emptyNetworksRegistry } =
    useGetNetworksRegistryQuery()
  const selectedAssetNetwork = selectedAsset
    ? networksRegistry.entities[networkEntityAdapter.selectId(selectedAsset)]
    : undefined

  // state
  const [showAdvancedFields, setShowAdvancedFields] =
    React.useState<boolean>(false)
  const [showNetworkDropDown, setShowNetworkDropDown] =
    React.useState<boolean>(false)

  // Form States
  const [customTokenName, setCustomTokenName] = React.useState<
    string | undefined
  >(selectedAsset?.name)
  const [customTokenSymbol, setCustomTokenSymbol] = React.useState<
    string | undefined
  >(selectedAsset?.symbol)
  const [customTokenDecimals, setCustomTokenDecimals] = React.useState<
    string | undefined
  >(selectedAsset?.decimals.toString())
  const [customCoingeckoId, setCustomCoingeckoId] = React.useState<
    string | undefined
  >(selectedAsset?.coingeckoId)
  const [customIconURL, setCustomIconURL] = React.useState<string | undefined>(
    selectedAsset?.logo
  )
  const [customAssetsNetwork, setCustomAssetsNetwork] = React.useState<
    BraveWallet.NetworkInfo | undefined
  >(selectedAssetNetwork)

  // more state
  const [hasError, setHasError] = React.useState<boolean>(false)

  // computed
  const tokenContractAddress = selectedAsset
    ? selectedAsset.contractAddress
    : contractAddress

  // mutations
  const [addUserToken] = useAddUserTokenMutation()
  const [updateUserToken] = useUpdateUserTokenMutation()

  // queries
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
        isCompressed: false,
        isErc20: customAssetsNetwork.coin !== BraveWallet.CoinType.SOL,
        isErc721: false,
        isErc1155: false,
        splTokenProgram: BraveWallet.SPLTokenProgram.kUnknown,
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
    (event: InputEventDetail) => {
      setHasError(false)
      setCustomTokenName(event.value)
    },
    []
  )

  const handleTokenSymbolChanged = React.useCallback(
    (event: InputEventDetail) => {
      setHasError(false)
      setCustomTokenSymbol(event.value)
    },
    []
  )

  const resetBaseInputFields = React.useCallback(() => {
    setCustomTokenName(undefined)
    setCustomTokenSymbol(undefined)
    setCustomTokenDecimals(undefined)
    setCustomCoingeckoId(undefined)
    setCustomIconURL(undefined)
  }, [])

  const resetInputFields = React.useCallback(() => {
    resetBaseInputFields()
    if (onChangeContractAddress) {
      onChangeContractAddress('')
    }
  }, [resetBaseInputFields, onChangeContractAddress])

  const handleTokenAddressChanged = React.useCallback(
    (event: InputEventDetail) => {
      setHasError(false)

      if (event.value === '') {
        resetInputFields()
        return
      }

      if (onChangeContractAddress) {
        onChangeContractAddress(event.value)
      }
    },
    [onChangeContractAddress, resetInputFields]
  )

  const handleTokenDecimalsChanged = React.useCallback(
    (event: InputEventDetail) => {
      setHasError(false)
      setCustomTokenDecimals(event.value)
    },
    []
  )

  const handleCoingeckoIDChanged = React.useCallback(
    (event: InputEventDetail) => {
      setHasError(false)
      setCustomCoingeckoId(event.value)
    },
    []
  )

  const handleIconURLChanged = React.useCallback((event: InputEventDetail) => {
    setHasError(false)
    setCustomIconURL(event.value)
  }, [])

  // methods
  const onClickAddCustomToken = React.useCallback(async () => {
    if (!tokenInfo) {
      return
    }

    if (tokenAlreadyExists && selectedAsset) {
      await updateUserToken({
        existingToken: selectedAsset,
        updatedToken: tokenInfo
      }).unwrap()
      onHideForm()
      return
    }

    try {
      await addUserToken(tokenInfo).unwrap()
      onHideForm()
    } catch (error) {
      setHasError(true)
    }
  }, [
    tokenInfo,
    addUserToken,
    onHideForm,
    updateUserToken,
    tokenAlreadyExists,
    selectedAsset
  ])

  const onToggleShowAdvancedFields = () =>
    setShowAdvancedFields((prev) => !prev)

  const onHideNetworkDropDown = React.useCallback(() => {
    if (showNetworkDropDown) {
      setShowNetworkDropDown(false)
    }
  }, [showNetworkDropDown])

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
      !tokenContractAddress?.toLowerCase().startsWith('0x'))

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
        <FullWidthFormColumn>
          <NetworksDropdown
            placeholder={getLocale('braveWalletSelectNetwork')}
            networks={networkList}
            onSelectNetwork={onSelectCustomNetwork}
            selectedNetwork={customAssetsNetwork}
            showAllNetworksOption={false}
            label={
              <InputLabel>{getLocale('braveWalletSelectNetwork')}</InputLabel>
            }
          />
        </FullWidthFormColumn>

        <FormRow>
          <FormColumn>
            <Input
              value={tokenContractAddress}
              onInput={handleTokenAddressChanged}
              placeholder={'0x099689220846644F87D1137665CDED7BF3422747'}
            >
              <Row
                gap='4px'
                justifyContent='flex-start'
              >
                <InputLabel>
                  {customAssetsNetwork?.coin === BraveWallet.CoinType.SOL
                    ? getLocale('braveWalletTokenMintAddress')
                    : getLocale('braveWalletNFTDetailContractAddress')}
                </InputLabel>
              </Row>
            </Input>
          </FormColumn>

          <FormColumn>
            <Input
              value={name}
              onInput={handleTokenNameChanged}
              disabled={isTokenInfoLoading}
            >
              <InputLabel>
                {getLocale('braveWalletWatchListTokenName')}
              </InputLabel>
            </Input>
          </FormColumn>
        </FormRow>

        <FormRow>
          <FormColumn>
            <Input
              value={symbol}
              onInput={handleTokenSymbolChanged}
              disabled={isTokenInfoLoading}
            >
              <InputLabel>
                {getLocale('braveWalletWatchListTokenSymbol')}
              </InputLabel>
            </Input>
          </FormColumn>
          <FormColumn>
            <Input
              value={decimals}
              onInput={handleTokenDecimalsChanged}
              disabled={isDecimalDisabled}
              type='number'
            >
              <InputLabel>
                {getLocale('braveWalletWatchListTokenDecimals')}
              </InputLabel>
            </Input>
          </FormColumn>
        </FormRow>

        <Column
          fullWidth
          gap={'12px'}
        >
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
            <FullWidthFormColumn>
              <Input
                value={iconURL}
                onInput={handleIconURLChanged}
              >
                <InputLabel>{getLocale('braveWalletIconURL')}</InputLabel>
              </Input>

              <Input
                value={coingeckoId}
                onInput={handleCoingeckoIDChanged}
                disabled={isTokenInfoLoading}
              >
                <InputLabel>
                  {getLocale('braveWalletWatchListCoingeckoId')}
                </InputLabel>
              </Input>
            </FullWidthFormColumn>
          )}
        </Column>

        {hasError && (
          <ErrorText>{getLocale('braveWalletWatchListError')}</ErrorText>
        )}

        {tokenAlreadyExists && !selectedAsset && (
          <ErrorText>
            {getLocale('braveWalletCustomTokenExistsError')}
          </ErrorText>
        )}
      </FormWrapper>

      <ButtonRow gap='16px'>
        <LeoSquaredButton
          onClick={onClickCancel}
          kind='outline'
        >
          {getLocale('braveWalletButtonCancel')}
        </LeoSquaredButton>

        <Tooltip
          text={<FormErrorsList errors={formErrors} />}
          isVisible={buttonDisabled}
          maxWidth={120}
          verticalPosition='above'
        >
          <Row>
            <LeoSquaredButton
              onClick={onClickAddCustomToken}
              isDisabled={
                buttonDisabled || (!selectedAsset && tokenAlreadyExists)
              }
            >
              {selectedAsset
                ? getLocale('braveWalletButtonSaveChanges')
                : getLocale('braveWalletWatchListAdd')}
            </LeoSquaredButton>
          </Row>
        </Tooltip>
      </ButtonRow>
    </>
  )
}
