import * as React from 'react'

// Constants
import {
  PriceDataObjectType,
  AccountTransactions,
  BraveWallet,
  WalletAccountType,
  AccountAssetOptionType,
  DefaultCurrencies,
  AddAccountNavTypes
} from '../../../../constants/types'
import { getLocale } from '../../../../../common/locale'
import { CurrencySymbols } from '../../../../utils/currency-symbols'

// Utils
import {
  formatWithCommasAndDecimals,
  formatFiatAmountWithCommasAndDecimals,
  formatTokenAmountWithCommasAndDecimals
} from '../../../../utils/format-prices'
import { formatBalance } from '../../../../utils/format-balances'
import { sortTransactionByDate } from '../../../../utils/tx-utils'

// Options
import { ChartTimelineOptions } from '../../../../options/chart-timeline-options'
// import { ETH } from '../../../../options/asset-options'

// Components
import { SearchBar, BackButton, withPlaceholderIcon } from '../../../shared'
import {
  ChartControlBar,
  LineChart,
  PortfolioAssetItem,
  AddButton,
  PortfolioAccountItem,
  PortfolioTransactionItem,
  SelectNetworkDropdown,
  EditVisibleAssetsModal
} from '../../'

// Hooks
import { useTransactionParser } from '../../../../common/hooks'

// Styled Components
import {
  StyledWrapper,
  TopRow,
  BalanceTitle,
  BalanceText,
  ButtonRow,
  AssetIcon,
  AssetRow,
  PriceRow,
  AssetNameText,
  DetailText,
  InfoColumn,
  PriceText,
  DividerText,
  SubDivider,
  PercentBubble,
  PercentText,
  ArrowIcon,
  BalanceRow,
  EmptyTransactionContainer,
  TransactionPlaceholderText,
  AssetBalanceDisplay,
  DividerRow,
  Spacer,
  CoinGeckoText
} from './style'

export interface Props {
  toggleNav: () => void
  onChangeTimeline: (path: BraveWallet.AssetPriceTimeframe) => void
  onSelectAsset: (asset: BraveWallet.ERCToken | undefined) => void
  onSelectAccount: (account: WalletAccountType) => void
  onClickAddAccount: (tabId: AddAccountNavTypes) => () => void
  onSelectNetwork: (network: BraveWallet.EthereumChain) => void
  onAddUserAsset: (token: BraveWallet.ERCToken) => void
  onSetUserAssetVisible: (token: BraveWallet.ERCToken, isVisible: boolean) => void
  onShowVisibleAssetsModal: (showModal: boolean) => void
  onRemoveUserAsset: (token: BraveWallet.ERCToken) => void
  showVisibleAssetsModal: boolean
  defaultCurrencies: DefaultCurrencies
  addUserAssetError: boolean
  selectedNetwork: BraveWallet.EthereumChain
  networkList: BraveWallet.EthereumChain[]
  userAssetList: AccountAssetOptionType[]
  accounts: WalletAccountType[]
  selectedTimeline: BraveWallet.AssetPriceTimeframe
  selectedPortfolioTimeline: BraveWallet.AssetPriceTimeframe
  selectedAsset: BraveWallet.ERCToken | undefined
  selectedAssetFiatPrice: BraveWallet.AssetPrice | undefined
  selectedAssetCryptoPrice: BraveWallet.AssetPrice | undefined
  selectedAssetPriceHistory: PriceDataObjectType[]
  portfolioPriceHistory: PriceDataObjectType[]
  portfolioBalance: string
  transactions: AccountTransactions
  isLoading: boolean
  fullAssetList: BraveWallet.ERCToken[]
  userVisibleTokensInfo: BraveWallet.ERCToken[]
  isFetchingPortfolioPriceHistory: boolean
  transactionSpotPrices: BraveWallet.AssetPrice[]
  onRetryTransaction: (transaction: BraveWallet.TransactionInfo) => void
  onSpeedupTransaction: (transaction: BraveWallet.TransactionInfo) => void
  onCancelTransaction: (transaction: BraveWallet.TransactionInfo) => void
  onFindTokenInfoByContractAddress: (contractAddress: string) => void
  foundTokenInfoByContractAddress?: BraveWallet.ERCToken
}

