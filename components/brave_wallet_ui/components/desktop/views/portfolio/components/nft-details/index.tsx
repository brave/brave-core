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
import { CurrencySymbols } from '../../../../../../utils/currency-symbols'
import { getLocale } from '../../../../../../../common/locale'

// Styled Components
import {
  StyledWrapper,
  NFTImage,
  DetailColumn,
  TokenName,
  TokenFiatValue,
  TokenCryptoValue,
  DetailSectionRow,
  DetailSectionColumn,
  DetailSectionTitle,
  DetailSectionValue,
  ProjectDetailRow,
  ProjectDetailName,
  ProjectDetailDescription,
  ProjectDetailImage,
  ProjectDetailButtonRow,
  ProjectDetailButton,
  ProjectDetailButtonSeperator,
  ProjectWebsiteIcon,
  ProjectTwitterIcon,
  ProjectFacebookIcon,
  ProjectDetailIDRow,
  ExplorerIcon,
  ExplorerButton
} from './style'

export interface Props {
  selectedAsset: BraveWallet.BlockchainToken
  nftMetadata: NFTMetadataReturnType
  defaultCurrencies: DefaultCurrencies
  selectedNetwork: BraveWallet.EthereumChain
}

const NFTDetails = (props: Props) => {
  const {
    selectedAsset,
    nftMetadata,
    defaultCurrencies,
    selectedNetwork
  } = props

  const onClickViewOnBlockExplorer = useExplorer(selectedNetwork)

  const onClickWebsite = () => {
    chrome.tabs.create({ url: nftMetadata.contractInformation.website }, () => {
      if (chrome.runtime.lastError) {
        console.error('tabs.create failed: ' + chrome.runtime.lastError.message)
      }
    })
  }

  const onClickTwitter = () => {
    chrome.tabs.create({ url: nftMetadata.contractInformation.twitter }, () => {
      if (chrome.runtime.lastError) {
        console.error('tabs.create failed: ' + chrome.runtime.lastError.message)
      }
    })
  }

  const onClickFacebook = () => {
    chrome.tabs.create({ url: nftMetadata.contractInformation.facebook }, () => {
      if (chrome.runtime.lastError) {
        console.error('tabs.create failed: ' + chrome.runtime.lastError.message)
      }
    })
  }

  return (
    <StyledWrapper>
      <NFTImage src={selectedAsset.logo} />
      <DetailColumn>
        <TokenName>
          {selectedAsset.name} {
            selectedAsset.tokenId
              ? '#' + new Amount(selectedAsset.tokenId).toNumber()
              : ''
          }
        </TokenName>
        <TokenFiatValue>{CurrencySymbols[defaultCurrencies.fiat]}{nftMetadata.floorFiatPrice}</TokenFiatValue>
        <TokenCryptoValue>{nftMetadata.floorCryptoPrice} {selectedNetwork.symbol}</TokenCryptoValue>
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
          <ProjectDetailImage src={nftMetadata.contractInformation.logo} />
          <ProjectDetailName>{nftMetadata.contractInformation.name}</ProjectDetailName>
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
        </ProjectDetailRow>
        <ProjectDetailDescription>{nftMetadata.contractInformation.description}</ProjectDetailDescription>
      </DetailColumn>
    </StyledWrapper>

  )
}

export default NFTDetails
