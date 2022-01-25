import * as React from 'react'

// Constants
import {
  PriceDataObjectType,
  AccountTransactions,
  BraveWallet,
  WalletAccountType,
  UserAssetInfoType,
  DefaultCurrencies,
  AddAccountNavTypes
} from '../../../../constants/types'
import { getLocale } from '../../../../../common/locale'
import { CurrencySymbols } from '../../../../utils/currency-symbols'

// Utils
import {
  formatWithCommasAndDecimals,
  formatTokenAmountWithCommasAndDecimals
} from '../../../../utils/format-prices'
import { sortTransactionByDate } from '../../../../utils/tx-utils'
import { formatBalance } from '../../../../utils/format-balances'

// Options
import { ChartTimelineOptions } from '../../../../options/chart-timeline-options'

// Components
import { BackButton, withPlaceholderIcon } from '../../../shared'
import {
  ChartControlBar,
  LineChart,
  SelectNetworkDropdown,
  EditVisibleAssetsModal
} from '../../'

// import NFTDetails from './components/nft-details'
import TokenLists from './components/token-lists'
import AccountsAndTransactionsList from './components/accounts-and-transctions-list'

// Hooks
import { usePricing, useTransactionParser } from '../../../../common/hooks'

// Styled Components
import {
  StyledWrapper,
  TopRow,
  BalanceTitle,
  BalanceText,
  AssetIcon,
  AssetRow,
  PriceRow,
  AssetNameText,
  DetailText,
  InfoColumn,
  PriceText,
  PercentBubble,
  PercentText,
  ArrowIcon,
  BalanceRow
} from './style'

export interface Props {
  toggleNav: () => void
  onChangeTimeline: (path: BraveWallet.AssetPriceTimeframe) => void
  onSelectAsset: (asset: BraveWallet.BlockchainToken | undefined) => void
  onSelectAccount: (account: WalletAccountType) => void
  onClickAddAccount: (tabId: AddAccountNavTypes) => () => void
  onSelectNetwork: (network: BraveWallet.EthereumChain) => void
  onAddCustomAsset: (token: BraveWallet.BlockchainToken) => void
  onShowVisibleAssetsModal: (showModal: boolean) => void
  onUpdateVisibleAssets: (updatedTokensList: BraveWallet.BlockchainToken[]) => void
  showVisibleAssetsModal: boolean
  defaultCurrencies: DefaultCurrencies
  addUserAssetError: boolean
  selectedNetwork: BraveWallet.EthereumChain
  networkList: BraveWallet.EthereumChain[]
  userAssetList: UserAssetInfoType[]
  accounts: WalletAccountType[]
  selectedTimeline: BraveWallet.AssetPriceTimeframe
  selectedPortfolioTimeline: BraveWallet.AssetPriceTimeframe
  selectedAsset: BraveWallet.BlockchainToken | undefined
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
  foundTokenInfoByContractAddress?: BraveWallet.BlockchainToken
}

