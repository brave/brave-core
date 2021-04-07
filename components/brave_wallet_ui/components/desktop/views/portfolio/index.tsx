import * as React from 'react'
import { ChartTimelineType, PriceDataObjectType, RPCAssetType } from '../../../../constants/types'
import locale from '../../../../constants/locale'
import { formatePrices } from '../../../../utils/format-prices'
import { ChartTimelineOptions } from '../../../../options/chart-timeline-options'
import { AssetOptions } from '../../../../options/asset-options'
import { PriceHistoryMockData } from '../../../../stories/mock-data/price-history-data'
import { mockRPCResponse } from '../../../../stories/mock-data/rpc-response'
import { CurrentPriceMockData } from '../../../../stories/mock-data/current-price-data'
import { ChartControlBar, LineChart, PortfolioAssetItem, AddButton } from '../../'
import { SearchBar } from '../../../shared'
import {
  StyledWrapper,
  TopRow,
  BalanceTitle,
  BalanceText,
  ButtonRow
} from './style'

const Portfolio = () => {
  const [selectedTimeline, setSelectedTimeline] = React.useState<ChartTimelineType>('24HRS')
  const [priceData, setPriceData] = React.useState<PriceDataObjectType[]>(PriceHistoryMockData.slice(15, 20))
  const [assetList, setAssetList] = React.useState<RPCAssetType[]>(mockRPCResponse[0].assets)

  // This will change once we hit a real api for pricing
  const historyAmount = (path: ChartTimelineType) => {
    switch (path) {
      case '5MIN':
        return 17
      case '24HRS':
        return 15
      case '7Day':
        return 12
      case '1Month':
        return 10
      case '3Months':
        return 8
      case '1Year':
        return 4
      case 'AllTime':
        return 0
    }
  }

  const changeTimeline = (path: ChartTimelineType) => {
    setPriceData(PriceHistoryMockData.slice(historyAmount(path), 20))
    setSelectedTimeline(path)
  }

  const findFiatValue = (asset: RPCAssetType) => {
    const data = CurrentPriceMockData.find((coin) => coin.symbol === asset.symbol)
    const value = data ? data.price * asset.balance : 0
    return formatePrices(value)
  }

  const findIcon = (asset: RPCAssetType) => {
    const option = AssetOptions.find((coin) => coin.symbol === asset.symbol)
    return option ? option.icon : ''
  }

  const findAssetAmount = (id: string) => {
    const holdings = mockRPCResponse[0].assets
    return holdings.find((coin) => {
      return coin.symbol === id
    })
  }

  const getFullBalance = () => {
    const prices = CurrentPriceMockData
    const amountList = prices.map((coin) => {
      const amount = findAssetAmount(coin.symbol)?.balance
      const holdingAmount = amount ? amount : 0
      return coin.price * holdingAmount
    })
    const grandTotal = amountList.reduce(function (a, b) {
      return a + b
    }, 0)
    return formatePrices(grandTotal)
  }

  const filterAssets = (event: any) => {
    const search = event.target.value
    if (search === '') {
      setAssetList(mockRPCResponse[0].assets)
    } else {
      const filteredList = mockRPCResponse[0].assets.filter((asset) => {
        return (
          asset.name.toLowerCase() === search.toLowerCase() ||
          asset.name.toLowerCase().startsWith(search.toLowerCase()) ||
          asset.symbol.toLocaleLowerCase() === search.toLowerCase() ||
          asset.symbol.toLowerCase().startsWith(search.toLowerCase())
        )
      })
      setAssetList(filteredList)
    }
  }

  const expandTo = (asset: string) => () => {
    alert(`Will Expand to ${asset} view`)
  }

  const addCoin = () => {
    alert('Will Show New Coins To Add!!')
  }

  return (
    <StyledWrapper>
      <TopRow>
        <BalanceTitle>{locale.balance}</BalanceTitle>
        <ChartControlBar
          onSubmit={changeTimeline}
          selectedTimeline={selectedTimeline}
          timelineOptions={ChartTimelineOptions}
        />
      </TopRow>
      <BalanceText>${getFullBalance()}</BalanceText>
      <LineChart priceData={priceData} />
      <SearchBar placeholder={locale.searchText} action={filterAssets} />
      {assetList.map((asset) =>
        <PortfolioAssetItem
          action={expandTo(asset.name)}
          key={asset.id}
          name={asset.name}
          assetBalance={asset.balance.toString()}
          fiatBalance={findFiatValue(asset)}
          symbol={asset.symbol}
          icon={findIcon(asset)}
        />
      )}
      <ButtonRow>
        <AddButton
          buttonType='secondary'
          onSubmit={addCoin}
          text={locale.addCoin}
        />
      </ButtonRow>
    </StyledWrapper>
  )
}

export default Portfolio
