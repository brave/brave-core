// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { skipToken } from '@reduxjs/toolkit/query/react'
import Input, { InputEventDetail } from '@brave/leo/react/input'
import Alert from '@brave/leo/react/alert'

// utils
import { BraveWallet } from '../../../constants/types'
import Amount from '../../../utils/amount'
import { getLocale } from '$web-common/locale'
import {
  networkEntityAdapter,
  emptyNetworksRegistry
} from '../../../common/slices/entities/network.entity'
import withPlaceholderIcon from '../create-placeholder-icon'
import { getAssetIdKey } from '../../../utils/asset-utils'

// hooks
import useGetTokenInfo from '../../../common/hooks/use-get-token-info'
import {
  useAddUserTokenMutation,
  useGetIsTokenOwnedByUserQuery,
  useGetNetworksRegistryQuery,
  useGetNftMetadataQuery,
  useUpdateUserTokenMutation
} from '../../../common/slices/api.slice'
import {
  useGetCustomAssetSupportedNetworks //
} from '../../../common/hooks/use_get_custom_asset_supported_networks'

// components
import { NetworksDropdown } from '../dropdowns/networks_dropdown'
import Tooltip from '../tooltip'
import { FormErrorsList } from './form-errors-list'
import { NftIcon } from '../nft-icon/nft-icon'
import { InfoIconTooltip } from '../info_icon_tooltip/info_icon_tooltip'

// styles
import {
  ButtonRow,
  ButtonRowSpacer,
  ErrorText,
  FormWrapper,
  FullWidthFormColumn,
  InputLabel,
  PreviewImageContainer,
  TokenNamePreviewText,
  TokenTickerPreviewText
} from './add-custom-token-form-styles'
import { Column, LeoSquaredButton, Row } from '../style'
import { Skeleton } from '../loading-skeleton/styles'

