// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'
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
import { getLocale } from '$web-common/locale'
import { SelectNetworkDropdown } from '../../desktop'
import {
  BraveWallet,
  WalletState
} from '../../../constants/types'
import { NavButton } from '../../extension'
import { useSelector } from 'react-redux'
import Amount from '../../../utils/amount'
import {
  useAssetManagement,
  useLib,
  useTokenInfo
} from '../../../common/hooks'

interface Props {
  contractAddress: string | undefined
  onHideForm: () => void
}

export const AddCustomTokenForm = (props: Props) => {
  const {
    contractAddress,
    onHideForm
  } = props

  // state
  const [showTokenIDRequired, setShowTokenIDRequired] = React.useState<boolean>(false)
  const [showAdvancedFields, setShowAdvancedFields] = React.useState<boolean>(false)
  const [showNetworkDropDown, setShowNetworkDropDown] = React.useState<boolean>(false)

  // Form States
  const [tokenName, setTokenName] = React.useState<string>('')
  const [tokenID, setTokenID] = React.useState<string>('')
  const [tokenSymbol, setTokenSymbol] = React.useState<string>('')
  const [tokenContractAddress, setTokenContractAddress] = React.useState<string>(contractAddress || '')
  const [tokenDecimals, setTokenDecimals] = React.useState<string>('')
  const [coingeckoID, setCoingeckoID] = React.useState<string>('')
  const [iconURL, setIconURL] = React.useState<string>('')
  const [customAssetsNetwork, setCustomAssetsNetwork] = React.useState<BraveWallet.NetworkInfo>()

  // redux
  const userVisibleTokensInfo = useSelector(({ wallet }: { wallet: WalletState }) => wallet.userVisibleTokensInfo)
  const fullTokenList = useSelector(({ wallet }: { wallet: WalletState }) => wallet.fullTokenList)
  const selectedNetwork = useSelector(({ wallet }: { wallet: WalletState }) => wallet.selectedNetwork)
  const addUserAssetError = useSelector(({ wallet }: { wallet: WalletState }) => wallet.addUserAssetError)

  // more state
  const [hasError, setHasError] = React.useState<boolean>(addUserAssetError)

  // custom hooks
  const { getBlockchainTokenInfo } = useLib()
  const {
    onFindTokenInfoByContractAddress,
    foundTokenInfoByContractAddress
  } = useTokenInfo(getBlockchainTokenInfo, userVisibleTokensInfo, fullTokenList, selectedNetwork)
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
    setTokenDecimals('0')
    setTokenID(event.target.value)
  }, [])

  const handleTokenSymbolChanged = React.useCallback((event: React.ChangeEvent<HTMLInputElement>) => {
    setHasError(false)
    setTokenSymbol(event.target.value)
  }, [])

  const handleTokenAddressChanged = React.useCallback((event: React.ChangeEvent<HTMLInputElement>) => {
    setHasError(false)
    setTokenContractAddress(event.target.value)
  }, [])

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
    setTokenContractAddress('')
    setTokenSymbol('')
    setTokenDecimals('')
    setTokenID('')
    setCoingeckoID('')
    setIconURL('')
  }, [])

  const onClickAddCustomToken = React.useCallback(() => {
    if (!customAssetsNetwork) return

    if (foundTokenInfoByContractAddress) {
      if (foundTokenInfoByContractAddress.isErc721) {
        let token = foundTokenInfoByContractAddress
        token.tokenId = tokenID ? new Amount(tokenID).toHex() : ''
        token.logo = token.logo ? token.logo : iconURL
        token.chainId = customAssetsNetwork.chainId
        onAddCustomAsset(token)
        return
      }
      let foundToken = foundTokenInfoByContractAddress
      foundToken.coingeckoId = coingeckoID !== '' ? coingeckoID : foundTokenInfoByContractAddress.coingeckoId
      foundToken.logo = foundToken.logo ? foundToken.logo : iconURL
      foundToken.chainId = customAssetsNetwork.chainId
      onAddCustomAsset(foundToken)
    } else {
      const newToken: BraveWallet.BlockchainToken = {
        contractAddress: tokenContractAddress,
        decimals: Number(tokenDecimals),
        isErc20: customAssetsNetwork.coin !== BraveWallet.CoinType.SOL && !tokenID,
        isErc721: customAssetsNetwork.coin !== BraveWallet.CoinType.SOL && !!tokenID,
        name: tokenName,
        symbol: tokenSymbol,
        tokenId: tokenID ? new Amount(tokenID).toHex() : '',
        logo: iconURL,
        visible: true,
        coingeckoId: coingeckoID,
        chainId: customAssetsNetwork.chainId,
        coin: customAssetsNetwork.coin
      }
      onAddCustomAsset(newToken)
    }
    onHideForm()
  }, [tokenContractAddress, foundTokenInfoByContractAddress, customAssetsNetwork, iconURL, tokenDecimals, tokenName, tokenSymbol, tokenID, coingeckoID, onAddCustomAsset])

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

  // memos
  const isDecimalDisabled = React.useMemo((): boolean => {
    return foundTokenInfoByContractAddress?.isErc721 ?? tokenID !== ''
  }, [foundTokenInfoByContractAddress, tokenID])

  const buttonDisabled = React.useMemo((): boolean => {
    return tokenName === '' ||
      tokenSymbol === '' ||
      (tokenDecimals === '0' && tokenID === '') ||
      tokenDecimals === '' ||
      tokenContractAddress === '' ||
      !customAssetsNetwork ||
      (customAssetsNetwork?.coin !== BraveWallet.CoinType.SOL &&
        !tokenContractAddress.toLowerCase().startsWith('0x'))
  }, [tokenName, tokenSymbol, tokenDecimals, tokenID, tokenContractAddress, customAssetsNetwork])

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
    }
    if (foundTokenInfoByContractAddress?.isErc721) {
      if (tokenID === '') {
        setShowTokenIDRequired(true)
        setShowAdvancedFields(true)
      }
    } else {
      setShowTokenIDRequired(false)
    }
  }, [foundTokenInfoByContractAddress, tokenID, tokenContractAddress, onFindTokenInfoByContractAddress, resetInputFields])

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
          <Input
            value={tokenName}
            onChange={handleTokenNameChanged}
          />
        </FormColumn>
        <FormColumn>
          <InputLabel>{getLocale('braveWalletWatchListTokenAddress')}</InputLabel>
          <Input
            value={tokenContractAddress}
            onChange={handleTokenAddressChanged}
          />
        </FormColumn>
      </FormRow>
      <FormRow>
        <FormColumn>
          <InputLabel>{getLocale('braveWalletWatchListTokenSymbol')}</InputLabel>
          <Input
            value={tokenSymbol}
            onChange={handleTokenSymbolChanged}
          />
        </FormColumn>
        <FormColumn>
          <InputLabel>{getLocale('braveWalletWatchListTokenDecimals')}</InputLabel>
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
      {showAdvancedFields &&
        <>
          <InputLabel>{getLocale('braveWalletIconURL')}</InputLabel>
          <Input
            value={iconURL}
            onChange={handleIconURLChanged}
          />
          <InputLabel>{getLocale('braveWalletWatchListCoingeckoId')}</InputLabel>
          <Input
            value={coingeckoID}
            onChange={handleCoingeckoIDChanged}
          />
          {customAssetsNetwork?.coin !== BraveWallet.CoinType.SOL &&
            <>
              <InputLabel>{getLocale('braveWalletWatchListTokenId')}</InputLabel>
              <Input
                value={tokenID}
                onChange={handleTokenIDChanged}
                type='number'
                disabled={Number(tokenDecimals) > 0}
              />
            </>
          }
          {showTokenIDRequired &&
            <ErrorText>{getLocale('braveWalletWatchListTokenIdError')}</ErrorText>
          }
        </>
      }
      {hasError &&
        <ErrorText>{getLocale('braveWalletWatchListError')}</ErrorText>
      }
      <ButtonRow>
        <NavButton
          onSubmit={onClickCancel}
          text={getLocale('braveWalletButtonCancel')}
          buttonType='secondary'
        />
        <NavButton
          onSubmit={onClickAddCustomToken}
          text={getLocale('braveWalletWatchListAdd')}
          buttonType='primary'
          disabled={buttonDisabled}
        />
      </ButtonRow>
    </FormWrapper>
  )
}
