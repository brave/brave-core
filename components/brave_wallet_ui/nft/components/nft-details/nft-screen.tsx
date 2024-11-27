// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'
import { useHistory } from 'react-router'
import { skipToken } from '@reduxjs/toolkit/dist/query'
import Alert from '@brave/leo/react/alert'
import Tooltip from '@brave/leo/react/tooltip'

// types
import { BraveWallet, AccountPageTabs } from '../../../constants/types'

// hooks
import useExplorer from '../../../common/hooks/explorer'
import useBalancesFetcher from '../../../common/hooks/use-balances-fetcher'

// utils
import Amount from '../../../utils/amount'
import {
  getNFTTokenStandard,
  isComponentInStorybook,
  stripERC20TokenImageURL
} from '../../../utils/string-utils'
import { reduceAddress } from '../../../utils/reduce-address'
import { getLocale } from '../../../../common/locale'
import { makeAccountRoute } from '../../../utils/routes-utils'
import { isTokenWatchOnly } from '../../../utils/asset-utils'

// queries & mutations
import {
  useGetNftMetadataQuery,
  useGetNftOwnerQuery,
  useUpdateUserTokenMutation
} from '../../../common/slices/api.slice'
import { useAccountsQuery } from '../../../common/slices/api.slice.extra'

// components
import { Skeleton } from '../../../components/shared/loading-skeleton/styles'
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
  HighlightedText,
  AccountAddress,
  AccountName,
  CopyIcon,
  ViewAccount,
  ErrorMessage,
  NftMultimediaWrapper,
  NftMultimedia,
  IconWrapper,
  NetworkIconWrapper
} from './nft-screen.styles'
import { Row, VerticalSpace } from '../../../components/shared/style'

interface Props {
  selectedAsset: BraveWallet.BlockchainToken
  tokenNetwork?: BraveWallet.NetworkInfo
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

export const NftScreen = ({ selectedAsset, tokenNetwork }: Props) => {
  // state
  const nftDetailsRef = React.useRef<HTMLIFrameElement>(null)

  // queries
  const {
    data: nftMetadata,
    isLoading: isFetchingNFTMetadata,
    error: nftMetadataError
  } = useGetNftMetadataQuery(selectedAsset, {
    skip: !selectedAsset
  })

  const { data: ownerAddress } = useGetNftOwnerQuery(
    tokenNetwork
      ? {
          contract: selectedAsset.contractAddress,
          tokenId: selectedAsset.tokenId,
          chainId: tokenNetwork.chainId
        }
      : skipToken
  )

  const { accounts } = useAccountsQuery()

  const { data: tokenBalancesRegistry } = useBalancesFetcher(
    tokenNetwork
      ? {
          accounts,
          networks: [tokenNetwork],
          isSpamRegistry: false
        }
      : skipToken
  )

  const { data: spamTokenBalancesRegistry } = useBalancesFetcher(
    tokenNetwork
      ? {
          accounts,
          networks: [tokenNetwork],
          isSpamRegistry: true
        }
      : skipToken
  )

  // mutations
  const [updateUserToken] = useUpdateUserTokenMutation()

  // hooks
  const history = useHistory()
  const onClickViewOnBlockExplorer = useExplorer(
    tokenNetwork || new BraveWallet.NetworkInfo()
  )

  // memos
  const ownerAccount = React.useMemo(() => {
    if (!ownerAddress) return

    return accounts.find(
      (account) => account.address.toLowerCase() === ownerAddress.toLowerCase()
    )
  }, [accounts, ownerAddress])

  const tokenIdAsNumberString = React.useMemo(
    () =>
      selectedAsset.tokenId ? new Amount(selectedAsset.tokenId).format() : '',
    [selectedAsset.tokenId]
  )
  const reducedTokenId = tokenIdAsNumberString
    ? reduceAddress(tokenIdAsNumberString, '...')
    : ''

  const nftIFrameUrl = React.useMemo(() => {
    const params = new URLSearchParams({
      displayMode: 'details',
      nftMetadata: nftMetadata ? JSON.stringify(nftMetadata) : '',
      error: nftMetadataError as string
    })
    return `chrome-untrusted://nft-display?${params}`
  }, [nftMetadata, nftMetadataError])

  // methods
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

  // render
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
      {isTokenWatchOnly(
        selectedAsset,
        accounts,
        tokenBalancesRegistry,
        spamTokenBalancesRegistry
      ) && (
        <>
          <Alert
            type='info'
          >
            {getLocale('braveWalletUnownedNftAlert')}
          </Alert>
          <VerticalSpace space='24px' />
        </>
      )}
      <TopWrapper>
        <NftMultimediaWrapper>
          {isFetchingNFTMetadata ? (
            <Skeleton {...createSkeletonProps(360, 360)} />
          ) : isStorybook ? (
            <NftMultimedia
              as='img'
              visible={!isFetchingNFTMetadata}
              src={nftMetadata?.imageURL}
            />
          ) : (
            <NftMultimedia
              visible={!isFetchingNFTMetadata}
              ref={nftDetailsRef}
              sandbox='allow-scripts allow-popups allow-same-origin'
              src={nftIFrameUrl}
              allowFullScreen
            />
          )}
          {tokenNetwork && !isFetchingNFTMetadata && (
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
        <NftName>{selectedAsset.name}</NftName>
      </TopWrapper>
      <SectionTitle>{getLocale('braveWalletNFTDetailsOverview')}</SectionTitle>
      {ownerAddress && (
        <SectionWrapper>
          <InfoBox>
            <InfoTitle>{getLocale('braveWalletNFTDetailsOwnedBy')}</InfoTitle>
            <Row
              justifyContent='flex-start'
              gap='4px'
            >
              {ownerAccount && <AccountName>{ownerAccount.name}</AccountName>}
              <Tooltip text={ownerAddress}>
                <AccountAddress>
                  {reduceAddress(ownerAddress, '...')}
                </AccountAddress>
              </Tooltip>
              <CopyTooltip
                text={ownerAddress}
                verticalPosition='above'
              >
                <CopyIcon name='copy' />
              </CopyTooltip>
            </Row>
            {ownerAccount ? (
              <ViewAccount onClick={() => onClickViewAccount(ownerAccount)}>
                {getLocale('braveWalletNFTDetailsViewAccount')}
              </ViewAccount>
            ) : (
              <ViewAccount
                onClick={onClickViewOnBlockExplorer('address', ownerAddress)}
              >
                {getLocale('braveWalletPortfolioViewOnExplorerMenuLabel')}
              </ViewAccount>
            )}
          </InfoBox>
        </SectionWrapper>
      )}
      <SectionWrapper>
        {selectedAsset.tokenId !== '' && (
          <InfoBox>
            <InfoTitle>{getLocale('braveWalletNFTDetailTokenID')}</InfoTitle>
            <Row
              justifyContent='flex-start'
              gap='4px'
            >
              <Tooltip text={tokenIdAsNumberString}>
                <InfoText>{reducedTokenId}</InfoText>
              </Tooltip>
              <CopyTooltip
                text={tokenIdAsNumberString}
                verticalPosition='above'
              >
                <CopyIcon name='copy' />
              </CopyTooltip>
            </Row>
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