const NftIconWithPlaceholder = withPlaceholderIcon(NftIcon, {
  size: 'extra-big',
  marginLeft: 0,
  marginRight: 0
})

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

  const { data: networksRegistry = emptyNetworksRegistry } =
    useGetNetworksRegistryQuery()
  const selectedAssetNetwork = selectedAsset
    ? networksRegistry.entities[networkEntityAdapter.selectId(selectedAsset)]
    : undefined

  // state
  const [showNetworkDropDown, setShowNetworkDropDown] =
    React.useState<boolean>(false)
  const [hasError, setHasError] = React.useState<boolean>(false)

  // Form States
  const [customTokenID, setCustomTokenID] = React.useState<string | undefined>(
    selectedAsset?.tokenId
  )
  const [customAssetsNetwork, setCustomAssetsNetwork] = React.useState<
    BraveWallet.NetworkInfo | undefined
  >(selectedAssetNetwork)

  // mutations
  const [addUserToken] = useAddUserTokenMutation()
  const [updateUserToken] = useUpdateUserTokenMutation()

  // queries
  const {
    tokenInfo: matchedTokenInfo,
    isVisible: tokenAlreadyExists,
    isLoading: isTokenInfoLoading
  } = useGetTokenInfo(
    customAssetsNetwork &&
      tokenContractAddress &&
      (customAssetsNetwork.coin === BraveWallet.CoinType.ETH
        ? !!customTokenID
        : true)
      ? {
          contractAddress: tokenContractAddress,
          tokenId: customTokenID,
          network: {
            chainId: customAssetsNetwork.chainId,
            coin: customAssetsNetwork.coin
          }
        }
      : skipToken
  )

  const networkList = useGetCustomAssetSupportedNetworks()

  const name = matchedTokenInfo?.name || selectedAsset?.name || ''
  const symbol = matchedTokenInfo?.symbol || selectedAsset?.symbol || ''
  const logo = matchedTokenInfo?.logo || selectedAsset?.logo || ''

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
        logo,
        isCompressed: false, // isCompressed will be set by the backend
        tokenId: customTokenID ? new Amount(customTokenID).toHex() : '',
        isErc20: false,
        isErc721:
          !!customTokenID &&
          customAssetsNetwork.coin !== BraveWallet.CoinType.SOL,
        isErc1155: false,
        splTokenProgram: BraveWallet.SPLTokenProgram.kUnknown,
        isNft: true,
        isSpam: false,
        visible: true
      }
    }, [
      customAssetsNetwork,
      tokenContractAddress,
      name,
      symbol,
      logo,
      customTokenID
    ])

  const {
    data: nftMetadata,
    isFetching: isFetchingNftMetadata,
    isError: hasNftMetadataError
  } = useGetNftMetadataQuery(
    tokenInfo &&
      (tokenInfo.coin === BraveWallet.CoinType.SOL || tokenInfo.tokenId)
      ? tokenInfo
      : skipToken
  )

  const { data: userOwnsNft, isFetching: isFetchingBalanceCheck } =
    useGetIsTokenOwnedByUserQuery(tokenInfo ?? skipToken)

  // computed
  const customAssetsNetworkError = !customAssetsNetwork?.chainId
  const tokenNameError = !tokenInfo?.name
  const tokenSymbolError = !tokenInfo?.symbol
  const tokenIdError =
    tokenInfo?.coin === BraveWallet.CoinType.ETH && !customTokenID
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
      tokenSymbolError && getLocale('braveWalletTokenSymbolIsRequiredError'),
      tokenIdError && getLocale('braveWalletWatchListTokenIdError')
    ]
  }, [
    tokenContractAddressError,
    customAssetsNetworkError,
    tokenNameError,
    tokenSymbolError,
    tokenIdError
  ])

  const metadataAsset: BraveWallet.BlockchainToken | undefined =
    React.useMemo(() => {
      return tokenInfo &&
        !isFetchingNftMetadata &&
        (tokenInfo?.coin === BraveWallet.CoinType.SOL || customTokenID)
        ? {
            ...tokenInfo,
            logo: nftMetadata?.imageURL || tokenInfo.logo || '',
            name:
              nftMetadata?.contractInformation?.name || tokenInfo.name || '',
            // TODO: response from core currently doesn't have symbol
            symbol: tokenInfo.symbol || ''
          }
        : undefined
    }, [customTokenID, isFetchingNftMetadata, nftMetadata, tokenInfo])

  // methods
  const resetBaseInputFields = React.useCallback(() => {
    setCustomTokenID(undefined)
  }, [])

  const resetInputFields = React.useCallback(() => {
    resetBaseInputFields()
    onChangeContractAddress('')
  }, [resetBaseInputFields, onChangeContractAddress])

  const handleTokenIDChanged = React.useCallback((detail: InputEventDetail) => {
    setHasError(false)
    setCustomTokenID(detail.value)
  }, [])

  const handleTokenAddressChanged = React.useCallback(
    (event: InputEventDetail) => {
      setHasError(false)
      if (event.value === '') {
        resetInputFields()
        return
      }

      onChangeContractAddress(event.value)
    },
    [onChangeContractAddress, resetInputFields]
  )

  const addOrUpdateToken = React.useCallback(async () => {
    const updatedToken = metadataAsset ?? tokenInfo
    if (!updatedToken) {
      return
    }

    if (tokenAlreadyExists && selectedAsset) {
      await updateUserToken({
        existingToken: selectedAsset,
        updatedToken
      }).unwrap()
      onHideForm()
      return
    }

    try {
      await addUserToken(updatedToken).unwrap()
      onHideForm()
    } catch (error) {
      setHasError(true)
    }
  }, [
    metadataAsset,
    tokenInfo,
    tokenAlreadyExists,
    selectedAsset,
    updateUserToken,
    onHideForm,
    addUserToken
  ])

  const onHideNetworkDropDown = React.useCallback(() => {
    if (showNetworkDropDown) {
      setShowNetworkDropDown(false)
    }
  }, [showNetworkDropDown])

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

        <FullWidthFormColumn>
          <Input
            value={tokenContractAddress}
            onChange={handleTokenAddressChanged}
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
              <InfoIconTooltip
                placement='bottom'
                text={getLocale('braveWalletHowToFindNftContractAddress')}
              />
            </Row>
          </Input>
        </FullWidthFormColumn>

        <FullWidthFormColumn>
          {customAssetsNetwork?.coin !== BraveWallet.CoinType.SOL && (
            <Input
              value={customTokenID ? new Amount(customTokenID).format() : ''}
              onInput={handleTokenIDChanged}
              type='number'
              placeholder={'1234'}
            >
              <Row
                gap='4px'
                justifyContent='flex-start'
              >
                <InputLabel>
                  {getLocale('braveWalletNFTDetailTokenID')}
                </InputLabel>
                <InfoIconTooltip
                  placement='bottom'
                  text={getLocale('braveWalletHowToFindNftTokenId')}
                />
              </Row>
            </Input>
          )}
        </FullWidthFormColumn>
        <>
          {tokenInfo?.coin === BraveWallet.CoinType.ETH &&
            !tokenInfo?.tokenId && (
              <ErrorText>
                {getLocale('braveWalletWatchListTokenIdError')}
              </ErrorText>
            )}
        </>

        {hasError ? (
          <ErrorText>{getLocale('braveWalletWatchListError')}</ErrorText>
        ) : (
          <Column
            fullWidth
            alignItems='center'
            justifyContent='center'
          >
            {isFetchingNftMetadata ? (
              <Column
                fullWidth
                gap={'4px'}
              >
                <PreviewImageContainer>
                  <Skeleton
                    width={96}
                    height={96}
                    enableAnimation
                  />
                </PreviewImageContainer>

                <Skeleton
                  width={96}
                  height={22}
                  enableAnimation
                />
                <Skeleton
                  width={96}
                  height={22}
                  enableAnimation
                />
              </Column>
            ) : (
              <>
                {hasNftMetadataError ? (
                  <Column>
                    <ErrorText>
                      {getLocale('braveWalletFetchNftMetadataError')}
                    </ErrorText>
                  </Column>
                ) : (
                  <>
                    {metadataAsset && (
                      <Column
                        fullWidth
                        gap={'16px'}
                      >
                        <Column fullWidth>
                          <PreviewImageContainer>
                            <NftIconWithPlaceholder
                              key={getAssetIdKey(metadataAsset)}
                              asset={metadataAsset}
                              responsive
                            />
                          </PreviewImageContainer>

                          <TokenNamePreviewText>
                            {metadataAsset.name}
                          </TokenNamePreviewText>
                          <TokenTickerPreviewText>
                            {metadataAsset.symbol}
                          </TokenTickerPreviewText>
                        </Column>

                        {!userOwnsNft && (
                          <Alert
                            mode='simple'
                            type='info'
                          >
                            {getLocale('braveWalletUnownedNftAlert')}
                          </Alert>
                        )}
                      </Column>
                    )}
                  </>
                )}
              </>
            )}
          </Column>
        )}

        <ButtonRowSpacer />
      </FormWrapper>

      <ButtonRow gap='16px'>
        <LeoSquaredButton
          onClick={onClickCancel}
          kind='plain-faint'
        >
          {getLocale('braveWalletButtonCancel')}
        </LeoSquaredButton>

        <Tooltip
          text={<FormErrorsList errors={formErrors} />}
          isVisible={buttonDisabled}
          verticalPosition='above'
        >
          <Row>
            <LeoSquaredButton
              onClick={addOrUpdateToken}
              isDisabled={buttonDisabled}
              isLoading={
                isTokenInfoLoading ||
                isFetchingBalanceCheck ||
                isFetchingNftMetadata
              }
            >
              {!userOwnsNft
                ? getLocale('braveWalletWatchThisNft')
                : selectedAsset
                ? getLocale('braveWalletButtonSaveChanges')
                : getLocale('braveWalletWatchListAdd')}
            </LeoSquaredButton>
          </Row>
        </Tooltip>
      </ButtonRow>
    </>
  )
}
