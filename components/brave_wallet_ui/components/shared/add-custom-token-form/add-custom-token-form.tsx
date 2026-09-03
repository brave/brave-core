// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { skipToken } from '@reduxjs/toolkit/query/react'
import Input, { InputEventDetail } from '@brave/leo/react/input'
import Button from '@brave/leo/react/button'

// utils
import { getLocale } from '$web-common/locale'

// types
import { BraveWallet } from '../../../constants/types'

// hooks
import useGetTokenInfo from '../../../common/hooks/use-get-token-info'
import {
  useGetCustomAssetSupportedNetworks, //
} from '../../../common/hooks/use_get_custom_asset_supported_networks'
import {
  useAddUserTokenMutation,
  useGetNetworksRegistryQuery,
  useUpdateUserTokenMutation,
} from '../../../common/slices/api.slice'
import {
  emptyNetworksRegistry,
  networkEntityAdapter,
} from '../../../common/slices/entities/network.entity'

// components
import Tooltip from '../tooltip'
import { FormErrorsList } from './form-errors-list'
import { NetworksDropdown } from '../dropdowns/networks_dropdown'
import { NumberInput } from '../number_input/number_input'

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
  SubDivider,
} from './add-custom-token-form-styles'
import { Column, Row } from '../style'

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
    onChangeContractAddress,
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
    selectedAsset?.logo,
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
    isLoading: isTokenInfoLoading,
  } = useGetTokenInfo(
    customAssetsNetwork && tokenContractAddress
      ? {
          contractAddress: tokenContractAddress,
          network: {
            chainId: customAssetsNetwork.chainId,
            coin: customAssetsNetwork.coin,
          },
        }
      : skipToken,
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
        zcashTokenType: BraveWallet.ZCashTokenType.kNone,
        visible: true,
      }
    }, [
      customAssetsNetwork,
      tokenContractAddress,
      name,
      symbol,
      decimals,
      coingeckoId,
      iconURL,
    ])

  // Handle Form Input Changes
  const handleTokenNameChanged = React.useCallback(
    (event: InputEventDetail) => {
      setHasError(false)
      setCustomTokenName(event.value)
    },
    [],
  )

  const handleTokenSymbolChanged = React.useCallback(
    (event: InputEventDetail) => {
      setHasError(false)
      setCustomTokenSymbol(event.value)
    },
    [],
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
    [onChangeContractAddress, resetInputFields],
  )

  const handleTokenDecimalsChanged = React.useCallback(
    (event: InputEventDetail) => {
      setHasError(false)
      setCustomTokenDecimals(event.value)
    },
    [],
  )

  const handleCoingeckoIDChanged = React.useCallback(
    (event: InputEventDetail) => {
      setHasError(false)
      setCustomCoingeckoId(event.value)
    },
    [],
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
        updatedToken: tokenInfo,
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
    selectedAsset,
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
    [setCustomAssetsNetwork, onHideNetworkDropDown, resetBaseInputFields],
  )

  const onClickCancel = React.useCallback(() => {
    resetInputFields()
    onHideForm()
  }, [resetInputFields, onHideForm])

  // computed
  const isDecimalDisabled =
    isTokenInfoLoading
    || tokenInfo?.isErc721
    || tokenInfo?.isErc1155
    || tokenInfo?.isNft
  const tokenNameError = !tokenInfo?.name
  const tokenSymbolError = !tokenInfo?.symbol
  const tokenDecimalsError = decimals === '' || Number(decimals) === 0
  const customAssetsNetworkError = !tokenInfo?.chainId
  const tokenContractAddressError =
    tokenInfo?.contractAddress === ''
    || (tokenInfo?.coin !== BraveWallet.CoinType.SOL
      && !tokenContractAddress?.toLowerCase().startsWith('0x'))

  const buttonDisabled =
    isTokenInfoLoading
    || tokenNameError
    || tokenSymbolError
    || tokenDecimalsError
    || tokenContractAddressError
    || customAssetsNetworkError

  // memos
  const formErrors = React.useMemo(() => {
    return [
      customAssetsNetworkError
        && getLocale(S.BRAVE_WALLET_NETWORK_IS_REQUIRED_ERROR),
      tokenNameError && getLocale(S.BRAVE_WALLET_TOKEN_NAME_IS_REQUIRED_ERROR),
      tokenContractAddressError
        && getLocale(S.BRAVE_WALLET_INVALID_TOKEN_CONTRACT_ADDRESS_ERROR),
      tokenSymbolError
        && getLocale(S.BRAVE_WALLET_TOKEN_SYMBOL_IS_REQUIRED_ERROR),
      tokenDecimalsError
        && getLocale(S.BRAVE_WALLET_TOKEN_DECIMALS_IS_REQUIRED_ERROR),
    ]
  }, [
    customAssetsNetworkError,
    tokenNameError,
    tokenContractAddressError,
    tokenSymbolError,
    tokenDecimalsError,
  ])

  // render
  return (
    <>
      <FormWrapper onClick={onHideNetworkDropDown}>
        <FullWidthFormColumn>
          <NetworksDropdown
            placeholder={getLocale(S.BRAVE_WALLET_SELECT_NETWORK)}
            networks={networkList}
            onSelectNetwork={onSelectCustomNetwork}
            selectedNetwork={customAssetsNetwork}
            showAllNetworksOption={false}
            label={
              <InputLabel>
                {getLocale(S.BRAVE_WALLET_SELECT_NETWORK)}
              </InputLabel>
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
                    ? getLocale(S.BRAVE_WALLET_TOKEN_MINT_ADDRESS)
                    : getLocale(S.BRAVE_WALLET_NFT_DETAIL_CONTRACT_ADDRESS)}
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
                {getLocale(S.BRAVE_WALLET_WATCH_LIST_TOKEN_NAME)}
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
                {getLocale(S.BRAVE_WALLET_WATCH_LIST_TOKEN_SYMBOL)}
              </InputLabel>
            </Input>
          </FormColumn>
          <FormColumn>
            <NumberInput
              value={decimals}
              onInput={handleTokenDecimalsChanged}
              disabled={isDecimalDisabled}
            >
              <InputLabel>
                {getLocale(S.BRAVE_WALLET_WATCH_LIST_TOKEN_DECIMALS)}
              </InputLabel>
            </NumberInput>
          </FormColumn>
        </FormRow>

        <Column
          fullWidth
          gap={'12px'}
        >
          <DividerRow>
            <AdvancedButton onClick={onToggleShowAdvancedFields}>
              <DividerText
                textColor='tertiary'
                variant='default.semibold'
                isBold={true}
              >
                {getLocale(S.BRAVE_WALLET_WATCH_LIST_ADVANCED)}
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
                <InputLabel>{getLocale(S.BRAVE_WALLET_ICON_URL)}</InputLabel>
              </Input>

              <Input
                value={coingeckoId}
                onInput={handleCoingeckoIDChanged}
                disabled={isTokenInfoLoading}
              >
                <InputLabel>
                  {getLocale(S.BRAVE_WALLET_WATCH_LIST_COINGECKO_ID)}
                </InputLabel>
              </Input>
            </FullWidthFormColumn>
          )}
        </Column>

        {hasError && (
          <ErrorText
            textColor='error'
            textAlign='left'
            variant='small.regular'
          >
            {getLocale(S.BRAVE_WALLET_WATCH_LIST_ERROR)}
          </ErrorText>
        )}

        {tokenAlreadyExists && !selectedAsset && (
          <ErrorText
            textColor='error'
            textAlign='left'
            variant='small.regular'
          >
            {getLocale(S.BRAVE_WALLET_CUSTOM_TOKEN_EXISTS_ERROR)}
          </ErrorText>
        )}
      </FormWrapper>

      <ButtonRow gap='16px'>
        <Button
          onClick={onClickCancel}
          kind='outline'
        >
          {getLocale(S.BRAVE_WALLET_BUTTON_CANCEL)}
        </Button>

        <Tooltip
          text={<FormErrorsList errors={formErrors} />}
          isVisible={buttonDisabled}
          maxWidth={120}
          verticalPosition='above'
        >
          <Row>
            <Button
              onClick={onClickAddCustomToken}
              isDisabled={
                buttonDisabled || (!selectedAsset && tokenAlreadyExists)
              }
            >
              {selectedAsset
                ? getLocale(S.BRAVE_WALLET_BUTTON_SAVE_CHANGES)
                : getLocale(S.BRAVE_WALLET_WATCH_LIST_ADD)}
            </Button>
          </Row>
        </Tooltip>
      </ButtonRow>
    </>
  )
}
