import * as React from 'react'

// Constants
import {
  PriceDataObjectType,
  TransactionListInfo,
  AssetPriceInfo,
  WalletAccountType,
  AssetPriceTimeframe,
  AccountAssetOptionType,
  TokenInfo,
  EthereumChain,
  TransactionInfo,
  TransactionType
} from '../../../../constants/types'
import { getLocale } from '../../../../../common/locale'

// Utils
import { formatWithCommasAndDecimals } from '../../../../utils/format-prices'
import { formatBalance } from '../../../../utils/format-balances'

// Options
import { ChartTimelineOptions } from '../../../../options/chart-timeline-options'
import { ETH } from '../../../../options/asset-options'

// Components
import { SearchBar, BackButton } from '../../../shared'
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
  DividerRow
} from './style'

export interface Props {
  toggleNav: () => void
  onChangeTimeline: (path: AssetPriceTimeframe) => void
  onSelectAsset: (asset: TokenInfo | undefined) => void
  onClickAddAccount: () => void
  fetchFullTokenList: () => void
  onSelectNetwork: (network: EthereumChain) => void
  onAddUserAsset: (token: TokenInfo) => void
  onSetUserAssetVisible: (contractAddress: string, isVisible: boolean) => void
  onRemoveUserAsset: (contractAddress: string) => void
  addUserAssetError: boolean
  selectedNetwork: EthereumChain
  networkList: EthereumChain[]
  userAssetList: AccountAssetOptionType[]
  accounts: WalletAccountType[]
  selectedTimeline: AssetPriceTimeframe
  selectedPortfolioTimeline: AssetPriceTimeframe
  selectedAsset: TokenInfo | undefined
  selectedUSDAssetPrice: AssetPriceInfo | undefined
  selectedBTCAssetPrice: AssetPriceInfo | undefined
  selectedAssetPriceHistory: PriceDataObjectType[]
  portfolioPriceHistory: PriceDataObjectType[]
  portfolioBalance: string
  transactions: (TransactionListInfo | undefined)[]
  isLoading: boolean
  fullAssetList: TokenInfo[]
  userVisibleTokensInfo: TokenInfo[]
  isFetchingPortfolioPriceHistory: boolean
  transactionSpotPrices: AssetPriceInfo[]
}

const Portfolio = (props: Props) => {
  const {
    toggleNav,
    onChangeTimeline,
    onSelectAsset,
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
    transactionSpotPrices
  } = props

  const [filteredAssetList, setfilteredAssetList] = React.useState<AccountAssetOptionType[]>(userAssetList)
  const [hoverBalance, setHoverBalance] = React.useState<string>()
  const [hoverPrice, setHoverPrice] = React.useState<string>()
  const [showNetworkDropdown, setShowNetworkDropdown] = React.useState<boolean>(false)
  const [showVisibleAssetsModal, setShowVisibleAssetsModal] = React.useState<boolean>(false)

  const toggleShowNetworkDropdown = () => {
    setShowNetworkDropdown(!showNetworkDropdown)
  }

  React.useMemo(() => {
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

  const selectAsset = (asset: TokenInfo) => () => {
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

  const getFiatBalance = (account: WalletAccountType, asset: TokenInfo) => {
    const found = account.tokens.find((token) => token.asset.contractAddress === asset.contractAddress)
    return (found) ? found.fiatBalance : '0'
  }

  const getAssetBalance = (account: WalletAccountType, asset: TokenInfo) => {
    const found = account.tokens.find((token) => token.asset.contractAddress === asset.contractAddress)
    return (found) ? formatBalance(found.assetBalance, found.asset.decimals) : '0'
  }

  const priceHistory = React.useMemo(() => {
    if (parseFloat(portfolioBalance) === 0) {
      return []
    } else {
      return portfolioHistory
    }
  }, [portfolioHistory, portfolioBalance])

  const selectedAssetTransactions = React.useMemo((): TransactionInfo[] => {
    const list = transactions.map((account) => {
      return account?.transactions
    })
    const combinedList = [].concat.apply([], list)
    if (selectedAsset?.symbol === selectedNetwork.symbol) {
      return combinedList.filter((tx: TransactionInfo) => tx.txType === TransactionType.ETHSend || tx.txType === TransactionType.ERC20Approve)
    } else {
      return combinedList.filter((tx: TransactionInfo) => tx.txData.baseData.to.toLowerCase() === selectedAsset?.contractAddress.toLowerCase())
    }
  }, [selectedAsset, transactions])

  const findAccount = (address: string): WalletAccountType | undefined => {
    return accounts.find((account) => address.toLowerCase() === account.address.toLowerCase())
  }

  const fullAssetBalances = React.useMemo(() => {
    return filteredAssetList.find((asset) => asset.asset.contractAddress.toLowerCase() === selectedAsset?.contractAddress.toLowerCase())
  }, [filteredAssetList, selectedAsset])

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
          <BalanceText>${hoverBalance ? hoverBalance : portfolioBalance}</BalanceText>
        </>
      ) : (
        <InfoColumn>
          <AssetRow>
            <AssetIcon icon={selectedAsset.symbol === 'ETH' ? ETH.asset.logo : selectedAsset.logo} />
            <AssetNameText>{selectedAsset.name}</AssetNameText>
          </AssetRow>
          <DetailText>{selectedAsset.name} {getLocale('braveWalletPrice')} ({selectedAsset.symbol})</DetailText>
          <PriceRow>
            <PriceText>${hoverPrice ? hoverPrice : selectedUSDAssetPrice ? formatWithCommasAndDecimals(selectedUSDAssetPrice.price) : 0.00}</PriceText>
            <PercentBubble isDown={selectedUSDAssetPrice ? Number(selectedUSDAssetPrice.assetTimeframeChange) < 0 : false}>
              <ArrowIcon isDown={selectedUSDAssetPrice ? Number(selectedUSDAssetPrice.assetTimeframeChange) < 0 : false} />
              <PercentText>{selectedUSDAssetPrice ? Number(selectedUSDAssetPrice.assetTimeframeChange).toFixed(2) : 0.00}%</PercentText>
            </PercentBubble>
          </PriceRow>
          <DetailText>{selectedBTCAssetPrice ? selectedBTCAssetPrice.price : 0} BTC</DetailText>
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
            <AssetBalanceDisplay>${formatWithCommasAndDecimals(fullAssetBalances?.fiatBalance ?? '')} ({formatWithCommasAndDecimals(fullAssetBalances?.assetBalance ?? '')} {selectedAsset.symbol})</AssetBalanceDisplay>
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
                  transaction={transaction}
                  account={findAccount(transaction.fromAddress)}
                  transactionSpotPrices={transactionSpotPrices}
                  visibleTokens={userVisibleTokensInfo}
                />
              )}
            </>
          ) : (
            <EmptyTransactionContainer>
              <TransactionPlaceholderText>{getLocale('braveWalletTransactionPlaceholder')}</TransactionPlaceholderText>
            </EmptyTransactionContainer>
          )}

        </>
      }
      {!selectedAsset &&
        <>
          <SearchBar placeholder={getLocale('braveWalletSearchText')} action={filterAssets} />
          {filteredAssetList.map((item) =>
            <PortfolioAssetItem
              action={selectAsset(item.asset)}
              key={item.asset.contractAddress}
              name={item.asset.name}
              assetBalance={item.assetBalance}
              fiatBalance={item.fiatBalance}
              symbol={item.asset.symbol}
              logo={item.asset.logo}
              isVisible={item.asset.visible}
            />
          )}
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
        />
      }
    </StyledWrapper>
  )
}

export default Portfolio
