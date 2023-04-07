// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useDispatch } from 'react-redux'

// utils
import { BraveWallet } from '../../../constants/types'
import Amount from '../../../utils/amount'
import { getLocale } from '$web-common/locale'
import { WalletActions } from '../../../common/actions'
import { stripERC20TokenImageURL } from '../../../utils/string-utils'
import {
  networkEntityAdapter,
  emptyNetworksRegistry
} from '../../../common/slices/entities/network.entity'

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
import {
  useSafeWalletSelector,
  useUnsafeWalletSelector
} from '../../../common/hooks/use-safe-selector'
import { WalletSelectors } from '../../../common/selectors'

// components
import { SelectNetworkDropdown } from '../../desktop'
import { NavButton } from '../../extension'
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
  InputLabel
} from './add-custom-token-form-styles'

interface Props {
  contractAddress: string
  selectedAsset?: BraveWallet.BlockchainToken
  onHideForm: () => void
  onTokenFound?: (contractAddress: string) => void
  onChangeContractAddress?: (contractAddress: string) => void
}

export const AddNftForm = (props: Props) => {
  const {
    selectedAsset,
    contractAddress: tokenContractAddress,
    onHideForm,
    onTokenFound,
    onChangeContractAddress
  } = props

  // redux
  const dispatch = useDispatch()
  const fullTokenList = useUnsafeWalletSelector(WalletSelectors.fullTokenList)
  const userVisibleTokensInfo = useUnsafeWalletSelector(
    WalletSelectors.userVisibleTokensInfo
  )
  const addUserAssetError = useSafeWalletSelector(
    WalletSelectors.addUserAssetError
  )

  // queries
  const { data: selectedNetwork } = useGetSelectedChainQuery()
  const { data: networksRegistry = emptyNetworksRegistry } =
    useGetNetworksRegistryQuery()

  const selectedAssetNetwork = selectedAsset
    ? networksRegistry.entities[networkEntityAdapter.selectId(selectedAsset)]
    : undefined

  // state
  const [showTokenIDRequired, setShowTokenIDRequired] = React.useState<boolean>(false)
  const [showNetworkDropDown, setShowNetworkDropDown] = React.useState<boolean>(false)
  const [hasError, setHasError] = React.useState<boolean>(addUserAssetError)

  // Form States
  const [tokenName, setTokenName] = React.useState<string>(selectedAsset?.name || '')
  const [tokenID, setTokenID] = React.useState<string>(selectedAsset?.tokenId ? parseInt(selectedAsset.tokenId, 16).toString() : '')
  const [tokenSymbol, setTokenSymbol] = React.useState<string>(selectedAsset?.symbol || '')
  const [customAssetsNetwork, setCustomAssetsNetwork] = React.useState<
    BraveWallet.NetworkInfo | undefined
  >(selectedAssetNetwork)


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
    if (onChangeContractAddress) {
      onChangeContractAddress(event.target.value)
    }
  }, [onChangeContractAddress])

  // methods
  const resetInputFields = React.useCallback(() => {
    setTokenName('')
    if (onChangeContractAddress) {
      onChangeContractAddress('')
    }
    setTokenSymbol('')
    setTokenID('')
  }, [onChangeContractAddress])

  const addOrUpdateToken = React.useCallback(() => {
     if (selectedAsset) {
       if (!customAssetsNetwork) return

      const token = {
        ...selectedAsset,
        contractAddress: tokenContractAddress,
        name: tokenName,
        symbol: tokenSymbol,
        tokenId: tokenID ? new Amount(tokenID).toHex() : '',
        chainId: customAssetsNetwork.chainId,
        coin: customAssetsNetwork.coin,
        logo: stripERC20TokenImageURL(selectedAsset.logo) || ''
      }
      dispatch(WalletActions.updateUserAsset({
        existing: selectedAsset,
        updated: token
      }))
      onHideForm()
    } else {
      onAddCustomToken()
    }
  }, [selectedAsset, tokenContractAddress, tokenName, tokenSymbol, customAssetsNetwork, tokenID])

  const onAddCustomToken = React.useCallback(() => {
    if (!customAssetsNetwork) return

    if (foundTokenInfoByContractAddress) {
      if (foundTokenInfoByContractAddress.isErc721) {
        let token = { ...foundTokenInfoByContractAddress }
        token.tokenId = tokenID ? new Amount(tokenID).toHex() : ''
        token.chainId = customAssetsNetwork.chainId
        token.name = tokenName
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
        isErc1155: false,
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
      const network =
        networksRegistry.entities[
          networkEntityAdapter.selectId(foundTokenInfoByContractAddress)
        ]
      if (network) setCustomAssetsNetwork(network)
    }
    if (foundTokenInfoByContractAddress?.isErc20 && onTokenFound) {
      onTokenFound(tokenContractAddress)
    }
  }, [
    foundTokenInfoByContractAddress,
    onFindTokenInfoByContractAddress,
    tokenContractAddress,
    resetInputFields,
    networksRegistry,
    onTokenFound
  ])

  return (
    <FormWrapper onClick={onHideNetworkDropDown}>
      <FullWidthFormColumn>
        <InputLabel>{getLocale('braveWalletWatchListTokenAddress')}</InputLabel>
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
        />
      </FullWidthFormColumn>
      <FullWidthFormColumn>
        <InputLabel>{getLocale('braveWalletWatchListTokenName')}</InputLabel>
        <Input
          value={tokenName}
          onChange={handleTokenNameChanged}
          width='100%'
        />
      </FullWidthFormColumn>
      <FullWidthFormColumn>
        <InputLabel>{getLocale('braveWalletWatchListTokenSymbol')}</InputLabel>
        <Input
          value={tokenSymbol}
          onChange={handleTokenSymbolChanged}
          width='100%'
        />
      </FullWidthFormColumn>
      <FullWidthFormColumn>
        {customAssetsNetwork?.coin !== BraveWallet.CoinType.SOL &&
          <>
            <InputLabel>{getLocale('braveWalletWatchListTokenId')}</InputLabel>
            <Input
              value={tokenID}
              onChange={handleTokenIDChanged}
              type='number'
              width='100%'
            />
          </>
        }
      </FullWidthFormColumn>
      <>
        {showTokenIDRequired &&
          <ErrorText>{getLocale('braveWalletWatchListTokenIdError')}</ErrorText>
          }
      </>
      {hasError &&
        <ErrorText>{getLocale('braveWalletWatchListError')}</ErrorText>
      }
      <ButtonRowSpacer />
      <ButtonRow style={{ justifyContent: selectedAsset ? 'center' : 'flex-start' }}>
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
            onSubmit={addOrUpdateToken}
            text={selectedAsset ? 'Save changes' : getLocale('braveWalletWatchListAdd')}
            buttonType='primary'
            disabled={buttonDisabled}
          />
        </Tooltip>
      </ButtonRow>
    </FormWrapper>
  )
}
