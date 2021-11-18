import * as React from 'react'

// Constants
import {
  PriceDataObjectType,
  AccountTransactions,
  AssetPrice,
  WalletAccountType,
  AssetPriceTimeframe,
  AccountAssetOptionType,
  ERCToken,
  EthereumChain,
  TransactionInfo
} from '../../../../constants/types'
import { getLocale } from '../../../../../common/locale'

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
  onChangeTimeline: (path: AssetPriceTimeframe) => void
  onSelectAsset: (asset: ERCToken | undefined) => void
  onSelectAccount: (account: WalletAccountType) => void
  onClickAddAccount: () => void
  fetchFullTokenList: () => void
  onSelectNetwork: (network: EthereumChain) => void
  onAddUserAsset: (token: ERCToken) => void
  onSetUserAssetVisible: (token: ERCToken, isVisible: boolean) => void
  onRemoveUserAsset: (token: ERCToken) => void
  addUserAssetError: boolean
  selectedNetwork: EthereumChain
  networkList: EthereumChain[]
  userAssetList: AccountAssetOptionType[]
  accounts: WalletAccountType[]
  selectedTimeline: AssetPriceTimeframe
  selectedPortfolioTimeline: AssetPriceTimeframe
  selectedAsset: ERCToken | undefined
  selectedUSDAssetPrice: AssetPrice | undefined
  selectedBTCAssetPrice: AssetPrice | undefined
  selectedAssetPriceHistory: PriceDataObjectType[]
  portfolioPriceHistory: PriceDataObjectType[]
  portfolioBalance: string
  transactions: AccountTransactions
  isLoading: boolean
  fullAssetList: ERCToken[]
  userVisibleTokensInfo: ERCToken[]
  isFetchingPortfolioPriceHistory: boolean
  transactionSpotPrices: AssetPrice[]
  onRetryTransaction: (transaction: TransactionInfo) => void
  onSpeedupTransaction: (transaction: TransactionInfo) => void
  onCancelTransaction: (transaction: TransactionInfo) => void
}

const Portfolio = (props: Props) => {
  const {
    toggleNav,
    onChangeTimeline,
    onSelectAsset,
    onSelectAccount,
    onClickAddAccount,
    onSelectNetwork,
    fetchFullTokenList,
    onAddUserAsset,
    onSetUserAssetVisible,
    onRemoveUserAsset,
    addUserAssetError,
    userVisibleTokensInfo,
    selectedNetwork,
    fullAssetList,
    portfolioPriceHistory,
    selectedAssetPriceHistory,
    selectedUSDAssetPrice,
    selectedBTCAssetPrice,
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
    onCancelTransaction
  } = props

  const [filteredAssetList, setfilteredAssetList] = React.useState<AccountAssetOptionType[]>(userAssetList)
  const [fullPortfolioFiatBalance, setFullPortfolioFiatBalance] = React.useState<string>(portfolioBalance)
  const [hoverBalance, setHoverBalance] = React.useState<string>()
  const [hoverPrice, setHoverPrice] = React.useState<string>()
  const [showNetworkDropdown, setShowNetworkDropdown] = React.useState<boolean>(false)
  const [showVisibleAssetsModal, setShowVisibleAssetsModal] = React.useState<boolean>(false)
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

  const selectAsset = (asset: ERCToken) => () => {
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

  const onClickSelectNetwork = (network: EthereumChain) => () => {
    onSelectNetwork(network)
    toggleShowNetworkDropdown()
  }

  const onHideNetworkDropdown = () => {
    if (showNetworkDropdown) {
      setShowNetworkDropdown(false)
    }
  }

  const toggleShowVisibleAssetModal = () => {
    if (fullAssetList.length === 0) {
      fetchFullTokenList()
    }
    setShowVisibleAssetsModal(!showVisibleAssetsModal)
  }

  const getFiatBalance = (account: WalletAccountType, asset: ERCToken) => {
    const found = account.tokens.find((token) => token.asset.contractAddress === asset.contractAddress)
    return (found) ? found.fiatBalance : ''
  }

  const getAssetBalance = (account: WalletAccountType, asset: ERCToken) => {
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

  const selectedAssetTransactions = React.useMemo((): TransactionInfo[] => {
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
          <BalanceText>{fullPortfolioFiatBalance !== '' ? '$' : ''}{hoverBalance || fullPortfolioFiatBalance}</BalanceText>
        </>
      ) : (
        <InfoColumn>
          <AssetRow>
            <AssetIconWithPlaceholder selectedAsset={selectedAsset} />
            <AssetNameText>{selectedAsset.name}</AssetNameText>
          </AssetRow>
          <DetailText>{selectedAsset.name} {getLocale('braveWalletPrice')} ({selectedAsset.symbol})</DetailText>
          <PriceRow>
            <PriceText>${hoverPrice || (selectedUSDAssetPrice ? formatWithCommasAndDecimals(selectedUSDAssetPrice.price) : 0.00)}</PriceText>
            <PercentBubble isDown={selectedUSDAssetPrice ? Number(selectedUSDAssetPrice.assetTimeframeChange) < 0 : false}>
              <ArrowIcon isDown={selectedUSDAssetPrice ? Number(selectedUSDAssetPrice.assetTimeframeChange) < 0 : false} />
              <PercentText>{selectedUSDAssetPrice ? Number(selectedUSDAssetPrice.assetTimeframeChange).toFixed(2) : 0.00}%</PercentText>
            </PercentBubble>
          </PriceRow>
          <DetailText>{selectedBTCAssetPrice ? selectedBTCAssetPrice.price : ''} BTC</DetailText>
        </InfoColumn>
      )}
      <LineChart
        isDown={selectedAsset && selectedUSDAssetPrice ? Number(selectedUSDAssetPrice.assetTimeframeChange) < 0 : false}
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
            <AssetBalanceDisplay>{formatFiatAmountWithCommasAndDecimals(fullAssetBalances?.fiatBalance ?? '')} {formatedFullAssetBalance}</AssetBalanceDisplay>
          </DividerRow>
          <SubDivider />
          {accounts.map((account) =>
            <PortfolioAccountItem
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
              onSubmit={onClickAddAccount}
              text={getLocale('braveWalletAddAccount')}
            />
          </ButtonRow>
          <DividerText>{getLocale('braveWalletTransactions')}</DividerText>
          <SubDivider />
          {selectedAssetTransactions.length !== 0 ? (
            <>
              {selectedAssetTransactions.map((transaction: TransactionInfo) =>
                <PortfolioTransactionItem
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
        />
      }
    </StyledWrapper>
  )
}

export default Portfolio
