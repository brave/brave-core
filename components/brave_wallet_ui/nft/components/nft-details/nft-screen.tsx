// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'
import { useHistory } from 'react-router'
import { skipToken } from '@reduxjs/toolkit/query'

// types
import { BraveWallet, AccountPageTabs } from '../../../constants/types'

// hooks
import useExplorer from '../../../common/hooks/explorer'

// utils
import Amount from '../../../utils/amount'
import {
  getCid,
  getNFTTokenStandard,
  isComponentInStorybook,
  stripERC20TokenImageURL
} from '../../../utils/string-utils'
import { reduceAddress } from '../../../utils/reduce-address'
import {
  CommandMessage,
  NftUiCommand,
  UpdateLoadingMessage,
  UpdateNFtMetadataErrorMessage,
  UpdateNFtMetadataMessage,
  UpdateNftImageLoadingMessage,
  braveNftDisplayOrigin,
  braveWalletPanelOrigin,
  sendMessageToNftUiFrame
} from '../../nft-ui-messages'
import { getLocale } from '../../../../common/locale'
import { makeAccountRoute } from '../../../utils/routes-utils'

// queries & mutations
import {
  useGetAutopinEnabledQuery,
  useGetIPFSUrlFromGatewayLikeUrlQuery,
  useGetNftMetadataQuery,
  useGetNftPinningStatusQuery,
  useUpdateUserTokenMutation,
  useGetIsImagePinnableQuery
} from '../../../common/slices/api.slice'

// components
import { Skeleton } from '../../../components/shared/loading-skeleton/styles'
import { ErrorTooltip } from '../../../components/desktop/nft-pinning-status/components/error-tooltip/error-tooltip'
import { CopyTooltip } from '../../../components/shared/copy-tooltip/copy-tooltip'
import CreateNetworkIcon from '../../../components/shared/create-network-icon'

// styles
import {
  NftName,
  StyledWrapper,
  TopWrapper,
  SectionTitle,
  SectionWrapper,
  InfoBox,
  InfoTitle,
  InfoText,
  ButtonIcon,
  HighlightedButton,
  Description,
  Divider,
  Properties,
  Property,
  Trait,
  TraitRarity,
  TraitType,
  TraitValue,
  PinningStatus,
  HighlightedText,
  AccountAddress,
  AccountName,
  CopyIcon,
  ViewAccount,
  InfoIcon,
  ErrorWrapper,
  ErrorMessage,
  ImageLink,
  NftMultimediaWrapper,
  NftMultimedia,
  IconWrapper,
  NetworkIconWrapper
} from './nft-screen.styles'
import { Row } from '../../../components/shared/style'

interface Props {
  selectedAsset: BraveWallet.BlockchainToken
  tokenNetwork?: BraveWallet.NetworkInfo
  ownerAccount?: BraveWallet.AccountInfo
}

const createSkeletonProps = (
  width?: string | number,
  height?: string | number
) => {
  return {
    width: width || '100%',
    height: height || '100%',
    enableAnimation: true
  }
}

const isStorybook = isComponentInStorybook()