const Portfolio = (props: Props) => {
  const {
    toggleNav,
    onChangeTimeline,
    onSelectAsset,
    onSelectAccount,
    onClickAddAccount,
    onSelectNetwork,
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
    portfolioBalance,
    transactions,
    userAssetList,
    isLoading,
    isFetchingPortfolioPriceHistory,
    transactionSpotPrices,
    onRetryTransaction,
    onSpeedupTransaction,
    onCancelTransaction,
    onFindTokenInfoByContractAddress,
    foundTokenInfoByContractAddress
  } = props

  const [filteredAssetList, setfilteredAssetList] = React.useState<UserAssetInfoType[]>(userAssetList)
  const [fullPortfolioFiatBalance, setFullPortfolioFiatBalance] = React.useState<string>(portfolioBalance)
  const [hoverBalance, setHoverBalance] = React.useState<string>()
  const [hoverPrice, setHoverPrice] = React.useState<string>()
  const [showNetworkDropdown, setShowNetworkDropdown] = React.useState<boolean>(false)
  const parseTransaction = useTransactionParser(selectedNetwork, accounts, transactionSpotPrices, userVisibleTokensInfo)

  const toggleShowNetworkDropdown = () => {
    setShowNetworkDropdown(!showNetworkDropdown)
  }

  const onSetFilteredAssetList = (filteredList: UserAssetInfoType[]) => {
    setfilteredAssetList(filteredList)
  }

  React.useEffect(() => {
    if (portfolioBalance !== '') {
      setFullPortfolioFiatBalance(portfolioBalance)
    }
  }, [portfolioBalance])

  React.useEffect(() => {
    setfilteredAssetList(userAssetList)
  }, [userAssetList])

  const portfolioHistory = React.useMemo(() => {
    return portfolioPriceHistory
  }, [portfolioPriceHistory])

  const selectAsset = (asset: BraveWallet.BlockchainToken) => () => {
    onSelectAsset(asset)
    toggleNav()
  }

  const goBack = () => {
    onSelectAsset(undefined)
    setfilteredAssetList(userAssetList)
    toggleNav()
  }

  const onUpdateBalance = (value: number | undefined) => {
    if (!selectedAsset) {
      if (value) {
        setHoverBalance(formatWithCommasAndDecimals(value.toString()))
      } else {
        setHoverBalance(undefined)
      }
    } else {
      if (value) {
        setHoverPrice(formatWithCommasAndDecimals(value.toString()))
      } else {
        setHoverPrice(undefined)
      }
    }
  }

  const onClickSelectNetwork = (network: BraveWallet.EthereumChain) => () => {
    onSelectNetwork(network)
    toggleShowNetworkDropdown()
  }

  const onHideNetworkDropdown = () => {
    if (showNetworkDropdown) {
      setShowNetworkDropdown(false)
    }
  }

  const toggleShowVisibleAssetModal = () => {
    onShowVisibleAssetsModal(!showVisibleAssetsModal)
  }

  const priceHistory = React.useMemo(() => {
    if (parseFloat(portfolioBalance) === 0) {
      return []
    } else {
      return portfolioHistory
    }
  }, [portfolioHistory, portfolioBalance])

  const selectedAssetTransactions = React.useMemo((): BraveWallet.TransactionInfo[] => {
    const filteredTransactions = Object.values(transactions).flatMap((txInfo) =>
      txInfo.flatMap((tx) =>
        parseTransaction(tx).symbol === selectedAsset?.symbol ? tx : []
      ))

    return sortTransactionByDate(filteredTransactions, 'descending')
  }, [selectedAsset, transactions])

  const fullAssetBalances = React.useMemo(() => {
    return filteredAssetList.find((asset) => asset.asset.contractAddress.toLowerCase() === selectedAsset?.contractAddress.toLowerCase())
  }, [filteredAssetList, selectedAsset])

  const AssetIconWithPlaceholder = React.useMemo(() => {
    return withPlaceholderIcon(AssetIcon, { size: 'big', marginLeft: 0, marginRight: 12 })
  }, [])

  const formattedFullAssetBalance = fullAssetBalances?.assetBalance
    ? '(' + formatTokenAmountWithCommasAndDecimals(
      formatBalance(fullAssetBalances?.assetBalance ?? '', selectedAsset?.decimals ?? 18),
      selectedAsset?.symbol ?? ''
    ) + ')'
    : ''

  const { computeFiatAmount } = usePricing(transactionSpotPrices)
  const fullAssetFiatBalance = fullAssetBalances?.assetBalance
    ? computeFiatAmount(
      fullAssetBalances.assetBalance,
      fullAssetBalances.asset.symbol,
      fullAssetBalances.asset.decimals
    )
    : ''

  return (
    <StyledWrapper onClick={onHideNetworkDropdown}>
      <TopRow>
        <BalanceRow>
          {!selectedAsset ? (
            <BalanceTitle>{getLocale('braveWalletBalance')}</BalanceTitle>
          ) : (
            <BackButton onSubmit={goBack} />
          )}
          {!selectedAsset?.isErc721 &&
            <SelectNetworkDropdown
              onClick={toggleShowNetworkDropdown}
              networkList={networkList}
              showNetworkDropDown={showNetworkDropdown}
              selectedNetwork={selectedNetwork}
              onSelectNetwork={onClickSelectNetwork}
            />
          }
        </BalanceRow>
        {!selectedAsset?.isErc721 &&
          <ChartControlBar
            onSubmit={onChangeTimeline}
            selectedTimeline={selectedAsset ? selectedTimeline : selectedPortfolioTimeline}
            timelineOptions={ChartTimelineOptions()}
          />
        }
      </TopRow>
      {!selectedAsset ? (
        <>
          <BalanceText>{fullPortfolioFiatBalance !== '' ? CurrencySymbols[defaultCurrencies.fiat] : ''}{hoverBalance || fullPortfolioFiatBalance}</BalanceText>
        </>
      ) : (
        <>
          {!selectedAsset.isErc721 &&
            <InfoColumn>
              <AssetRow>
                <AssetIconWithPlaceholder selectedAsset={selectedAsset} />
                <AssetNameText>{selectedAsset.name}</AssetNameText>
              </AssetRow>
              <DetailText>{selectedAsset.name} {getLocale('braveWalletPrice')} ({selectedAsset.symbol})</DetailText>
              <PriceRow>
                <PriceText>{CurrencySymbols[defaultCurrencies.fiat]}{hoverPrice || (selectedAssetFiatPrice ? formatWithCommasAndDecimals(selectedAssetFiatPrice.price) : 0.00)}</PriceText>
                <PercentBubble isDown={selectedAssetFiatPrice ? Number(selectedAssetFiatPrice.assetTimeframeChange) < 0 : false}>
                  <ArrowIcon isDown={selectedAssetFiatPrice ? Number(selectedAssetFiatPrice.assetTimeframeChange) < 0 : false} />
                  <PercentText>{selectedAssetFiatPrice ? Number(selectedAssetFiatPrice.assetTimeframeChange).toFixed(2) : 0.00}%</PercentText>
                </PercentBubble>
              </PriceRow>
              <DetailText>{selectedAssetCryptoPrice ? formatWithCommasAndDecimals(selectedAssetCryptoPrice.price) : ''} {defaultCurrencies.crypto}</DetailText>
            </InfoColumn>
          }
        </>
      )}
      {!selectedAsset?.isErc721 &&
        <LineChart
          isDown={selectedAsset && selectedAssetFiatPrice ? Number(selectedAssetFiatPrice.assetTimeframeChange) < 0 : false}
          isAsset={!!selectedAsset}
          priceData={selectedAsset ? selectedAssetPriceHistory : priceHistory}
          onUpdateBalance={onUpdateBalance}
          isLoading={selectedAsset ? isLoading : parseFloat(portfolioBalance) === 0 ? false : isFetchingPortfolioPriceHistory}
          isDisabled={selectedAsset ? false : parseFloat(portfolioBalance) === 0}
        />
      }

      {/* Temp Commented out until we have an API to get NFT MetaData */}
      {/* {selectedAsset?.isErc721 &&
        <NFTDetails
          selectedAsset={selectedAsset}
          nftMetadata={}
          defaultCurrencies={defaultCurrencies}
          selectedNetwork={selectedNetwork}
        />
      } */}

      <AccountsAndTransactionsList
        accounts={accounts}
        defaultCurrencies={defaultCurrencies}
        formattedFullAssetBalance={formattedFullAssetBalance}
        fullAssetFiatBalance={fullAssetFiatBalance}
        selectedAsset={selectedAsset}
        selectedAssetTransactions={selectedAssetTransactions}
        selectedNetwork={selectedNetwork}
        transactionSpotPrices={transactionSpotPrices}
        userVisibleTokensInfo={userVisibleTokensInfo}
        onClickAddAccount={onClickAddAccount}
        onSelectAccount={onSelectAccount}
        onSelectAsset={selectAsset}
        onCancelTransaction={onCancelTransaction}
        onRetryTransaction={onRetryTransaction}
        onSpeedupTransaction={onSpeedupTransaction}
      />
      {!selectedAsset &&
        <TokenLists
          defaultCurrencies={defaultCurrencies}
          userAssetList={userAssetList}
          filteredAssetList={filteredAssetList}
          tokenPrices={transactionSpotPrices}
          onSetFilteredAssetList={onSetFilteredAssetList}
          onSelectAsset={selectAsset}
          onShowAssetModal={toggleShowVisibleAssetModal}
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
          onFindTokenInfoByContractAddress={onFindTokenInfoByContractAddress}
          foundTokenInfoByContractAddress={foundTokenInfoByContractAddress}
          onUpdateVisibleAssets={onUpdateVisibleAssets}
        />
      }
    </StyledWrapper>
  )
}

export default Portfolio
