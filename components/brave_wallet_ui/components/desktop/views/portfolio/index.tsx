import * as React from 'react'

// Constants
import {
  PriceDataObjectType,
  RPCTransactionType,
  AssetPriceInfo,
  WalletAccountType,
  AssetPriceTimeframe,
  AccountAssetOptionType,
  TokenInfo,
  EthereumChain
} from '../../../../constants/types'
import locale from '../../../../constants/locale'

// Utils
import { formatPrices } from '../../../../utils/format-prices'
import { formatBalance } from '../../../../utils/format-balances'

// Options
import { ChartTimelineOptions } from '../../../../options/chart-timeline-options'

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
  BalanceRow
} from './style'

export interface Props {
  toggleNav: () => void
  onChangeTimeline: (path: AssetPriceTimeframe) => void
  onSelectAsset: (asset: TokenInfo | undefined) => void
  onClickAddAccount: () => void
  onUpdateVisibleTokens: (list: string[]) => void
  fetchFullTokenList: () => void
  onSelectNetwork: (network: EthereumChain) => void
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
  transactions: (RPCTransactionType | undefined)[]
  isLoading: boolean
  fullAssetList: TokenInfo[]
  userWatchList: string[]
  isFetchingPortfolioPriceHistory: boolean
}

const Portfolio = (props: Props) => {
  const {
    toggleNav,
    onChangeTimeline,
    onSelectAsset,
    onClickAddAccount,
    onSelectNetwork,
    onUpdateVisibleTokens,
    fetchFullTokenList,
    selectedNetwork,
    fullAssetList,
    userWatchList,
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
    isFetchingPortfolioPriceHistory
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

  const moreDetails = () => {
    alert('Will Show More Details Popover!!')
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
        setHoverBalance(formatPrices(value))
      } else {
        setHoverBalance(undefined)
      }
    } else {
      if (value) {
        setHoverPrice(formatPrices(value))
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
    if (!showVisibleAssetsModal) {
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

  return (
    <StyledWrapper onClick={onHideNetworkDropdown}>
      <TopRow>
        <BalanceRow>
          {!selectedAsset ? (
            <BalanceTitle>{locale.balance}</BalanceTitle>
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
          timelineOptions={ChartTimelineOptions}
        />
      </TopRow>
      {!selectedAsset ? (
        <>
          <BalanceText>${hoverBalance ? hoverBalance : portfolioBalance}</BalanceText>
        </>
      ) : (
        <InfoColumn>
          <AssetRow>
            <AssetIcon icon={selectedAsset.icon} />
            <AssetNameText>{selectedAsset.name}</AssetNameText>
          </AssetRow>
          <DetailText>{selectedAsset.name} {locale.price} ({selectedAsset.symbol})</DetailText>
          <PriceRow>
            <PriceText>${hoverPrice ? hoverPrice : selectedUSDAssetPrice ? formatPrices(Number(selectedUSDAssetPrice.price)) : 0.00}</PriceText>
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
        priceData={selectedAsset ? selectedAssetPriceHistory : portfolioHistory}
        onUpdateBalance={onUpdateBalance}
        isLoading={selectedAsset ? isLoading : parseFloat(portfolioBalance) === 0 ? false : isFetchingPortfolioPriceHistory}
        isDisabled={selectedAsset ? false : parseFloat(portfolioBalance) === 0}
      />
      {selectedAsset &&
        <>
          <DividerText>{locale.accounts}</DividerText>
          <SubDivider />
          {accounts.map((account) =>
            <PortfolioAccountItem
              key={account.address}
              action={moreDetails}
              assetTicker={selectedAsset.symbol}
              name={account.name}
              address={account.address}
              fiatBalance={getFiatBalance(account, selectedAsset)}
              assetBalance={getAssetBalance(account, selectedAsset)}
            />
          )}
          <ButtonRow>
            <AddButton
              buttonType='secondary'
              onSubmit={onClickAddAccount}
              text={locale.addAccount}
            />
          </ButtonRow>
          <DividerText>{locale.transactions}</DividerText>
          <SubDivider />
          {transactions?.map((transaction) =>
            <PortfolioTransactionItem
              action={moreDetails}
              key={transaction?.hash}
              amount={transaction?.amount ? transaction.amount : 0}
              from={transaction?.from ? transaction.from : ''}
              to={transaction?.to ? transaction.to : ''}
              ticker={selectedAsset.symbol}
            />
          )}
        </>
      }
      {!selectedAsset &&
        <>
          <SearchBar placeholder={locale.searchText} action={filterAssets} />
          {filteredAssetList.map((item) =>
            <PortfolioAssetItem
              action={selectAsset(item.asset)}
              key={item.asset.contractAddress}
              name={item.asset.name}
              assetBalance={item.assetBalance}
              fiatBalance={item.fiatBalance}
              symbol={item.asset.symbol}
              icon={item.asset.icon}
            />
          )}
          <ButtonRow>
            <AddButton
              buttonType='secondary'
              onSubmit={toggleShowVisibleAssetModal}
              text={locale.accountsEditVisibleAssets}
            />
          </ButtonRow>
        </>
      }
      {showVisibleAssetsModal &&
        <EditVisibleAssetsModal
          userAssetList={userAssetList}
          onUpdateVisibleTokens={onUpdateVisibleTokens}
          fullAssetList={fullAssetList}
          userWatchList={userWatchList}
          onClose={toggleShowVisibleAssetModal}
        />
      }
    </StyledWrapper>
  )
}

export default Portfolio