const Portfolio = (props: Props) => {
  const {
    toggleNav,
    onChangeTimeline,
    onSelectAsset,
    onSelectAccount,
    onClickAddAccount,
    onSelectNetwork,
    onAddUserAsset,
    onSetUserAssetVisible,
    onRemoveUserAsset,
    onShowVisibleAssetsModal,
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

  const [filteredAssetList, setfilteredAssetList] = React.useState<AccountAssetOptionType[]>(userAssetList)
  const [fullPortfolioFiatBalance, setFullPortfolioFiatBalance] = React.useState<string>(portfolioBalance)
  const [hoverBalance, setHoverBalance] = React.useState<string>()
  const [hoverPrice, setHoverPrice] = React.useState<string>()
  const [showNetworkDropdown, setShowNetworkDropdown] = React.useState<boolean>(false)
  const parseTransaction = useTransactionParser(selectedNetwork, accounts, transactionSpotPrices, userVisibleTokensInfo)

  const toggleShowNetworkDropdown = () => {
    setShowNetworkDropdown(!showNetworkDropdown)
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

  // This filters a list of assets when the user types in search bar
  const filterAssets = (event: any) => {
    const search = event.target.value
    if (search === '') {
      setfilteredAssetList(userAssetList)
    } else {
      const filteredList = userAssetList.filter((item) => {
        return (
          item.asset.name.toLowerCase() === search.toLowerCase() ||
          item.asset.name.toLowerCase().startsWith(search.toLowerCase()) ||
          item.asset.symbol.toLocaleLowerCase() === search.toLowerCase() ||
          item.asset.symbol.toLowerCase().startsWith(search.toLowerCase())
        )
      })
      setfilteredAssetList(filteredList)
    }
  }

  const selectAsset = (asset: BraveWallet.ERCToken) => () => {
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

  const getFiatBalance = (account: WalletAccountType, asset: BraveWallet.ERCToken) => {
    const found = account.tokens.find((token) => token.asset.contractAddress === asset.contractAddress)
    return (found) ? found.fiatBalance : ''
  }

  const getAssetBalance = (account: WalletAccountType, asset: BraveWallet.ERCToken) => {
    const found = account.tokens.find((token) => token.asset.contractAddress === asset.contractAddress)
    return (found) ? formatBalance(found.assetBalance, found.asset.decimals) : ''
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

  const findAccount = (address: string): WalletAccountType | undefined => {
    return accounts.find((account) => address === account.address)
  }

  const fullAssetBalances = React.useMemo(() => {
    return filteredAssetList.find((asset) => asset.asset.contractAddress.toLowerCase() === selectedAsset?.contractAddress.toLowerCase())
  }, [filteredAssetList, selectedAsset])

  const AssetIconWithPlaceholder = React.useMemo(() => {
    return withPlaceholderIcon(AssetIcon, { size: 'big', marginLeft: 0, marginRight: 12 })
  }, [])

  const erc271Tokens = React.useMemo(() => filteredAssetList.filter((token) => token.asset.isErc721), [filteredAssetList])

  const formatedFullAssetBalance = fullAssetBalances?.assetBalance
    ? '(' + formatTokenAmountWithCommasAndDecimals(fullAssetBalances?.assetBalance ?? '', selectedAsset?.symbol ?? '') + ')'
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
          <SelectNetworkDropdown
            onClick={toggleShowNetworkDropdown}
            networkList={networkList}
            showNetworkDropDown={showNetworkDropdown}
            selectedNetwork={selectedNetwork}
            onSelectNetwork={onClickSelectNetwork}
          />
        </BalanceRow>
        <ChartControlBar
          onSubmit={onChangeTimeline}
          selectedTimeline={selectedAsset ? selectedTimeline : selectedPortfolioTimeline}
          timelineOptions={ChartTimelineOptions()}
        />
      </TopRow>
      {!selectedAsset ? (
        <>
          <BalanceText>{fullPortfolioFiatBalance !== '' ? CurrencySymbols[defaultCurrencies.fiat] : ''}{hoverBalance || fullPortfolioFiatBalance}</BalanceText>
        </>
      ) : (
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
          <DetailText>{selectedAssetCryptoPrice ? selectedAssetCryptoPrice.price : ''} {defaultCurrencies.crypto}</DetailText>
        </InfoColumn>
      )}
      <LineChart
        isDown={selectedAsset && selectedAssetFiatPrice ? Number(selectedAssetFiatPrice.assetTimeframeChange) < 0 : false}
        isAsset={!!selectedAsset}
        priceData={selectedAsset ? selectedAssetPriceHistory : priceHistory}
        onUpdateBalance={onUpdateBalance}
        isLoading={selectedAsset ? isLoading : parseFloat(portfolioBalance) === 0 ? false : isFetchingPortfolioPriceHistory}
        isDisabled={selectedAsset ? false : parseFloat(portfolioBalance) === 0}
      />
      {selectedAsset &&
        <>
          <DividerRow>
            <DividerText>{getLocale('braveWalletAccounts')}</DividerText>
            <AssetBalanceDisplay>{formatFiatAmountWithCommasAndDecimals(fullAssetBalances?.fiatBalance ?? '', defaultCurrencies.fiat)} {formatedFullAssetBalance}</AssetBalanceDisplay>
          </DividerRow>
          <SubDivider />
          {accounts.map((account) =>
            <PortfolioAccountItem
              defaultCurrencies={defaultCurrencies}
              key={account.address}
              assetTicker={selectedAsset.symbol}
              name={account.name}
              address={account.address}
              fiatBalance={getFiatBalance(account, selectedAsset)}
              assetBalance={getAssetBalance(account, selectedAsset)}
              selectedNetwork={selectedNetwork}
            />
          )}
          <ButtonRow>
            <AddButton
              buttonType='secondary'
              onSubmit={onClickAddAccount('create')}
              text={getLocale('braveWalletAddAccount')}
            />
          </ButtonRow>
          <DividerText>{getLocale('braveWalletTransactions')}</DividerText>
          <SubDivider />
          {selectedAssetTransactions.length !== 0 ? (
            <>
              {selectedAssetTransactions.map((transaction: BraveWallet.TransactionInfo) =>
                <PortfolioTransactionItem
                  defaultCurrencies={defaultCurrencies}
                  key={transaction.id}
                  selectedNetwork={selectedNetwork}
                  accounts={accounts}
                  transaction={transaction}
                  account={findAccount(transaction.fromAddress)}
                  transactionSpotPrices={transactionSpotPrices}
                  visibleTokens={userVisibleTokensInfo}
                  displayAccountName={true}
                  onSelectAccount={onSelectAccount}
                  onSelectAsset={onSelectAsset}
                  onRetryTransaction={onRetryTransaction}
                  onSpeedupTransaction={onSpeedupTransaction}
                  onCancelTransaction={onCancelTransaction}
                />
              )}
            </>
          ) : (
            <EmptyTransactionContainer>
              <TransactionPlaceholderText>{getLocale('braveWalletTransactionPlaceholder')}</TransactionPlaceholderText>
            </EmptyTransactionContainer>
          )}
          <CoinGeckoText>{getLocale('braveWalletPoweredByCoinGecko')}</CoinGeckoText>
        </>
      }
      {!selectedAsset &&
        <>
          <SearchBar placeholder={getLocale('braveWalletSearchText')} action={filterAssets} />
          {filteredAssetList.filter((asset) => !asset.asset.isErc721).map((item) =>
            <PortfolioAssetItem
              defaultCurrencies={defaultCurrencies}
              action={selectAsset(item.asset)}
              key={item.asset.contractAddress}
              assetBalance={item.assetBalance}
              fiatBalance={item.fiatBalance}
              token={item.asset}
            />
          )}
          {erc271Tokens.length !== 0 &&
            <>
              <Spacer />
              <DividerText>{getLocale('braveWalletTopNavNFTS')}</DividerText>
              <SubDivider />
              {erc271Tokens.map((item) =>
                <PortfolioAssetItem
                  defaultCurrencies={defaultCurrencies}
                  action={selectAsset(item.asset)}
                  key={item.asset.contractAddress}
                  assetBalance={item.assetBalance}
                  fiatBalance={item.fiatBalance}
                  token={item.asset}
                />
              )}
            </>
          }
          <ButtonRow>
            <AddButton
              buttonType='secondary'
              onSubmit={toggleShowVisibleAssetModal}
              text={getLocale('braveWalletAccountsEditVisibleAssets')}
            />
          </ButtonRow>
        </>
      }
      {showVisibleAssetsModal &&
        <EditVisibleAssetsModal
          fullAssetList={fullAssetList}
          userVisibleTokensInfo={userVisibleTokensInfo}
          addUserAssetError={addUserAssetError}
          onClose={toggleShowVisibleAssetModal}
          onAddUserAsset={onAddUserAsset}
          onSetUserAssetVisible={onSetUserAssetVisible}
          onRemoveUserAsset={onRemoveUserAsset}
          selectedNetwork={selectedNetwork}
          onFindTokenInfoByContractAddress={onFindTokenInfoByContractAddress}
          foundTokenInfoByContractAddress={foundTokenInfoByContractAddress}
        />
      }
    </StyledWrapper>
  )
}

export default Portfolio
