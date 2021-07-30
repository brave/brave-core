import * as React from 'react'

// Constants
import {
  PriceDataObjectType,
  AssetOptionType,
  RPCTransactionType,
  AssetPriceInfo,
  UserAssetOptionType,
  WalletAccountType,
  AssetPriceTimeframe
} from '../../../../constants/types'
import locale from '../../../../constants/locale'

// Utils
import { formatePrices } from '../../../../utils/format-prices'

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
  PortfolioTransactionItem
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
  ArrowIcon
} from './style'

export interface Props {
  toggleNav: () => void
  onChangeTimeline: (path: AssetPriceTimeframe) => void
  onSelectAsset: (asset: AssetOptionType | undefined) => void
  onClickAddAccount: () => void
  userAssetList: UserAssetOptionType[]
  accounts: WalletAccountType[]
  selectedTimeline: AssetPriceTimeframe
  selectedAsset: AssetOptionType | undefined
  selectedUSDAssetPrice: AssetPriceInfo | undefined
  selectedBTCAssetPrice: AssetPriceInfo | undefined
  selectedAssetPriceHistory: PriceDataObjectType[]
  portfolioPriceHistory: PriceDataObjectType[]
  portfolioBalance: string
  transactions: (RPCTransactionType | undefined)[]
  isLoading: boolean
}

const Portfolio = (props: Props) => {
  const {
    toggleNav,
    onChangeTimeline,
    onSelectAsset,
    onClickAddAccount,
    portfolioPriceHistory,
    selectedAssetPriceHistory,
    selectedUSDAssetPrice,
    selectedBTCAssetPrice,
    selectedTimeline,
    accounts,
    selectedAsset,
    portfolioBalance,
    transactions,
    userAssetList,
    isLoading
  } = props

  const [filteredAssetList, setfilteredAssetList] = React.useState<UserAssetOptionType[]>(userAssetList)
  const [hoverBalance, setHoverBalance] = React.useState<string>()
  const [hoverPrice, setHoverPrice] = React.useState<string>()

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

  const addCoin = () => {
    alert('Will Show New Coins To Add!!')
  }

  const moreDetails = () => {
    alert('Will Show More Details Popover!!')
  }

  const selectAsset = (asset: AssetOptionType) => () => {
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
        setHoverBalance(formatePrices(value))
      } else {
        setHoverBalance(undefined)
      }
    } else {
      if (value) {
        setHoverPrice(formatePrices(value))
      } else {
        setHoverPrice(undefined)
      }
    }
  }

  return (
    <StyledWrapper>
      <TopRow>
        {!selectedAsset ? (
          <BalanceTitle>{locale.balance}</BalanceTitle>
        ) : (
          <BackButton onSubmit={goBack} />
        )}
        <ChartControlBar
          onSubmit={onChangeTimeline}
          selectedTimeline={selectedTimeline}
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
            <PriceText>${hoverPrice ? hoverPrice : selectedUSDAssetPrice ? formatePrices(Number(selectedUSDAssetPrice.price)) : 0.00}</PriceText>
            <PercentBubble isDown={selectedUSDAssetPrice ? Number(selectedUSDAssetPrice.asset24hChange) < 0 : false}>
              <ArrowIcon isDown={selectedUSDAssetPrice ? Number(selectedUSDAssetPrice.asset24hChange) < 0 : false} />
              <PercentText>{selectedUSDAssetPrice ? Number(selectedUSDAssetPrice.asset24hChange).toFixed(2) : 0.00}%</PercentText>
            </PercentBubble>
          </PriceRow>
          <DetailText>{selectedBTCAssetPrice ? selectedBTCAssetPrice.price : 0} BTC</DetailText>
        </InfoColumn>
      )}
      <LineChart
        isDown={selectedAsset && selectedUSDAssetPrice ? Number(selectedUSDAssetPrice.asset24hChange) < 0 : false}
        isAsset={!!selectedAsset}
        priceData={selectedAsset ? selectedAssetPriceHistory : portfolioPriceHistory}
        onUpdateBalance={onUpdateBalance}
        isLoading={isLoading}
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
              fiatBalance={account.fiatBalance}
              assetBalance={account.balance}
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
              key={item.asset.id}
              name={item.asset.name}
              assetBalance={item.assetBalance}
              fiatBalance={formatePrices(item.fiatBalance)}
              symbol={item.asset.symbol}
              icon={item.asset.icon}
            />
          )}
          <ButtonRow>
            <AddButton
              buttonType='secondary'
              onSubmit={addCoin}
              text={locale.addCoin}
            />
          </ButtonRow>
        </>
      }
    </StyledWrapper>
  )
}

export default Portfolio
