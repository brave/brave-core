import * as React from 'react'
import { useHistory, useParams } from 'react-router'

// Components
import { SearchBar } from '../../../shared'
import { AssetsFilterDropdown } from '../..'
import { AssetFilterOptions } from '../../../../options/market-data-filter-options'
import { makeNetworkAsset } from '../../../../options/asset-options'
import { useMarketDataManagement } from '../../../../common/hooks/market-data-management'
import { marketDataTableHeaders } from '../../../../options/market-data-headers'
import MarketDataTable from '../../../../components/market-datatable'
import { PortfolioView } from '../'

// Utils
import { debounce } from '../../../../../common/debounce'
import { getLocale } from '../../../../../common/locale'

// Types
import {
  AccountTransactions,
  AddAccountNavTypes,
  AssetFilterOption,
  BraveWallet,
  DefaultCurrencies,
  MarketDataTableColumnTypes,
  PriceDataObjectType,
  SortOrder,
  UserAssetInfoType,
  WalletAccountType,
  WalletRoutes
} from '../../../../constants/types'

// Styled Components
import {
  LoadIcon,
  LoadIconWrapper,
  StyledWrapper,
  TopRow
} from './style'

export interface Props {
  isLoadingCoinMarketData: boolean
  coinMarkets: BraveWallet.CoinMarket[]
  tradableAssets: BraveWallet.BlockchainToken[]
  defaultCurrencies: DefaultCurrencies
  selectedAsset: BraveWallet.BlockchainToken | undefined
  selectedNetwork: BraveWallet.NetworkInfo
  networkList: BraveWallet.NetworkInfo[]
  selectedTimeline: BraveWallet.AssetPriceTimeframe
  selectedPortfolioTimeline: BraveWallet.AssetPriceTimeframe
  selectedAssetFiatPrice: BraveWallet.AssetPrice | undefined
  selectedAssetCryptoPrice: BraveWallet.AssetPrice | undefined
  selectedAssetPriceHistory: PriceDataObjectType[]
  portfolioPriceHistory: PriceDataObjectType[]
  portfolioBalance: string
  isLoading: boolean
  isFetchingPortfolioPriceHistory: boolean
  onFetchCoinMarkets: (vsAsset: string, limit: number) => void
  toggleNav: () => void
  onChangeTimeline: (path: BraveWallet.AssetPriceTimeframe) => void
  onSelectAsset: (asset: BraveWallet.BlockchainToken | undefined) => void
  onSelectAccount: (account: WalletAccountType) => void
  onClickAddAccount: (tabId: AddAccountNavTypes) => () => void
  onAddCustomAsset: (token: BraveWallet.BlockchainToken) => void
  onShowVisibleAssetsModal: (showModal: boolean) => void
  onUpdateVisibleAssets: (updatedTokensList: BraveWallet.BlockchainToken[]) => void
  showVisibleAssetsModal: boolean
  addUserAssetError: boolean
  userAssetList: UserAssetInfoType[]
  accounts: WalletAccountType[]
  transactions: AccountTransactions
  fullAssetList: BraveWallet.BlockchainToken[]
  userVisibleTokensInfo: BraveWallet.BlockchainToken[]
  transactionSpotPrices: BraveWallet.AssetPrice[]
  onRetryTransaction: (transaction: BraveWallet.TransactionInfo) => void
  onSpeedupTransaction: (transaction: BraveWallet.TransactionInfo) => void
  onCancelTransaction: (transaction: BraveWallet.TransactionInfo) => void
  onFindTokenInfoByContractAddress: (contractAddress: string) => void
  foundTokenInfoByContractAddress: BraveWallet.BlockchainToken | undefined
}

interface ParamsType {
  id?: string
}

