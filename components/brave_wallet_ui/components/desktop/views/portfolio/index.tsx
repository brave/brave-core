import * as React from 'react'

// Constants
import {
  PriceDataObjectType,
  AccountTransactions,
  BraveWallet,
  WalletAccountType,
  UserAssetInfoType,
  DefaultCurrencies,
  AddAccountNavTypes,
  SupportedTestNetworks
} from '../../../../constants/types'
import { getLocale } from '../../../../../common/locale'

// Utils
import { sortTransactionByDate } from '../../../../utils/tx-utils'
import { getTokensNetwork, getTokensCoinType } from '../../../../utils/network-utils'
import Amount from '../../../../utils/amount'

// Options

// Components
import {
  EditVisibleAssetsModal
} from '../../'

// import NFTDetails from './components/nft-details'
import TokenLists from './components/token-lists'
import AccountsAndTransactionsList from './components/accounts-and-transctions-list'

// Hooks
import { usePricing, useTransactionParser } from '../../../../common/hooks'

// Styled Components
import {
  AssetNotSupported,
  CoinGeckoText,
  StyledWrapper
} from './style'
import PortfolioTopSection from './components/portfolio-top-section'
import AssetStats from './components/asset-stats'

export interface Props {
  toggleNav: () => void
  onChangeTimeline: (path: BraveWallet.AssetPriceTimeframe) => void
  onSelectAsset: (asset: BraveWallet.BlockchainToken | undefined) => void
  onSelectAccount: (account: WalletAccountType) => void
  onClickAddAccount: (tabId: AddAccountNavTypes) => () => void
  onAddCustomAsset: (token: BraveWallet.BlockchainToken) => void
  onShowVisibleAssetsModal: (showModal: boolean) => void
  onUpdateVisibleAssets: (updatedTokensList: BraveWallet.BlockchainToken[]) => void
  showVisibleAssetsModal: boolean
  defaultCurrencies: DefaultCurrencies
  addUserAssetError: boolean
  selectedNetwork: BraveWallet.NetworkInfo
  networkList: BraveWallet.NetworkInfo[]
  userAssetList: UserAssetInfoType[]
  accounts: WalletAccountType[]
  selectedTimeline: BraveWallet.AssetPriceTimeframe
  selectedPortfolioTimeline: BraveWallet.AssetPriceTimeframe
  selectedAsset: BraveWallet.BlockchainToken | undefined
  selectedCoinMarket: BraveWallet.CoinMarket | undefined
  selectedAssetFiatPrice: BraveWallet.AssetPrice | undefined
  selectedAssetCryptoPrice: BraveWallet.AssetPrice | undefined
  selectedAssetPriceHistory: PriceDataObjectType[]
  portfolioPriceHistory: PriceDataObjectType[]
  portfolioBalance: string
  transactions: AccountTransactions
  isLoading: boolean
  fullAssetList: BraveWallet.BlockchainToken[]
  userVisibleTokensInfo: BraveWallet.BlockchainToken[]
  isFetchingPortfolioPriceHistory: boolean
  transactionSpotPrices: BraveWallet.AssetPrice[]
  onRetryTransaction: (transaction: BraveWallet.TransactionInfo) => void
  onSpeedupTransaction: (transaction: BraveWallet.TransactionInfo) => void
  onCancelTransaction: (transaction: BraveWallet.TransactionInfo) => void
  onFindTokenInfoByContractAddress: (contractAddress: string) => void
  foundTokenInfoByContractAddress: BraveWallet.BlockchainToken | undefined
  isSupportedInBraveWallet: boolean
  onGoBack?: () => void
}

