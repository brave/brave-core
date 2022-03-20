import * as React from 'react'
import { BraveWallet, DefaultCurrencies, PriceDataObjectType } from '../../../../../../constants/types'
import PortfolioTopSection from '../../../portfolio/components/portfolio-top-section'
import { DividerText, Spacer, SubDivider } from '../../../portfolio/components/portfolio-top-section/style'
import {
  AssetDescription,
  AssetDescriptionWrapper,
  StyledWrapper
} from './style'

interface Props {
  selectedAsset: BraveWallet.BlockchainToken | undefined
  defaultCurrencies: DefaultCurrencies
  selectedNetwork: BraveWallet.NetworkInfo
  portfolioPriceHistory: PriceDataObjectType[]
  selectedAssetPriceHistory: PriceDataObjectType[]
  networkList: BraveWallet.NetworkInfo[]
  selectedTimeline: BraveWallet.AssetPriceTimeframe
  selectedPortfolioTimeline: BraveWallet.AssetPriceTimeframe
  selectedAssetFiatPrice: BraveWallet.AssetPrice | undefined
  selectedAssetCryptoPrice: BraveWallet.AssetPrice | undefined
  portfolioBalance: string
  isLoading: boolean
  isFetchingPortfolioPriceHistory: boolean
  goBack: () => void
  onChangeTimeline: (path: BraveWallet.AssetPriceTimeframe) => void
  onSelectNetwork: (network: BraveWallet.NetworkInfo) => void
}

const MarketAssetDetailView = (props: Props) => {
  const {
    defaultCurrencies,
    portfolioPriceHistory,
    selectedAssetPriceHistory,
    selectedNetwork,
    selectedAssetFiatPrice,
    selectedAssetCryptoPrice,
    selectedTimeline,
    selectedPortfolioTimeline,
    networkList,
    selectedAsset,
    portfolioBalance,
    isLoading,
    isFetchingPortfolioPriceHistory,
    goBack,
    onSelectNetwork,
    onChangeTimeline
  } = props

  return (
    <StyledWrapper>
      <PortfolioTopSection
        onChangeTimeline={onChangeTimeline}
        onSelectNetwork={(network) => () => onSelectNetwork(network)}
        defaultCurrencies={defaultCurrencies}
        selectedNetwork={selectedNetwork}
        portfolioPriceHistory={portfolioPriceHistory}
        selectedAssetPriceHistory={selectedAssetPriceHistory}
        selectedAssetFiatPrice={selectedAssetFiatPrice}
        selectedAssetCryptoPrice={selectedAssetCryptoPrice}
        selectedTimeline={selectedTimeline}
        selectedPortfolioTimeline={selectedPortfolioTimeline}
        networkList={networkList}
        selectedAsset={selectedAsset}
        portfolioBalance={portfolioBalance}
        isLoading={isLoading}
        isFetchingPortfolioPriceHistory={isFetchingPortfolioPriceHistory}
        goBack={goBack}
        showNetworkSelector={false}
        showToggleBalanceButton={false}
      />
      <Spacer />
      <DividerText>Information</DividerText>
      <SubDivider />
      <AssetDescriptionWrapper>
        <AssetDescription>
          Ethereum is a decentralized open-source blockchain system that
          features its own cryptocurrency. Ether ETH works as a platform for
          numerous other cryptocurrencies, as well as for the execution of
          decentralized smart contracts. Ethereum was first decribed in a 2013
          white paper by Vitalic Buterin. Buterin, along with other co-founders,
          secured funding for the project in an online public crowd sale in the
          summer of 2014 and officially launched the blockchain on July 30,
          2015. Ethereum’s own purported goal is to become a global platform for
          decentralized applications, allowing users from all over the world to
          write and run software that is resistant to censorship, downtime and
          fraud.
        </AssetDescription>
      </AssetDescriptionWrapper>
    </StyledWrapper>
  )
}

export default MarketAssetDetailView