const MarketView = (props: Props) => {
  const {
    isLoadingCoinMarketData,
    coinMarkets,
    tradableAssets,
    defaultCurrencies,
    selectedAsset,
    portfolioPriceHistory,
    selectedAssetPriceHistory,
    selectedNetwork,
    selectedAssetFiatPrice,
    selectedAssetCryptoPrice,
    selectedTimeline,
    selectedPortfolioTimeline,
    networkList,
    portfolioBalance,
    isLoading,
    isFetchingPortfolioPriceHistory,
    showVisibleAssetsModal,
    addUserAssetError,
    userVisibleTokensInfo,
    fullAssetList,
    accounts,
    transactions,
    userAssetList,
    transactionSpotPrices,
    foundTokenInfoByContractAddress,
    onSelectAsset,
    onChangeTimeline,
    onFetchCoinMarkets,
    toggleNav,
    onSelectAccount,
    onClickAddAccount,
    onAddCustomAsset,
    onShowVisibleAssetsModal,
    onUpdateVisibleAssets,
    onRetryTransaction,
    onSpeedupTransaction,
    onCancelTransaction,
    onFindTokenInfoByContractAddress
  } = props
  const [coinsMarketData, setCoinsMarketData] = React.useState<BraveWallet.CoinMarket[]>([])
  const [tableHeaders, setTableHeaders] = React.useState(marketDataTableHeaders)
  const [currentFilter, setCurrentFilter] = React.useState<AssetFilterOption>('all')
  const [sortOrder, setSortOrder] = React.useState<SortOrder>('desc')
  const [sortByColumnId, setSortByColumnId] = React.useState<MarketDataTableColumnTypes>('marketCap')
  const { sortCoinMarketData, searchCoinMarkets } = useMarketDataManagement(coinsMarketData, sortOrder, sortByColumnId)
  const [currentPage, setCurrentPage] = React.useState<number>(1)
  const [searchTerm, setSearchTerm] = React.useState('')
  const [moreDataAvailable, setMoreDataAvailable] = React.useState<boolean>(false)
  const [selectedCoinMarket, setSelectedCoinMarket] = React.useState<BraveWallet.CoinMarket>()
  const [isSupportedBraveWallet, setIsSupportedBraveWallet] = React.useState<boolean>(false)
  const defaultCurrency = 'usd'
  const assetsRequestLimit = 250
  const assetsPerPage = 20
  const { id } = useParams<ParamsType>()
  const history = useHistory()

  const search = (query: string) => {
    const filtered = filterCoinMarkets(coinMarkets, currentFilter)
    const searchResults = searchCoinMarkets(filtered, query)
    setCoinsMarketData(paginate(searchResults, assetsPerPage, 1))
    setCurrentPage(1)
  }

  const debounceSearch = (query: string) => {
    return debounce(search, 500)(query)
  }

  const onSelectFilter = (value: AssetFilterOption) => {
    setCurrentFilter(value)
  }

  const paginate = (coinMarkets: BraveWallet.CoinMarket[], perPage: number, pageNumber: number) => {
    const pageData = coinMarkets.slice((pageNumber - 1) * perPage, pageNumber * perPage)
    const nextPageData = coinMarkets.slice(pageNumber * perPage, (pageNumber + 1) * perPage)
    setMoreDataAvailable(nextPageData.length > 0)

    return pageData
  }

  const tradableAssetsSymbols = React.useMemo(() => {
    return tradableAssets.map(asset => asset.symbol.toLowerCase())
  }, [tradableAssets])

  const filterCoinMarkets = (coins: BraveWallet.CoinMarket[], filter: AssetFilterOption) => {
    if (filter === 'all') {
      return coins
    } else if (filter === 'tradable') {
      const filtered = coins.filter(asset => tradableAssetsSymbols.includes(asset.symbol.toLowerCase()))
      return filtered
    }

    return []
  }

  const getNextPage = () => {
    const nextPage = currentPage + 1
    const searchResults = searchCoinMarkets(coinMarkets, searchTerm)
    const filteredCoinMarkets = filterCoinMarkets(searchResults, currentFilter)
    const nextPageData = paginate(filteredCoinMarkets, assetsPerPage, nextPage)
    setCoinsMarketData([...visibleCoinMarkets, ...nextPageData])
    setCurrentPage(nextPage)
  }

  const onSort = (columnId: MarketDataTableColumnTypes, newSortOrder: SortOrder) => {
    const updatedTableHeaders = tableHeaders.map(header => {
      if (header.id === columnId) {
        return {
          ...header,
          sortOrder: newSortOrder
        }
      } else {
        return {
          ...header,
          sortOrder: undefined
        }
      }
    })

    setTableHeaders(updatedTableHeaders)
    setSortByColumnId(columnId)
    setSortOrder(newSortOrder)
  }

  const onSelectCoinMarket = (coinMarket: BraveWallet.CoinMarket) => {
    const nativeAsset = makeNetworkAsset(selectedNetwork)
    const supportedAsset = [...fullAssetList, nativeAsset].find(a => a.symbol.toLowerCase() === coinMarket.symbol.toLowerCase())
    if (supportedAsset) {
      onSelectAsset(supportedAsset)
      setIsSupportedBraveWallet(true)
    } else {
      // Token fields have to be set here to avoid null reference
      const asset = new BraveWallet.BlockchainToken()
      asset.coingeckoId = coinMarket.id
      asset.name = coinMarket.name
      asset.contractAddress = ''
      asset.symbol = coinMarket.symbol.toUpperCase()
      asset.logo = coinMarket.image
      onSelectAsset(asset)
      setIsSupportedBraveWallet(false)
    }

    setSelectedCoinMarket(coinMarket)
    history.push(`${WalletRoutes.Market}/${coinMarket.symbol}`)
  }

  const onGoBack = () => {
    setSelectedCoinMarket(undefined)
    history.push(WalletRoutes.Market)
  }

  React.useEffect(() => {
    if (coinMarkets.length === 0) {
      onFetchCoinMarkets(defaultCurrency, assetsRequestLimit)
    }
  }, [])

  React.useEffect(() => {
    const filteredCoinMarkets = filterCoinMarkets(coinMarkets, currentFilter)
    setCoinsMarketData(paginate(filteredCoinMarkets, assetsPerPage, currentPage))
  }, [coinMarkets])

  React.useEffect(() => {
    const searchResults = searchCoinMarkets(coinMarkets, searchTerm)
    const filteredCoinMarkets = filterCoinMarkets(searchResults, currentFilter)
    setCoinsMarketData(paginate(filteredCoinMarkets, assetsPerPage, 1))
    setCurrentPage(1)
  }, [currentFilter])

  const visibleCoinMarkets = React.useMemo(() => {
    return sortCoinMarketData()
  }, [sortOrder, sortByColumnId, coinsMarketData])

  return (
    <StyledWrapper>
      {id && selectedAsset && selectedCoinMarket ? (
        <PortfolioView
          onChangeTimeline={onChangeTimeline}
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
          selectedCoinMarket={selectedCoinMarket}
          portfolioBalance={portfolioBalance}
          isLoading={isLoading}
          isFetchingPortfolioPriceHistory={isFetchingPortfolioPriceHistory}
          toggleNav={toggleNav}
          onSelectAsset={onSelectAsset}
          onSelectAccount={onSelectAccount}
          onClickAddAccount={onClickAddAccount}
          onAddCustomAsset={onAddCustomAsset}
          onShowVisibleAssetsModal={onShowVisibleAssetsModal}
          onUpdateVisibleAssets={onUpdateVisibleAssets}
          showVisibleAssetsModal={showVisibleAssetsModal}
          addUserAssetError={addUserAssetError}
          userAssetList={userAssetList}
          accounts={accounts}
          transactions={transactions}
          fullAssetList={fullAssetList}
          userVisibleTokensInfo={userVisibleTokensInfo}
          transactionSpotPrices={transactionSpotPrices}
          onRetryTransaction={onRetryTransaction}
          onSpeedupTransaction={onSpeedupTransaction}
          onCancelTransaction={onCancelTransaction}
          onFindTokenInfoByContractAddress={onFindTokenInfoByContractAddress}
          foundTokenInfoByContractAddress={foundTokenInfoByContractAddress}
          onGoBack={onGoBack}
          isSupportedInBraveWallet={isSupportedBraveWallet}
        />
      ) : (
        <>
          <TopRow>
            <AssetsFilterDropdown
              options={AssetFilterOptions()}
              value={currentFilter}
              onSelectFilter={onSelectFilter}
            />
            <SearchBar
              placeholder={getLocale('braveWalletSearchText')}
              autoFocus={true}
              action={(event) => {
                setSearchTerm(event.target.value)
                debounceSearch(event.target.value)
              }}
              disabled={isLoadingCoinMarketData}
            />
          </TopRow>
          {isLoadingCoinMarketData ? (
            <LoadIconWrapper>
              <LoadIcon />
            </LoadIconWrapper>
          ) : (
            <MarketDataTable
              headers={tableHeaders}
              coinMarketData={visibleCoinMarkets}
              moreDataAvailable={moreDataAvailable}
              showEmptyState={searchTerm !== '' || currentFilter !== 'all'}
              onFetchMoreMarketData={getNextPage}
              onSort={onSort}
              onSelectCoinMarket={onSelectCoinMarket}
            />
          )}
        </>
      )}
    </StyledWrapper>
  )
}

export default MarketView
