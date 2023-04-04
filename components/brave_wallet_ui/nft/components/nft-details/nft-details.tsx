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
import {
  isValidateUrl,
  stripERC20TokenImageURL
} from '../../../utils/string-utils'

// Styled Components
import {
  CopyIcon,
  DetailColumn,
  DetailSectionColumn,
  DetailSectionRow,
  DetailSectionTitle,
  DetailSectionValue,
  ErrorMessage,
  ExplorerButton,
  ExplorerIcon,
  NftStandard,
  ProjectDetailButton,
  ProjectDetailButtonRow,
  ProjectDetailButtonSeperator,
  ProjectDetailDescription,
  ProjectDetailIDRow,
  ProjectDetailRow,
  ProjectFacebookIcon,
  ProjectTwitterIcon,
  ProjectWebsiteIcon,
  StyledWrapper,
  TokenName,
  HighlightedDetailSectionValue,
  Subdivider,
  ProjectDetailName
} from './nft-details-styles'
import { CreateNetworkIcon } from '../../../components/shared'
import { Row } from '../../../components/shared/style'
import CopyTooltip from '../../../components/shared/copy-tooltip/copy-tooltip'
import { NftPinningStatus } from '../../../components/desktop/nft-pinning-status/nft-pinning-status'
import { PinningStatusType } from '../../../page/constants/action_types'

interface Props {
  isLoading?: boolean
  selectedAsset: BraveWallet.BlockchainToken
  nftMetadata?: NFTMetadataReturnType
  nftMetadataError?: string
  tokenNetwork?: BraveWallet.NetworkInfo
  nftPinningStatus?: PinningStatusType,
  imageIpfsUrl?: string
}

export const NftDetails = ({
  selectedAsset,
  nftMetadata,
  nftMetadataError,
  tokenNetwork,
  nftPinningStatus,
  imageIpfsUrl
}: Props) => {
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
              <DetailColumn>
                {selectedAsset.isErc721 &&
                  <NftStandard>
                    <CreateNetworkIcon network={tokenNetwork} size='small' />
                    ERC-721
                  </NftStandard>
                }
                <TokenName>
                  {selectedAsset.name}{' '}
                  {selectedAsset.tokenId
                    ? '#' + new Amount(selectedAsset.tokenId).toNumber()
                    : ''}
                </TokenName>
                {/* TODO: Add floorFiatPrice & floorCryptoPrice when data is available from backend: https://github.com/brave/brave-browser/issues/22627 */}
                {/* <TokenFiatValue>{CurrencySymbols[defaultCurrencies.fiat]}{nftMetadata.floorFiatPrice}</TokenFiatValue> */}
                {/* <TokenCryptoValue>{nftMetadata.floorCryptoPrice} {selectedNetwork.symbol}</TokenCryptoValue> */}
                <DetailSectionRow>
                  <DetailSectionColumn>
                    <DetailSectionTitle>Contract</DetailSectionTitle>
                    <Row gap='4px'>
                      <HighlightedDetailSectionValue>{selectedAsset.contractAddress}</HighlightedDetailSectionValue>
                      <CopyTooltip text={selectedAsset.contractAddress}>
                        <CopyIcon />
                      </CopyTooltip>
                    </Row>
                  </DetailSectionColumn>
                </DetailSectionRow>
                <DetailSectionRow style={{ gap: '10px' }}>
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
                <DetailSectionColumn>
                  <DetailSectionTitle>{getLocale('braveWalletNFTDetailDescription')}</DetailSectionTitle>
                  <ProjectDetailDescription>{nftMetadata.contractInformation.description}</ProjectDetailDescription>
                </DetailSectionColumn>
                {nftPinningStatus?.code === BraveWallet.TokenPinStatusCode.STATUS_PINNED &&
                  <>
                    <Subdivider />
                    <DetailSectionRow>
                      <DetailSectionColumn>
                        <DetailSectionTitle>{getLocale('braveWalletNFTDetailImageAddress')}</DetailSectionTitle>
                        <HighlightedDetailSectionValue
                          href={stripERC20TokenImageURL(imageIpfsUrl)}
                          target='_blank'>
                            {stripERC20TokenImageURL(imageIpfsUrl)}
                        </HighlightedDetailSectionValue>
                      </DetailSectionColumn>
                    </DetailSectionRow>
                  </>
                }
                {selectedAsset && nftPinningStatus?.code && nftPinningStatus.code !== BraveWallet.TokenPinStatusCode.STATUS_NOT_PINNED &&
                  <DetailSectionRow>
                    <DetailSectionColumn>
                      <Row marginBottom={16} />
                      <NftPinningStatus pinningStatusCode={nftPinningStatus.code} />
                    </DetailSectionColumn>
                  </DetailSectionRow>
                }
              </DetailColumn>
            </>
          }
        </>
      }
    </StyledWrapper>

  )
}
