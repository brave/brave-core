// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useSelector } from 'react-redux'

// utils
import { getLocale } from '$web-common/locale'
import {
  emptyNetworksRegistry,
  networkEntityAdapter
} from '../../../common/slices/entities/network.entity'

// types
import {
  BraveWallet,
  WalletState
} from '../../../constants/types'

// hooks
import {
  useAssetManagement,
  useLib,
  useTokenInfo
} from '../../../common/hooks'
import {
  useGetNetworksRegistryQuery,
  useGetSelectedChainQuery
} from '../../../common/slices/api.slice'

// components
import { SelectNetworkDropdown } from '../../desktop'
import { NavButton } from '../../extension'
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
  SubDivider
} from './add-custom-token-form-styles'


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
  const { data: selectedNetwork } = useGetSelectedChainQuery()
  const { data: networksRegistry = emptyNetworksRegistry } =
    useGetNetworksRegistryQuery()

  // state
  const [showAdvancedFields, setShowAdvancedFields] = React.useState<boolean>(false)
  const [showNetworkDropDown, setShowNetworkDropDown] = React.useState<boolean>(false)

  // Form States
  const [tokenName, setTokenName] = React.useState<string>('')
  const [tokenSymbol, setTokenSymbol] = React.useState<string>('')
  const [tokenDecimals, setTokenDecimals] = React.useState<string>('')
  const [coingeckoID, setCoingeckoID] = React.useState<string>('')
  const [iconURL, setIconURL] = React.useState<string>('')
  const [customAssetsNetwork, setCustomAssetsNetwork] = React.useState<BraveWallet.NetworkInfo>()

  // redux
  const userVisibleTokensInfo = useSelector(({ wallet }: { wallet: WalletState }) => wallet.userVisibleTokensInfo)
  const fullTokenList = useSelector(({ wallet }: { wallet: WalletState }) => wallet.fullTokenList)
  const addUserAssetError = useSelector(({ wallet }: { wallet: WalletState }) => wallet.addUserAssetError)

  // more state
  const [hasError, setHasError] = React.useState<boolean>(addUserAssetError)

  // custom hooks
  const { getBlockchainTokenInfo } = useLib()
  const {
    onFindTokenInfoByContractAddress,
    foundTokenInfoByContractAddress
  } = useTokenInfo(getBlockchainTokenInfo, userVisibleTokensInfo, fullTokenList, customAssetsNetwork || selectedNetwork)
  const {
    onAddCustomAsset
  } = useAssetManagement()

  // Handle Form Input Changes
  const handleTokenNameChanged = React.useCallback((event: React.ChangeEvent<HTMLInputElement>) => {
    setHasError(false)
    setTokenName(event.target.value)
  }, [])

  const handleTokenSymbolChanged = React.useCallback((event: React.ChangeEvent<HTMLInputElement>) => {
    setHasError(false)
    setTokenSymbol(event.target.value)
  }, [])

  const handleTokenAddressChanged = React.useCallback((event: React.ChangeEvent<HTMLInputElement>) => {
    setHasError(false)
    onChangeContractAddress(event.target.value)
  }, [onChangeContractAddress])

  const handleTokenDecimalsChanged = React.useCallback((event: React.ChangeEvent<HTMLInputElement>) => {
    setHasError(false)
    setTokenDecimals(event.target.value)
  }, [])

  const handleCoingeckoIDChanged = React.useCallback((event: React.ChangeEvent<HTMLInputElement>) => {
    setHasError(false)
    setCoingeckoID(event.target.value)
  }, [])

  const handleIconURLChanged = React.useCallback((event: React.ChangeEvent<HTMLInputElement>) => {
    setHasError(false)
    setIconURL(event.target.value)
  }, [])

  // methods
  const resetInputFields = React.useCallback(() => {
    setTokenName('')
    onChangeContractAddress('')
    setTokenSymbol('')
    setTokenDecimals('')
    setCoingeckoID('')
    setIconURL('')
  }, [onChangeContractAddress])

  const onClickAddCustomToken = React.useCallback(() => {
    if (!customAssetsNetwork) return

    if (foundTokenInfoByContractAddress) {
      if (foundTokenInfoByContractAddress.isErc721) {
        onNftAssetFound(foundTokenInfoByContractAddress.contractAddress)
      }
      let foundToken = { ...foundTokenInfoByContractAddress }
      foundToken.coingeckoId = coingeckoID !== '' ? coingeckoID : foundTokenInfoByContractAddress.coingeckoId
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
        name: tokenName,
        symbol: tokenSymbol,
        tokenId: '',
        logo: iconURL,
        visible: true,
        coingeckoId: coingeckoID,
        chainId: customAssetsNetwork.chainId,
        coin: customAssetsNetwork.coin
      }
      onAddCustomAsset(newToken)
    }
    onHideForm()
  }, [tokenContractAddress, foundTokenInfoByContractAddress, customAssetsNetwork, iconURL, tokenDecimals, tokenName, tokenSymbol, coingeckoID, onAddCustomAsset, onHideForm, onNftAssetFound])

  const onToggleShowAdvancedFields = () => setShowAdvancedFields(prev => !prev)

  const onHideNetworkDropDown = React.useCallback(() => {
    if (showNetworkDropDown) {
      setShowNetworkDropDown(false)
    }
  }, [showNetworkDropDown])

  const onShowNetworkDropDown = React.useCallback(() => {
    setShowNetworkDropDown(true)
  }, [])

  const onSelectCustomNetwork = React.useCallback((network: BraveWallet.NetworkInfo) => {
    setCustomAssetsNetwork(network)
    onHideNetworkDropDown()
  }, [onHideNetworkDropDown])

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
    (
      customAssetsNetwork?.coin !== BraveWallet.CoinType.SOL &&
      !tokenContractAddress.toLowerCase().startsWith('0x')
    )

  const buttonDisabled = tokenNameError ||
    tokenSymbolError ||
    tokenDecimalsError ||
    tokenContractAddressError ||
    customAssetsNetworkError

  // memos
  const formErrors = React.useMemo(() => {
    return [
      customAssetsNetworkError && getLocale('braveWalletNetworkIsRequiredError'),
      tokenNameError && getLocale('braveWalletTokenNameIsRequiredError'),
      tokenContractAddressError && getLocale('braveWalletInvalidTokenContractAddressError'),
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
    if (tokenContractAddress !== '' && customAssetsNetwork?.chainId !== undefined) {
      return userVisibleTokensInfo.some((t) =>
        t.contractAddress.toLocaleLowerCase() === tokenContractAddress.toLowerCase() &&
        t.chainId === customAssetsNetwork?.chainId
      )
    }
    return false
  }, [tokenContractAddress, userVisibleTokensInfo, customAssetsNetwork?.chainId])

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
          <InputLabel>{getLocale('braveWalletWatchListTokenName')}</InputLabel>
          <Input value={tokenName} onChange={handleTokenNameChanged} />
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
          <Input value={tokenSymbol} onChange={handleTokenSymbolChanged} />
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
          <DividerText>{getLocale('braveWalletWatchListAdvanced')}</DividerText>
        </AdvancedButton>
        <AdvancedButton onClick={onToggleShowAdvancedFields}>
          <AdvancedIcon rotated={showAdvancedFields} />
        </AdvancedButton>
      </DividerRow>
      <SubDivider />
      {showAdvancedFields && (
        <>
          <InputLabel>{getLocale('braveWalletIconURL')}</InputLabel>
          <Input value={iconURL} onChange={handleIconURLChanged} />
          <InputLabel>
            {getLocale('braveWalletWatchListCoingeckoId')}
          </InputLabel>
          <Input value={coingeckoID} onChange={handleCoingeckoIDChanged} />
        </>
      )}
      {hasError && (
        <ErrorText>{getLocale('braveWalletWatchListError')}</ErrorText>
      )}
      {tokenAlreadyExists &&
        <ErrorText>{getLocale('braveWalletCustomTokenExistsError')}</ErrorText>
      }
      <ButtonRow>
        <NavButton
          onSubmit={onClickCancel}
          text={getLocale('braveWalletButtonCancel')}
          buttonType='secondary'
        />
        <Tooltip
          text={<FormErrorsList errors={formErrors} />}
          isVisible={buttonDisabled}
          verticalPosition='above'
        >
          <NavButton
            onSubmit={onClickAddCustomToken}
            text={getLocale('braveWalletWatchListAdd')}
            buttonType='primary'
            disabled={buttonDisabled || tokenAlreadyExists}
          />
        </Tooltip>
      </ButtonRow>
    </FormWrapper>
  )
}