export const NftScreen = (props: Props) => {
  const { selectedAsset, tokenNetwork, ownerAccount } = props

  // state
  const [showTooltip, setShowTooltip] = React.useState<boolean>(false)
  const nftDetailsRef = React.useRef<HTMLIFrameElement>(null)
  const [nftIframeLoaded, setNftIframeLoaded] = React.useState(false)
  const [nftImageLoading, setNftImageLoading] = React.useState<boolean>(false)

  // queries
  const {
    data: nftMetadata,
    isLoading: isFetchingNFTMetadata,
    error: nftMetadataError
  } = useGetNftMetadataQuery(selectedAsset, {
    skip: !selectedAsset
  })
  const { data: isAutoPinEnabled } = useGetAutopinEnabledQuery()
  const { data: isNftPinnable } = useGetIsImagePinnableQuery(
    nftMetadata?.imageURL || skipToken
  )
  const { data: currentNftPinningStatus } = useGetNftPinningStatusQuery(
    selectedAsset,
    {
      skip: !selectedAsset || !isAutoPinEnabled || !isNftPinnable
    }
  )
  const { data: ipfsImageUrl } = useGetIPFSUrlFromGatewayLikeUrlQuery(
    nftMetadata?.imageURL || skipToken
  )

  // mutations
  const [updateUserToken] = useUpdateUserTokenMutation()

  // hooks
  const history = useHistory()
  const onClickViewOnBlockExplorer = useExplorer(
    tokenNetwork || new BraveWallet.NetworkInfo()
  )

  // memos
  const isNftPinned = React.useMemo(() => {
    return (
      currentNftPinningStatus?.code ===
      BraveWallet.TokenPinStatusCode.STATUS_PINNED
    )
  }, [currentNftPinningStatus?.code])

  const pinningStatusText = React.useMemo(() => {
    switch (currentNftPinningStatus?.code) {
      case BraveWallet.TokenPinStatusCode.STATUS_PINNING_IN_PROGRESS:
        return getLocale('braveWalletNFTDetailsPinningInProgress')

      case BraveWallet.TokenPinStatusCode.STATUS_PINNING_PENDING:
        return getLocale('braveWalletNFTDetailsPinningInProgress')

      case BraveWallet.TokenPinStatusCode.STATUS_NOT_PINNED:
        return getLocale('braveWalletNFTDetailsNotAvailable')

      case BraveWallet.TokenPinStatusCode.STATUS_PINNED:
        return getLocale('braveWalletNFTDetailsPinningSuccessful')

      case BraveWallet.TokenPinStatusCode.STATUS_PINNING_FAILED:
        return getLocale('braveWalletNFTDetailsPinningFailed')

      default:
        return getLocale('braveWalletNFTDetailsNotAvailable')
    }
  }, [currentNftPinningStatus?.code])

  const tokenId = React.useMemo(
    () =>
      selectedAsset.tokenId ? new Amount(selectedAsset.tokenId).toNumber() : '',
    [selectedAsset.tokenId]
  )

  // methods
  const onNftDetailsLoad = React.useCallback(() => {
    setNftIframeLoaded(true)
  }, [])

  const onMessageEventListener = React.useCallback(
    (event: MessageEvent<CommandMessage>) => {
      // validate message origin
      if (
        event.origin === braveNftDisplayOrigin ||
        event.origin === braveWalletPanelOrigin
      ) {
        const message = event.data
        if (message.command === NftUiCommand.UpdateNftImageLoading) {
          const { payload } = message as UpdateNftImageLoadingMessage
          setNftImageLoading(payload)
        }
      }
    },
    []
  )

  const onClickContractAddress = React.useCallback(() => {
    onClickViewOnBlockExplorer(
      selectedAsset.coin === BraveWallet.CoinType.ETH ? 'nft' : 'token',
      selectedAsset.contractAddress,
      selectedAsset.tokenId
    )()
  }, [
    onClickViewOnBlockExplorer,
    selectedAsset.coin,
    selectedAsset.contractAddress,
    selectedAsset.tokenId
  ])

  const onClickViewAccount = React.useCallback(
    (account: BraveWallet.AccountInfo) => {
      history.push(makeAccountRoute(account, AccountPageTabs.AccountAssetsSub))
    },
    [history]
  )

  // effects
  React.useEffect(() => {
    if (!nftIframeLoaded) return

    if (nftDetailsRef?.current) {
      const command: UpdateLoadingMessage = {
        command: NftUiCommand.UpdateLoading,
        payload: isFetchingNFTMetadata
      }
      sendMessageToNftUiFrame(nftDetailsRef.current.contentWindow, command)
    }
  }, [nftIframeLoaded, nftDetailsRef, isFetchingNFTMetadata])

  React.useEffect(() => {
    if (!nftIframeLoaded) return

    if (nftMetadata && nftDetailsRef?.current) {
      const command: UpdateNFtMetadataMessage = {
        command: NftUiCommand.UpdateNFTMetadata,
        payload: {
          displayMode: 'details',
          nftMetadata
        }
      }
      sendMessageToNftUiFrame(nftDetailsRef.current.contentWindow, command)
    }

    if (nftMetadataError && nftDetailsRef?.current) {
      const command: UpdateNFtMetadataErrorMessage = {
        command: NftUiCommand.UpdateNFTMetadataError,
        payload: {
          displayMode: 'details',
          error: nftMetadataError as string
        }
      }
      sendMessageToNftUiFrame(nftDetailsRef.current.contentWindow, command)
    }
  }, [
    nftDetailsRef,
    nftIframeLoaded,
    nftMetadata,
    nftMetadataError,
    selectedAsset,
    tokenNetwork
  ])

  React.useEffect(() => {
    // update the asset logo if it doesn't currently have one + one was found
    // in the metadata
    if (
      selectedAsset &&
      nftMetadata?.imageURL &&
      stripERC20TokenImageURL(selectedAsset.logo) === ''
    ) {
      updateUserToken({
        existingToken: selectedAsset,
        updatedToken: { ...selectedAsset, logo: nftMetadata?.imageURL || '' }
      })
    }
  }, [selectedAsset, nftMetadata?.imageURL, updateUserToken])

  // Receive postMessage from chrome-untrusted://nft-display
  React.useEffect(() => {
    window.addEventListener('message', onMessageEventListener)
    return () => window.removeEventListener('message', onMessageEventListener)
  }, [onMessageEventListener])

  if (nftMetadataError)
    return (
      <StyledWrapper>
        <Row
          alignItems='center'
          justifyContent='center'
        >
          <ErrorMessage>
            {getLocale('braveWalletNftFetchingError')}
          </ErrorMessage>
        </Row>
      </StyledWrapper>
    )

  return (
    <StyledWrapper>
      <TopWrapper>
        <NftMultimediaWrapper>
          {isFetchingNFTMetadata ? (
            <Skeleton {...createSkeletonProps(360, 360)} />
          ) : isStorybook ? (
            <NftMultimedia
              as='img'
              visible={!nftImageLoading && !isFetchingNFTMetadata}
              src={nftMetadata?.imageURL}
            />
          ) : (
            <NftMultimedia
              onLoad={onNftDetailsLoad}
              visible={!nftImageLoading && !isFetchingNFTMetadata}
              ref={nftDetailsRef}
              sandbox='allow-scripts allow-popups allow-same-origin'
              src='chrome-untrusted://nft-display'
              allowFullScreen
            />
          )}
          {tokenNetwork && !isFetchingNFTMetadata && !nftImageLoading && (
            <IconWrapper>
              <NetworkIconWrapper>
                <CreateNetworkIcon
                  size='big'
                  network={tokenNetwork}
                  marginRight={0}
                />
              </NetworkIconWrapper>
            </IconWrapper>
          )}
        </NftMultimediaWrapper>
        <NftName>
          {selectedAsset.name} {tokenId ? `#${tokenId}` : ''}
        </NftName>
      </TopWrapper>
      <SectionTitle>{getLocale('braveWalletNFTDetailsOverview')}</SectionTitle>
      {ownerAccount && (
        <SectionWrapper>
          <InfoBox>
            <InfoTitle>{getLocale('braveWalletNFTDetailsOwnedBy')}</InfoTitle>
            <Row
              justifyContent='flex-start'
              gap='4px'
            >
              <AccountName>{ownerAccount.name}</AccountName>
              <AccountAddress>
                {reduceAddress(ownerAccount.address, '...')}
              </AccountAddress>
              <CopyTooltip
                text={selectedAsset.contractAddress}
                verticalPosition='above'
              >
                <CopyIcon name='copy' />
              </CopyTooltip>
            </Row>
            <ViewAccount onClick={() => onClickViewAccount(ownerAccount)}>
              {getLocale('braveWalletNFTDetailsViewAccount')}
            </ViewAccount>
          </InfoBox>
        </SectionWrapper>
      )}
      <SectionWrapper>
        {selectedAsset.tokenId !== '' && (
          <InfoBox>
            <InfoTitle>{getLocale('braveWalletNFTDetailTokenID')}</InfoTitle>
            <InfoText>{tokenId || ''}</InfoText>
          </InfoBox>
        )}
        {tokenNetwork && (
          <InfoBox>
            <InfoTitle>{getLocale('braveWalletNFTDetailBlockchain')}</InfoTitle>
            <InfoText>{tokenNetwork?.chainName}</InfoText>
          </InfoBox>
        )}
      </SectionWrapper>
      <SectionWrapper>
        <InfoBox>
          <InfoTitle>
            {getLocale('braveWalletNFTDetailContractAddress')}
          </InfoTitle>
          <HighlightedButton onClick={onClickContractAddress}>
            <HighlightedText>
              {reduceAddress(selectedAsset.contractAddress, '...')}
            </HighlightedText>
            <ButtonIcon name='launch' />
          </HighlightedButton>
        </InfoBox>
        <InfoBox>
          <InfoTitle>
            {getLocale('braveWalletNFTDetailTokenStandard')}
          </InfoTitle>
          <InfoText>{getNFTTokenStandard(selectedAsset)}</InfoText>
        </InfoBox>
      </SectionWrapper>
      <SectionWrapper>
        {isFetchingNFTMetadata ? (
          <Skeleton {...createSkeletonProps('100%', 50)} />
        ) : (
          <InfoBox>
            <InfoTitle>
              {getLocale('braveWalletNFTDetailDescription')}
            </InfoTitle>
            <Description>
              {nftMetadata?.contractInformation?.description || '-'}
            </Description>
          </InfoBox>
        )}
      </SectionWrapper>
      {isAutoPinEnabled && isNftPinnable && (
        <>
          <Divider />
          <SectionTitle>
            {isFetchingNFTMetadata ? (
              <Skeleton {...createSkeletonProps(200, 20)} />
            ) : (
              <>
                {getLocale('braveWalletNFTDetailsPinningStatusLabel')}:&nbsp;
                {currentNftPinningStatus?.code ? (
                  <PinningStatus pinningStatus={currentNftPinningStatus?.code}>
                    {pinningStatusText}
                  </PinningStatus>
                ) : (
                  getLocale('braveWalletNFTDetailsNotAvailable')
                )}
                {currentNftPinningStatus?.code ===
                  BraveWallet.TokenPinStatusCode.STATUS_PINNING_FAILED && (
                  <ErrorWrapper
                    onMouseOver={() => setShowTooltip(true)}
                    onMouseLeave={() => setShowTooltip(false)}
                  >
                    <InfoIcon name='help-outline' />
                    {showTooltip && <ErrorTooltip />}
                  </ErrorWrapper>
                )}
              </>
            )}
          </SectionTitle>
          <SectionWrapper>
            {isFetchingNFTMetadata ? (
              <Skeleton {...createSkeletonProps('100%', 55)} />
            ) : (
              <InfoBox>
                <InfoTitle>{getLocale('braveWalletNFTDetailsCid')}</InfoTitle>
                <InfoText>
                  {isNftPinned
                    ? stripERC20TokenImageURL(getCid(ipfsImageUrl || '-'))
                    : getLocale('braveWalletNFTDetailsNotAvailable')}
                </InfoText>
              </InfoBox>
            )}
            {isFetchingNFTMetadata ? (
              <Skeleton />
            ) : (
              <InfoBox>
                <InfoTitle>
                  {getLocale('braveWalletNFTDetailImageAddress')}
                </InfoTitle>
                {isNftPinned && ipfsImageUrl ? (
                  <ImageLink
                    href={stripERC20TokenImageURL(ipfsImageUrl)}
                    target='_blank'
                  >
                    <HighlightedText>{nftMetadata?.imageURL}</HighlightedText>
                    <ButtonIcon name='launch' />
                  </ImageLink>
                ) : (
                  <InfoText>
                    {getLocale('braveWalletNFTDetailsNotAvailable')}
                  </InfoText>
                )}
              </InfoBox>
            )}
          </SectionWrapper>
        </>
      )}
      {nftMetadata?.attributes?.length !== 0 && <Divider />}
      {isFetchingNFTMetadata ? (
        <SectionTitle>
          <Skeleton {...createSkeletonProps(200, 20)} />
        </SectionTitle>
      ) : (
        <>
          {nftMetadata?.attributes?.length !== 0 && (
            <SectionTitle>
              {getLocale('braveWalletNFTDetailsProperties')}
            </SectionTitle>
          )}
        </>
      )}
      {isFetchingNFTMetadata ? (
        <Skeleton {...createSkeletonProps('100%', 100)} />
      ) : (
        <>
          {nftMetadata?.attributes?.length !== 0 && (
            <Properties>
              {nftMetadata?.attributes?.map((attr, idx) => (
                <Property key={idx}>
                  <Trait>
                    <TraitType>{attr.traitType}</TraitType>
                    <TraitValue>{attr.value}</TraitValue>
                  </Trait>
                  {attr.traitRarity && (
                    <TraitRarity>{attr.traitRarity}</TraitRarity>
                  )}
                </Property>
              ))}
            </Properties>
          )}
        </>
      )}
    </StyledWrapper>
  )
}
