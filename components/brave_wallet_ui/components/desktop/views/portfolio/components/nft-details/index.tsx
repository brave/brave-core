import * as React from 'react'

// Types
import {
  BraveWallet,
  NFTMetadataReturnType,
  DefaultCurrencies
} from '../../../../../../constants/types'

// Hooks
import { useExplorer } from '../../../../../../common/hooks'

// Utils
import Amount from '../../../../../../utils/amount'
import { getLocale } from '$web-common/locale'

// Styled Components
import {
  StyledWrapper,
  LoadIcon,
  LoadingOverlay,
  DetailColumn,
  TokenName,
  DetailSectionRow,
  DetailSectionColumn,
  DetailSectionTitle,
  DetailSectionValue,
  ProjectDetailRow,
  ProjectDetailName,
  ProjectDetailDescription,
  ProjectDetailButtonRow,
  ProjectDetailButton,
  ProjectDetailButtonSeperator,
  ProjectWebsiteIcon,
  ProjectTwitterIcon,
  ProjectFacebookIcon,
  ProjectDetailIDRow,
  ExplorerIcon,
  ExplorerButton,
  NTFImageIframe,
  nftImageDimension,
  NftImageWrapper,
  NFTImageSkeletonWrapper
} from './style'
import { LoadingSkeleton } from '../../../../../shared'

export interface Props {
  isLoading: boolean
  selectedAsset: BraveWallet.BlockchainToken
  nftMetadata: NFTMetadataReturnType | undefined
  defaultCurrencies: DefaultCurrencies
  selectedNetwork: BraveWallet.NetworkInfo
}

const NFTDetails = (props: Props) => {
  const {
    isLoading,
    selectedAsset,
    nftMetadata,
    selectedNetwork
  } = props
  const [isImageLoaded, setIsImageLoaded] = React.useState(false)

  const onClickViewOnBlockExplorer = useExplorer(selectedNetwork)

  const onClickWebsite = () => {
    chrome.tabs.create({ url: nftMetadata?.contractInformation?.website }, () => {
      if (chrome.runtime.lastError) {
        console.error('tabs.create failed: ' + chrome.runtime.lastError.message)
      }
    })
  }

  const onClickTwitter = () => {
    chrome.tabs.create({ url: nftMetadata?.contractInformation?.twitter }, () => {
      if (chrome.runtime.lastError) {
        console.error('tabs.create failed: ' + chrome.runtime.lastError.message)
      }
    })
  }

  const onClickFacebook = () => {
    chrome.tabs.create({ url: nftMetadata?.contractInformation?.facebook }, () => {
      if (chrome.runtime.lastError) {
        console.error('tabs.create failed: ' + chrome.runtime.lastError.message)
      }
    })
  }

  return (
    <StyledWrapper>
      {isLoading &&
        <LoadingOverlay isLoading={isLoading}>
          <LoadIcon />
        </LoadingOverlay>
      }
      {nftMetadata &&
        <>
          <NftImageWrapper isLoading={!isImageLoaded}>
            <NTFImageIframe
              src={`chrome-untrusted://nft-display?imageUrl=${encodeURIComponent(nftMetadata.imageURL)}&imageWidth=${nftImageDimension}&imageHeight=${nftImageDimension}`}
              sandbox="allow-scripts"
              onLoad={() => setIsImageLoaded(true)}
            />
          </NftImageWrapper>
          {!isImageLoaded &&
            <LoadingSkeleton wrapper={NFTImageSkeletonWrapper} />
          }
          <DetailColumn>
            <TokenName>
              {selectedAsset.name} {
                selectedAsset.tokenId
                  ? '#' + new Amount(selectedAsset.tokenId).toNumber()
                  : ''
              }
            </TokenName>
            {/* TODO: Add floorFiatPrice & floorCryptoPrice when data is available from backend: https://github.com/brave/brave-browser/issues/22627 */}
            {/* <TokenFiatValue>{CurrencySymbols[defaultCurrencies.fiat]}{nftMetadata.floorFiatPrice}</TokenFiatValue> */}
            {/* <TokenCryptoValue>{nftMetadata.floorCryptoPrice} {selectedNetwork.symbol}</TokenCryptoValue> */}
            <DetailSectionRow>
              <DetailSectionColumn>
                <DetailSectionTitle>{getLocale('braveWalletNFTDetailBlockchain')}</DetailSectionTitle>
                <DetailSectionValue>{nftMetadata.chainName}</DetailSectionValue>
              </DetailSectionColumn>
              <DetailSectionColumn>
                <DetailSectionTitle>{getLocale('braveWalletNFTDetailTokenStandard')}</DetailSectionTitle>
                <DetailSectionValue>{nftMetadata.tokenType}</DetailSectionValue>
              </DetailSectionColumn>
              <DetailSectionColumn>
                <DetailSectionTitle>{getLocale('braveWalletNFTDetailTokenID')}</DetailSectionTitle>
                <ProjectDetailIDRow>
                  <DetailSectionValue>
                    {
                      selectedAsset.tokenId
                        ? '#' + new Amount(selectedAsset.tokenId).toNumber()
                        : ''
                    }
                  </DetailSectionValue>
                  <ExplorerButton onClick={onClickViewOnBlockExplorer('contract', selectedAsset.contractAddress, selectedAsset.tokenId)}>
                    <ExplorerIcon />
                  </ExplorerButton>
                </ProjectDetailIDRow>
              </DetailSectionColumn>
            </DetailSectionRow>
            <ProjectDetailRow>
              {/* TODO: Add nft logo when data is available from backend: https://github.com/brave/brave-browser/issues/22627 */}
              {/* <ProjectDetailImage src={nftMetadata.contractInformation.logo} /> */}
              <ProjectDetailName>{nftMetadata.contractInformation.name}</ProjectDetailName>
              {nftMetadata.contractInformation.website && nftMetadata.contractInformation.twitter && nftMetadata.contractInformation.facebook &&
                <ProjectDetailButtonRow>
                  <ProjectDetailButton onClick={onClickWebsite}>
                    <ProjectWebsiteIcon/>
                  </ProjectDetailButton>
                  <ProjectDetailButtonSeperator/>
                  <ProjectDetailButton onClick={onClickTwitter}>
                    <ProjectTwitterIcon/>
                  </ProjectDetailButton>
                  <ProjectDetailButtonSeperator/>
                  <ProjectDetailButton onClick={onClickFacebook}>
                    <ProjectFacebookIcon/>
                  </ProjectDetailButton>
                </ProjectDetailButtonRow>
              }
            </ProjectDetailRow>
            <ProjectDetailDescription>{nftMetadata.contractInformation.description}</ProjectDetailDescription>
          </DetailColumn>
        </>
      }
    </StyledWrapper>

  )
}

export default NFTDetails