const Portfolio = (props: Props) => {
  const {
    toggleNav,
    onChangeTimeline,
    onSelectAsset,
    onSelectAccount,
    onClickAddAccount,
    onAddCustomAsset,
    onShowVisibleAssetsModal,
    onUpdateVisibleAssets,
    showVisibleAssetsModal,
    defaultCurrencies,
    addUserAssetError,
    userVisibleTokensInfo,
    selectedNetwork,
    fullAssetList,
    portfolioPriceHistory,
    selectedAssetPriceHistory,
    selectedAssetFiatPrice,
    selectedAssetCryptoPrice,
    selectedTimeline,
    selectedPortfolioTimeline,
    accounts,
    networkList,
    selectedAsset,
    selectedCoinMarket,
    portfolioBalance,
    transactions,
    userAssetList,
    isLoading,
    isFetchingPortfolioPriceHistory,
    transactionSpotPrices,
    isSupportedInBraveWallet,
    onRetryTransaction,
    onSpeedupTransaction,
    onCancelTransaction,
    onFindTokenInfoByContractAddress,
    foundTokenInfoByContractAddress,
    onGoBack
  } = props

  // This will be removed once Network Filtering is integrated here
  // https://github.com/brave/brave-browser/issues/20780
  const mainnetAssets = React.useMemo(() => {
    return userAssetList.filter((asset) => !SupportedTestNetworks.includes(asset.asset.chainId))
  }, [userAssetList])

  const [filteredAssetList, setfilteredAssetList] = React.useState<UserAssetInfoType[]>(mainnetAssets)
  const [showNetworkDropdown, setShowNetworkDropdown] = React.useState<boolean>(false)
  const [hideBalances, setHideBalances] = React.useState<boolean>(false)
  const parseTransaction = useTransactionParser(selectedNetwork, accounts, transactionSpotPrices, userVisibleTokensInfo)

  const onSetFilteredAssetList = (filteredList: UserAssetInfoType[]) => {
    setfilteredAssetList(filteredList)
  }

  React.useEffect(() => {
    setfilteredAssetList(mainnetAssets)
  }, [mainnetAssets])

  const selectAsset = (asset: BraveWallet.BlockchainToken) => () => {
    onSelectAsset(asset)
    toggleNav()
  }

  const goBack = () => {
    onSelectAsset(undefined)
    setfilteredAssetList(userAssetList)
    toggleNav()
    if (onGoBack) {
      onGoBack()
    }
  }

  const onHideNetworkDropdown = () => {
    if (showNetworkDropdown) {
      setShowNetworkDropdown(false)
    }
  }

  const toggleShowVisibleAssetModal = () => {
    onShowVisibleAssetsModal(!showVisibleAssetsModal)
  }

  const selectedAssetTransactions = React.useMemo((): BraveWallet.TransactionInfo[] => {
    const filteredTransactions = Object.values(transactions).flatMap((txInfo) =>
      txInfo.flatMap((tx) =>
        parseTransaction(tx).symbol === selectedAsset?.symbol ? tx : []
      ))

    return sortTransactionByDate(filteredTransactions, 'descending')
  }, [selectedAsset, transactions])

  const fullAssetBalances = React.useMemo(() => {
    if (selectedAsset?.contractAddress === '') {
      return filteredAssetList.find(
        (asset) =>
          asset.asset.symbol.toLowerCase() === selectedAsset?.symbol.toLowerCase() &&
          asset.asset.chainId === selectedAsset?.chainId
      )
    }
    return filteredAssetList.find(
      (asset) =>
        asset.asset.contractAddress.toLowerCase() === selectedAsset?.contractAddress.toLowerCase() &&
        asset.asset.chainId === selectedAsset?.chainId
    )
  }, [filteredAssetList, selectedAsset])

  const formattedFullAssetBalance = fullAssetBalances?.assetBalance
    ? '(' + new Amount(fullAssetBalances?.assetBalance ?? '')
      .divideByDecimals(selectedAsset?.decimals ?? 18)
      .formatAsAsset(6, selectedAsset?.symbol ?? '') + ')'
    : ''

  const { computeFiatAmount } = usePricing(transactionSpotPrices)
  const fullAssetFiatBalance = fullAssetBalances?.assetBalance
    ? computeFiatAmount(
      fullAssetBalances.assetBalance,
      fullAssetBalances.asset.symbol,
      fullAssetBalances.asset.decimals
    )
    : Amount.empty()

  const onToggleHideBalances = () => {
    setHideBalances(!hideBalances)
  }

  const filteredAccountsByCoinType = React.useMemo(() => {
    if (!selectedAsset) {
      return []
    }
    const coinType = getTokensCoinType(networkList, selectedAsset)
    return accounts.filter((account) => account.coin === coinType)
  }, [networkList, accounts, selectedAsset])

  const selectedAssetsNetwork = React.useMemo(() => {
    if (!selectedAsset) {
      return
    }
    return getTokensNetwork(networkList, selectedAsset)
  }, [selectedAsset, networkList])

  return (
    <StyledWrapper onClick={onHideNetworkDropdown}>
      <PortfolioTopSection
        onChangeTimeline={onChangeTimeline}
        defaultCurrencies={defaultCurrencies}
        selectedAssetsNetwork={selectedAssetsNetwork}
        networkList={networkList}
        selectedTimeline={selectedTimeline}
        selectedPortfolioTimeline={selectedPortfolioTimeline}
        selectedAsset={selectedAsset}
        selectedAssetFiatPrice={selectedAssetFiatPrice}
        selectedAssetCryptoPrice={selectedAssetCryptoPrice}
        selectedAssetPriceHistory={selectedAssetPriceHistory}
        portfolioPriceHistory={portfolioPriceHistory}
        portfolioBalance={portfolioBalance}
        isLoading={isLoading}
        isFetchingPortfolioPriceHistory={isFetchingPortfolioPriceHistory}
        showToggleBalanceButton={isSupportedInBraveWallet}
        onToggleHideBalances={onToggleHideBalances}
        goBack={goBack}
        hideBalances={hideBalances}
      />
      {/* Temp Commented out until we have an API to get NFT MetaData */}
      {/* {selectedAsset?.isErc721 &&
        <NFTDetails
          selectedAsset={selectedAsset}
          nftMetadata={}
          defaultCurrencies={defaultCurrencies}
          selectedNetwork={selectedNetwork}
        />
      } */}

      {isSupportedInBraveWallet
        ? <AccountsAndTransactionsList
            accounts={filteredAccountsByCoinType}
            defaultCurrencies={defaultCurrencies}
            formattedFullAssetBalance={formattedFullAssetBalance}
            fullAssetFiatBalance={fullAssetFiatBalance}
            selectedAsset={selectedAsset}
            selectedAssetTransactions={selectedAssetTransactions}
            selectedNetwork={selectedAssetsNetwork ?? selectedNetwork}
            transactionSpotPrices={transactionSpotPrices}
            userVisibleTokensInfo={userVisibleTokensInfo}
            onClickAddAccount={onClickAddAccount}
            onSelectAccount={onSelectAccount}
            onSelectAsset={selectAsset}
            onCancelTransaction={onCancelTransaction}
            onRetryTransaction={onRetryTransaction}
            onSpeedupTransaction={onSpeedupTransaction}
            hideBalances={hideBalances}
            networkList={networkList}
          />
        : <>
            <AssetNotSupported>{getLocale('braveWalletAssetNotSupported')}</AssetNotSupported>
          </>
      }

      {selectedCoinMarket &&
        <AssetStats selectedCoinMarket={selectedCoinMarket}/>
      }

      {!selectedAsset &&
        <TokenLists
          defaultCurrencies={defaultCurrencies}
          userAssetList={mainnetAssets}
          filteredAssetList={filteredAssetList}
          tokenPrices={transactionSpotPrices}
          networks={networkList}
          onSetFilteredAssetList={onSetFilteredAssetList}
          onSelectAsset={selectAsset}
          onShowAssetModal={toggleShowVisibleAssetModal}
          hideBalances={hideBalances}
        />
      }
      {showVisibleAssetsModal &&
        <EditVisibleAssetsModal
          fullAssetList={fullAssetList}
          userVisibleTokensInfo={userVisibleTokensInfo}
          addUserAssetError={addUserAssetError}
          onClose={toggleShowVisibleAssetModal}
          onAddCustomAsset={onAddCustomAsset}
          selectedNetwork={selectedNetwork}
          networkList={networkList}
          onFindTokenInfoByContractAddress={onFindTokenInfoByContractAddress}
          foundTokenInfoByContractAddress={foundTokenInfoByContractAddress}
          onUpdateVisibleAssets={onUpdateVisibleAssets}
        />
      }

      <CoinGeckoText>{getLocale('braveWalletPoweredByCoinGecko')}</CoinGeckoText>
    </StyledWrapper>
  )
}

export default Portfolio
