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
import {
  getAssetIdKey,
  type GetBlockchainTokenIdArg
} from '../../../utils/asset-utils'

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
  InputLoadingIndicator,
  PreviewImageContainer,
  TokenNamePreviewText,
  TokenTickerPreviewText,
  DescriptionRow
} from './add-custom-token-form-styles'
import { Column, LeoSquaredButton, Row } from '../style'
import { Skeleton } from '../loading-skeleton/styles'
import Icon from '@brave/leo/react/icon'
import Tooltip from '../tooltip'

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
  const [customTokenName, setCustomTokenName] = React.useState(
    selectedAsset?.name || ''
  )
  const [customTokenID, setCustomTokenID] = React.useState<string | undefined>(
    selectedAsset?.tokenId
  )
  const [customTokenSymbol, setCustomTokenSymbol] = React.useState(
    selectedAsset?.symbol || ''
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
    isLoading: isTokenInfoLoading,
    isError: hasGetTokenInfoError
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

  const metadataLookupArg: GetBlockchainTokenIdArg | undefined =
    React.useMemo(() => {
      if (!customAssetsNetwork || !tokenContractAddress) {
        return undefined
      }

      return {
        chainId: customAssetsNetwork.chainId,
        coin: customAssetsNetwork.coin,
        contractAddress: tokenContractAddress,
        tokenId:
          customAssetsNetwork.coin !== BraveWallet.CoinType.SOL && customTokenID
            ? new Amount(customTokenID).toHex()
            : '',
        isErc721:
          !!customTokenID &&
          customAssetsNetwork.coin !== BraveWallet.CoinType.SOL,
        isNft: true,
        isShielded: false
      }
    }, [customAssetsNetwork, tokenContractAddress, customTokenID])

  // TODO: need symbol in response in order to simplify adding SOL NFTs
  const {
    data: nftMetadata,
    isFetching: isFetchingNftMetadata,
    isError: hasNftMetadataError
  } = useGetNftMetadataQuery(
    metadataLookupArg &&
      (metadataLookupArg.coin === BraveWallet.CoinType.SOL ||
        metadataLookupArg.tokenId)
      ? metadataLookupArg
      : skipToken
  )

  const { data: userOwnsNft, isFetching: isFetchingBalanceCheck } =
    useGetIsTokenOwnedByUserQuery(metadataLookupArg ?? skipToken)

  // computed
  const customAssetsNetworkError = !customAssetsNetwork?.chainId
  const tokenNameError = !customTokenName
  const tokenSymbolError = !customTokenSymbol
  const tokenIdError =
    selectedAssetNetwork?.coin === BraveWallet.CoinType.ETH && !customTokenID
  const tokenContractAddressError =
    tokenContractAddress === '' ||
    (customAssetsNetwork?.coin !== BraveWallet.CoinType.SOL &&
      !tokenContractAddress.toLowerCase().startsWith('0x'))

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

  const metadataAssetInfo:
    | (GetBlockchainTokenIdArg &
        Pick<BraveWallet.BlockchainToken, 'logo' | 'name' | 'symbol'>)
    | undefined = React.useMemo(() => {
    return metadataLookupArg &&
      !isFetchingNftMetadata &&
      (metadataLookupArg?.coin === BraveWallet.CoinType.SOL || customTokenID)
      ? {
          ...metadataLookupArg,
          logo:
            nftMetadata?.imageURL ||
            matchedTokenInfo?.logo ||
            selectedAsset?.logo ||
            '',
          name: customTokenName,
          // TODO: response from core currently doesn't have symbol
          symbol: customTokenSymbol
        }
      : undefined
  }, [
    customTokenID,
    customTokenName,
    customTokenSymbol,
    isFetchingNftMetadata,
    matchedTokenInfo?.logo,
    nftMetadata?.imageURL,
    selectedAsset?.logo,
    metadataLookupArg
  ])

  // methods
  const resetBaseInputFields = React.useCallback(() => {
    setCustomTokenID(undefined)
  }, [])

  const resetInputFields = React.useCallback(() => {
    resetBaseInputFields()
    onChangeContractAddress('')
  }, [resetBaseInputFields, onChangeContractAddress])

  const handleTokenNameChanged = React.useCallback(
    (detail: InputEventDetail) => {
      setHasError(false)
      setCustomTokenName(detail.value)
    },
    []
  )

  const handleTokenIDChanged = React.useCallback((detail: InputEventDetail) => {
    setHasError(false)
    setCustomTokenID(detail.value)
  }, [])

  const handleTokenSymbolChanged = React.useCallback(
    (detail: InputEventDetail) => {
      setHasError(false)
      setCustomTokenSymbol(detail.value)
    },
    []
  )

  const handleTokenAddressChanged = React.useCallback(
    (event: InputEventDetail) => {
      setHasError(false)
      onChangeContractAddress(event.value)
    },
    [onChangeContractAddress]
  )

  const addOrUpdateToken = React.useCallback(async () => {
    if (!customAssetsNetwork) {
      return
    }

    const updatedToken: BraveWallet.BlockchainToken = {
      name: customTokenName,
      symbol: customTokenSymbol,
      decimals: 0,
      coingeckoId: '',
      logo: matchedTokenInfo?.logo || selectedAsset?.logo || '',
      isCompressed: false, // isCompressed will be set by the backend
      isErc20: false,
      isErc1155: false,
      splTokenProgram: BraveWallet.SPLTokenProgram.kUnknown,
      isSpam: false,
      visible: true,
      chainId: customAssetsNetwork.chainId,
      coin: customAssetsNetwork.coin,
      contractAddress: tokenContractAddress,
      tokenId:
        customAssetsNetwork.coin !== BraveWallet.CoinType.SOL && customTokenID
          ? new Amount(customTokenID).toHex()
          : '',
      isNft: true,
      isErc721:
        matchedTokenInfo?.isErc721 ??
        (!!customTokenID &&
          customAssetsNetwork.coin !== BraveWallet.CoinType.SOL),
      isShielded: false
    }

    if (selectedAsset) {
      // remove existing token and add new one
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
    customAssetsNetwork,
    customTokenName,
    customTokenSymbol,
    matchedTokenInfo,
    selectedAsset,
    tokenContractAddress,
    customTokenID,
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

  // effects
  React.useEffect(() => {
    // wait for new data before syncing
    if (isFetchingNftMetadata || isTokenInfoLoading) {
      return
    }

    // sync form fields with found token info
    if (!hasGetTokenInfoError && matchedTokenInfo?.name) {
      setCustomTokenName(matchedTokenInfo.name)
    }

    if (!hasNftMetadataError && nftMetadata?.contractInformation.name) {
      setCustomTokenName(nftMetadata.contractInformation.name)
    }

    if (!hasGetTokenInfoError && matchedTokenInfo?.symbol) {
      setCustomTokenSymbol(matchedTokenInfo.symbol)
    }
  }, [
    hasGetTokenInfoError,
    hasNftMetadataError,
    isFetchingNftMetadata,
    isTokenInfoLoading,
    matchedTokenInfo?.name,
    matchedTokenInfo?.symbol,
    nftMetadata?.contractInformation.name
  ])

  // render
  return (
    <>
      <FormWrapper onClick={onHideNetworkDropDown}>
        {!selectedAsset && (
          <FullWidthFormColumn>
            <DescriptionRow>
              {getLocale('braveWalletAddNftModalDescription')}
            </DescriptionRow>
          </FullWidthFormColumn>
        )}

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
            onInput={handleTokenAddressChanged}
            placeholder={getLocale('braveWalletExempliGratia').replace(
              '$1',
              '0xbd3531da5cf5857e7cfaa92426877b022e612cf8'
            )}
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
                text={getLocale('braveWalletWhatIsAnNftContractAddress')}
              />
            </Row>
          </Input>
        </FullWidthFormColumn>

        {customAssetsNetwork &&
          customAssetsNetwork?.coin !== BraveWallet.CoinType.SOL && (
            <FullWidthFormColumn>
              <Input
                value={
                  customTokenID
                    ? new Amount(customTokenID).format(undefined, false)
                    : ''
                }
                onInput={handleTokenIDChanged}
                type='number'
                placeholder={getLocale('braveWalletExempliGratia').replace(
                  '$1',
                  '1234'
                )}
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
                    text={getLocale('braveWalletWhatIsAnNftTokenId')}
                  />
                </Row>
              </Input>
            </FullWidthFormColumn>
          )}

        <FullWidthFormColumn>
          <Input
            value={customTokenName}
            onInput={handleTokenNameChanged}
            type='text'
            placeholder={getLocale('braveWalletExempliGratia').replace(
              '$1',
              'Pudgy Penguin #1234'
            )}
          >
            <Row
              gap='4px'
              justifyContent='flex-start'
            >
              <InputLabel>
                {getLocale('braveWalletWatchListTokenName')}
              </InputLabel>
              <InfoIconTooltip
                placement='bottom'
                text={getLocale('braveWalletNftNameFieldExplanation')}
              />
            </Row>

            <div slot='left-icon'>
              {(isFetchingNftMetadata || isTokenInfoLoading) && (
                <InputLoadingIndicator />
              )}
            </div>
          </Input>
        </FullWidthFormColumn>

        <FullWidthFormColumn>
          <Input
            value={customTokenSymbol}
            onInput={handleTokenSymbolChanged}
            disabled={
              // prevent edits when data has been found on-chain
              !hasGetTokenInfoError && !!matchedTokenInfo?.symbol
            }
            type='text'
            placeholder={getLocale('braveWalletExempliGratia').replace(
              '$1',
              'PPG'
            )}
          >
            <Row
              gap='4px'
              justifyContent='flex-start'
            >
              <InputLabel>
                {getLocale('braveWalletWatchListTokenSymbol')}
              </InputLabel>
              <InfoIconTooltip
                placement='bottom'
                text={getLocale('braveWalletNftSymbolFieldExplanation')}
              />
            </Row>
            <div slot='left-icon'>
              {(isFetchingNftMetadata || isTokenInfoLoading) && (
                <InputLoadingIndicator />
              )}
            </div>
          </Input>
        </FullWidthFormColumn>

        {metadataLookupArg?.coin === BraveWallet.CoinType.ETH &&
          !metadataLookupArg?.tokenId && (
            <ErrorText>
              {getLocale('braveWalletWatchListTokenIdError')}
            </ErrorText>
          )}

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
                    {metadataAssetInfo && (
                      <Column
                        fullWidth
                        gap={'16px'}
                      >
                        <Row
                          alignItems='center'
                          gap='16px'
                        >
                          {selectedAsset && (
                            <>
                              <Column>
                                <PreviewImageContainer>
                                  <NftIconWithPlaceholder
                                    key={getAssetIdKey(selectedAsset)}
                                    asset={selectedAsset}
                                    responsive
                                  />
                                </PreviewImageContainer>

                                <TokenNamePreviewText>
                                  {selectedAsset.name}
                                </TokenNamePreviewText>
                                <TokenTickerPreviewText>
                                  {selectedAsset.symbol}
                                </TokenTickerPreviewText>
                              </Column>

                              <Column>
                                <Icon name={'carat-right'} />
                              </Column>
                            </>
                          )}

                          <Column>
                            <PreviewImageContainer>
                              <NftIconWithPlaceholder
                                key={getAssetIdKey(metadataAssetInfo)}
                                asset={metadataAssetInfo}
                                responsive
                              />
                            </PreviewImageContainer>

                            <TokenNamePreviewText>
                              {customTokenName}
                            </TokenNamePreviewText>
                            <TokenTickerPreviewText>
                              {metadataAssetInfo.symbol}
                            </TokenTickerPreviewText>
                          </Column>
                        </Row>

                        {!userOwnsNft && (
                          <Alert
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
          maxWidth={120}
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
