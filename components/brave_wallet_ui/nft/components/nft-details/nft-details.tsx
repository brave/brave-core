// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Types
import {
  BraveWallet,
  NFTMetadataReturnType
} from '../../../constants/types'

// Hooks
import { useExplorer } from '../../../common/hooks'

// Utils
import Amount from '../../../utils/amount'
import { getLocale } from '$web-common/locale'

// Styled Components
import {
  DetailColumn,
  DetailSectionColumn,
  DetailSectionRow,
  DetailSectionTitle,
  DetailSectionValue,
  ErrorMessage,
  ExplorerButton,
  ExplorerIcon,
  ProjectDetailButton,
  ProjectDetailButtonRow,
  ProjectDetailButtonSeperator,
  ProjectDetailDescription,
  ProjectDetailIDRow,
  ProjectDetailName,
  ProjectDetailRow,
  ProjectFacebookIcon,
  ProjectTwitterIcon,
  ProjectWebsiteIcon,
  StyledWrapper,
  TokenName
} from './nft-details-styles'
import { isValidateUrl } from '../../../utils/string-utils'
import { NftMultimedia } from '../nft-multimedia/nft-multimedia'
import { MultimediaWrapper } from '../nft-content/nft-content-styles'

interface Props {
  isLoading?: boolean
  selectedAsset: BraveWallet.BlockchainToken
  nftMetadata?: NFTMetadataReturnType
  nftMetadataError?: string
  tokenNetwork?: BraveWallet.NetworkInfo
}

export const NftDetails = ({ selectedAsset, nftMetadata, nftMetadataError, tokenNetwork }: Props) => {
  // custom hooks
  const onClickViewOnBlockExplorer = useExplorer(tokenNetwork || new BraveWallet.NetworkInfo())

  // methods
  const onClickLink = React.useCallback((url?: string) => {
    if (url && isValidateUrl(url)) {
      chrome.tabs.create({ url }, () => {
        if (chrome.runtime.lastError) {
          console.error('tabs.create failed: ' + chrome.runtime.lastError.message)
        }
      })
    }
  }, [])

  const onClickWebsite = () => {
    onClickLink(nftMetadata?.contractInformation?.website)
  }

  const onClickTwitter = () => {
    onClickLink(nftMetadata?.contractInformation?.twitter)
  }

  const onClickFacebook = () => {
    onClickLink(nftMetadata?.contractInformation?.facebook)
  }

  return (
    <StyledWrapper>
      {nftMetadataError
        ? <ErrorMessage>{getLocale('braveWalletNftFetchingError')}</ErrorMessage>
        : <>
          {nftMetadata &&
            <>
              <MultimediaWrapper>
                <NftMultimedia nftMetadata={nftMetadata} />
              </MultimediaWrapper>
              <DetailColumn>
                <TokenName>
                  {nftMetadata.contractInformation.name}{' '}
                  {selectedAsset.tokenId
                    ? '#' + new Amount(selectedAsset.tokenId).toNumber()
                    : ''}
                </TokenName>
                {/* TODO: Add floorFiatPrice & floorCryptoPrice when data is available from backend: https://github.com/brave/brave-browser/issues/22627 */}
                {/* <TokenFiatValue>{CurrencySymbols[defaultCurrencies.fiat]}{nftMetadata.floorFiatPrice}</TokenFiatValue> */}
                {/* <TokenCryptoValue>{nftMetadata.floorCryptoPrice} {selectedNetwork.symbol}</TokenCryptoValue> */}
                <DetailSectionRow>
                  <DetailSectionColumn>
                    <DetailSectionTitle>
                      {getLocale('braveWalletNFTDetailBlockchain')}
                    </DetailSectionTitle>
                    <DetailSectionValue>
                      {nftMetadata.chainName}
                    </DetailSectionValue>
                  </DetailSectionColumn>
                  <DetailSectionColumn>
                    <DetailSectionTitle>
                      {getLocale('braveWalletNFTDetailTokenStandard')}
                    </DetailSectionTitle>
                    <DetailSectionValue>
                      {nftMetadata.tokenType}
                    </DetailSectionValue>
                  </DetailSectionColumn>
                  <DetailSectionColumn>
                    <DetailSectionTitle>
                      {getLocale('braveWalletNFTDetailTokenID')}
                    </DetailSectionTitle>
                    <ProjectDetailIDRow>
                      <DetailSectionValue>
                        {selectedAsset.tokenId
                          ? '#' + new Amount(selectedAsset.tokenId).toNumber()
                          : ''}
                      </DetailSectionValue>
                      <ExplorerButton
                        onClick={onClickViewOnBlockExplorer(
                          selectedAsset.coin === BraveWallet.CoinType.ETH
                            ? 'nft'
                            : 'token',
                          selectedAsset.contractAddress,
                          selectedAsset.tokenId
                        )}
                      >
                        <ExplorerIcon />
                      </ExplorerButton>
                    </ProjectDetailIDRow>
                  </DetailSectionColumn>
                </DetailSectionRow>
                <ProjectDetailRow>
                  {/* TODO: Add nft logo when data is available from backend: https://github.com/brave/brave-browser/issues/22627 */}
                  {/* <ProjectDetailImage src={nftMetadata.contractInformation.logo} /> */}
                  <ProjectDetailName>
                    {nftMetadata.contractInformation.name}
                  </ProjectDetailName>
                  {nftMetadata.contractInformation.website &&
                    nftMetadata.contractInformation.twitter &&
                    nftMetadata.contractInformation.facebook && (
                      <ProjectDetailButtonRow>
                        <ProjectDetailButton onClick={onClickWebsite}>
                          <ProjectWebsiteIcon />
                        </ProjectDetailButton>
                        <ProjectDetailButtonSeperator />
                        <ProjectDetailButton onClick={onClickTwitter}>
                          <ProjectTwitterIcon />
                        </ProjectDetailButton>
                        <ProjectDetailButtonSeperator />
                        <ProjectDetailButton onClick={onClickFacebook}>
                          <ProjectFacebookIcon />
                        </ProjectDetailButton>
                      </ProjectDetailButtonRow>
                    )}
                </ProjectDetailRow>
                <ProjectDetailDescription>
                  {nftMetadata.contractInformation.description}
                </ProjectDetailDescription>
              </DetailColumn>
            </>
          }
        </>
      }
    </StyledWrapper>

  )
}
