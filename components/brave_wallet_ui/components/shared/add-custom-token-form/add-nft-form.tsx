// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useSelector } from 'react-redux'

// utils
import {
  BraveWallet,
  WalletState
} from '../../../constants/types'
import Amount from '../../../utils/amount'
import { getLocale } from '$web-common/locale'
import {
  useAssetManagement,
  useLib,
  useTokenInfo
} from '../../../common/hooks'

// components
import { SelectNetworkDropdown } from '../../desktop'
import { NavButton } from '../../extension'
import Tooltip from '../tooltip'
import { FormErrorsList } from './form-errors-list'

// styles
import {
  ButtonRow,
  ErrorText,
  FormColumn,
  FormRow,
  FormWrapper,
  Input,
  InputLabel
} from './add-custom-token-form-styles'

interface Props {
  contractAddress: string
  onHideForm: () => void
  onTokenFound: (contractAddress: string) => void
  onChangeContractAddress: (contractAddress: string) => void
}

export const AddNftForm = (props: Props) => {
  const {
    contractAddress: tokenContractAddress,
    onHideForm,
    onTokenFound,
    onChangeContractAddress
  } = props

  // state
  const [showTokenIDRequired, setShowTokenIDRequired] = React.useState<boolean>(false)
  const [showNetworkDropDown, setShowNetworkDropDown] = React.useState<boolean>(false)

  // Form States
  const [tokenName, setTokenName] = React.useState<string>('')
  const [tokenID, setTokenID] = React.useState<string>('')
  const [tokenSymbol, setTokenSymbol] = React.useState<string>('')
  const [customAssetsNetwork, setCustomAssetsNetwork] = React.useState<BraveWallet.NetworkInfo>()

  // redux
  const userVisibleTokensInfo = useSelector(({ wallet }: { wallet: WalletState }) => wallet.userVisibleTokensInfo)
  const fullTokenList = useSelector(({ wallet }: { wallet: WalletState }) => wallet.fullTokenList)
  const selectedNetwork = useSelector(({ wallet }: { wallet: WalletState }) => wallet.selectedNetwork)
  const addUserAssetError = useSelector(({ wallet }: { wallet: WalletState }) => wallet.addUserAssetError)
  const networks = useSelector(({ wallet }: { wallet: WalletState }) => wallet.networkList)

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

  const handleTokenIDChanged = React.useCallback((event: React.ChangeEvent<HTMLInputElement>) => {
    setHasError(false)
    setShowTokenIDRequired(false)
    setTokenID(event.target.value)
  }, [])

  const handleTokenSymbolChanged = React.useCallback((event: React.ChangeEvent<HTMLInputElement>) => {
    setHasError(false)
    setTokenSymbol(event.target.value)
  }, [])

  const handleTokenAddressChanged = React.useCallback((event: React.ChangeEvent<HTMLInputElement>) => {
    setHasError(false)
    onChangeContractAddress(event.target.value)
  }, [onChangeContractAddress])

  // methods
  const resetInputFields = React.useCallback(() => {
    setTokenName('')
    onChangeContractAddress('')
    setTokenSymbol('')
    setTokenID('')
  }, [onChangeContractAddress])

  const onClickAddCustomToken = React.useCallback(() => {
    if (!customAssetsNetwork) return

    if (foundTokenInfoByContractAddress) {
      if (foundTokenInfoByContractAddress.isErc721) {
        let token = { ...foundTokenInfoByContractAddress }
        token.tokenId = tokenID ? new Amount(tokenID).toHex() : ''
        token.chainId = customAssetsNetwork.chainId
        onAddCustomAsset(token)
        onHideForm()
        return
      }
      let foundToken = { ...foundTokenInfoByContractAddress }
      foundToken.chainId = customAssetsNetwork.chainId
      onAddCustomAsset(foundToken)
    } else {
      const newToken: BraveWallet.BlockchainToken = {
        contractAddress: tokenContractAddress,
        decimals: 0,
        isErc20: customAssetsNetwork.coin !== BraveWallet.CoinType.SOL && !tokenID,
        isErc721: customAssetsNetwork.coin !== BraveWallet.CoinType.SOL && !!tokenID,
        isNft: true,
        name: tokenName,
        symbol: tokenSymbol,
        tokenId: tokenID ? new Amount(tokenID).toHex() : '',
        logo: '',
        visible: true,
        coingeckoId: '',
        chainId: customAssetsNetwork.chainId,
        coin: customAssetsNetwork.coin
      }
      onAddCustomAsset(newToken)
    }
    onHideForm()
  }, [tokenContractAddress, foundTokenInfoByContractAddress, customAssetsNetwork, tokenName, tokenSymbol, tokenID, onAddCustomAsset, onHideForm])

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
  const customAssetsNetworkError = !customAssetsNetwork?.chainId
  const tokenNameError = tokenName === ''
  const tokenSymbolError = tokenSymbol === ''
  const tokenContractAddressError =
    tokenContractAddress === '' ||
    (customAssetsNetwork?.coin !== BraveWallet.CoinType.SOL &&
      !tokenContractAddress.toLowerCase().startsWith('0x')) ||
    (customAssetsNetwork?.coin === BraveWallet.CoinType.ETH && tokenID === '')

  const buttonDisabled = tokenNameError ||
    tokenSymbolError ||
    tokenContractAddressError ||
    customAssetsNetworkError

  // memos
  const formErrors = React.useMemo(() => {
    return [
      tokenContractAddressError && getLocale('braveWalletInvalidTokenContractAddressError'),
      customAssetsNetworkError && getLocale('braveWalletNetworkIsRequiredError'),
      tokenNameError && getLocale('braveWalletTokenNameIsRequiredError'),
      tokenSymbolError && getLocale('braveWalletTokenSymbolIsRequiredError')
    ]
  }, [
    customAssetsNetworkError,
    tokenNameError,
    tokenContractAddressError,
    tokenSymbolError
  ])

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
      const network = networks.find(network => network.chainId.toLowerCase() === foundTokenInfoByContractAddress.chainId.toLowerCase())
      if (network) setCustomAssetsNetwork(network)
    }
    if (foundTokenInfoByContractAddress?.isErc20) {
      onTokenFound(tokenContractAddress)
    }
  }, [foundTokenInfoByContractAddress, onFindTokenInfoByContractAddress, tokenContractAddress, resetInputFields, networks, onTokenFound])

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
        {customAssetsNetwork?.coin !== BraveWallet.CoinType.SOL && (
          <FormColumn>
            <InputLabel>{getLocale('braveWalletWatchListTokenId')}</InputLabel>
            <Input
              value={tokenID}
              onChange={handleTokenIDChanged}
              type='number'
            />
          </FormColumn>
        )}
      </FormRow>
      <>
        {showTokenIDRequired && (
          <ErrorText>{getLocale('braveWalletWatchListTokenIdError')}</ErrorText>
        )}
      </>
      {hasError && (
        <ErrorText>{getLocale('braveWalletWatchListError')}</ErrorText>
      )}
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
            disabled={buttonDisabled}
          />
        </Tooltip>
      </ButtonRow>
    </FormWrapper>
  )
}
