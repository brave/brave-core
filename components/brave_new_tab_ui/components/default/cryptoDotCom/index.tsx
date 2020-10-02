/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */
import * as React from 'react'

import createWidget from '../widget/index'
import { StyledTitleTab } from '../widgetTitleTab'

import {
  currencyNames,
  dynamicBuyLink,
  links
} from './data'

import {
  ActionAnchor,
  ActionButton,
  AssetIcon,
  AssetIconWrapper,
  BackArrow,
  BasicBox,
  Box,
  CryptoDotComIcon,
  FlexItem,
  Header,
  List,
  ListItem,
  PlainButton,
  StyledTitle,
  StyledTitleText,
  SVG,
  Text,
  WidgetWrapper,
  UpperCaseText
} from './style'
import CryptoDotComLogo from './assets/cryptoDotCom-logo'
import { CaratLeftIcon } from 'brave-ui/components/icons'

// Utils
import cryptoColors from '../exchangeWidget/colors'
import { getLocale } from '../../../../common/locale'

interface State {
  selectedAsset: string
}

interface TickerPrice {
  price: number
  volume: number
}

interface AssetRanking {
  lastPrice: number
  pair: string
  percentChange: number
}

interface ChartDataPoint {
  c: number
  h: number
  l: number
  o: number
  t: number
  v: number
}

interface Props {
  showContent: boolean
  optInTotal: boolean
  optInBTCPrice: boolean
  optInMarkets: boolean
  tickerPrices: Record<string, TickerPrice>
  losersGainers: Record<string, AssetRanking[]>
  supportedPairs: Record<string, string[]>
  charts: Record<string, ChartDataPoint[]>
  stackPosition: number
  onShowContent: () => void
  onDisableWidget: () => void
  onTotalPriceOptIn: () => void
  onBtcPriceOptIn: () => void
  onSetLosersGainers: () => Promise<void>
  onSetSupportedPairs: () => Promise<void>
  onSetTickerPrices: (assets: string[]) => Promise<void>
  onSetCharts: (asset: string[]) => Promise<void>
  onUpdateActions: () => Promise<void[]>
  onBuyCrypto: () => void
  onInteraction: () => void
  onOptInMarkets: () => void
}
interface ChartConfig {
  data: Array<any>
  chartHeight: number
  chartWidth: number
}

class CryptoDotCom extends React.PureComponent<Props, State> {
  private refreshInterval: any
  private topMovers: string[] = [ 'BTC', 'ETH', 'CRO' ]

  constructor (props: Props) {
    super(props)
    this.state = {
      selectedAsset: ''
    }
  }

  // This is a temporary function only necessary for MVP
  // Merges losers/gainers into one table
  transformLosersGainers = ({ losers = [], gainers = [] }: Record<string, AssetRanking[]>): Record<string, AssetRanking> => {
    const losersGainersMerged = [ ...losers, ...gainers ]
    return losersGainersMerged.reduce((mergedTable: object, asset: AssetRanking) => {
      let { pair: assetName, ...assetRanking } = asset
      assetName = assetName.split('_')[0]

      return {
        ...mergedTable,
        [assetName]: assetRanking
      }
    }, {})
  }

  componentDidMount () {
    this.checkSetRefreshInterval()
  }

  componentWillUnmount () {
    clearInterval(this.refreshInterval)
  }

  checkSetRefreshInterval = () => {
    if (!this.refreshInterval) {
      this.refreshInterval = setInterval(async () => {
        await this.props.onUpdateActions()
          .catch((_e) => console.debug('Could not update crypto.com data'))
      }, 30000)
    }
  }

  clearIntervals = () => {
    clearInterval(this.refreshInterval)
  }

  setSelectedAsset = (asset: string) => {
    this.setState({
      selectedAsset: asset
    })
  }

  handleViewMarketsClick = async () => {
    await Promise.all([
      this.props.onSetTickerPrices(this.topMovers),
      this.props.onSetLosersGainers()
    ])
    this.props.onInteraction()
    this.props.onOptInMarkets()
  }

  handleAssetDetailClick = async (asset: string) => {
    await Promise.all([
      this.props.onSetCharts([asset]),
      this.props.onSetSupportedPairs()
    ])
    this.setSelectedAsset(asset)
  }

  onClickBuyTop = () => {
    window.open(links.buyTop, '_blank', 'noopener')
    this.props.onBuyCrypto()
  }

  onClickBuyBottom = () => {
    window.open(links.buyBottom, '_blank', 'noopener')
    this.props.onBuyCrypto()
  }

  onClickBuyPair = (pair: string) => {
    window.open(dynamicBuyLink(pair), '_blank', 'noopener')
    this.props.onBuyCrypto()
  }

  formattedNum = (price: number) => {
    return new Intl.NumberFormat('en-IN', {
      style: 'currency',
      currency: 'USD',
      currencyDisplay: 'narrowSymbol'
    }).format(price)
  }

  plotData ({ data, chartHeight, chartWidth }: ChartConfig) {
    const pointsPerDay = 4
    const daysInrange = 7
    const yHighs = data.map((point: ChartDataPoint) => point.h)
    const yLows = data.map((point: ChartDataPoint) => point.l)
    const dataPoints = data.map((point: ChartDataPoint) => point.c)
    const chartAreaY = chartHeight - 2
    const max = Math.max(...yHighs)
    const min = Math.min(...yLows)
    const pixelsPerPoint = (max - min) / chartAreaY
    return dataPoints
      .map((v, i) => {
        const y = (v - min) / pixelsPerPoint
        const x = i * (chartWidth / (pointsPerDay * daysInrange))
        return `${x},${chartAreaY - y}`
      })
      .join('\n')
  }

  renderIconAsset = (key: string, isDetail: boolean = false) => {
    const iconBgColor = cryptoColors[key] || '#fff'

    return (
      <AssetIconWrapper $bg={iconBgColor} textColor='#000'>
        <AssetIcon
          isDetail={isDetail}
          className={`crypto-icon icon-${key}`}
        />
      </AssetIconWrapper>
    )
  }

  renderIndexView () {
    const { optInBTCPrice, onBtcPriceOptIn } = this.props
    const currency = 'BTC'
    const { price = null } = this.props.tickerPrices[currency] || {}

    const losersGainers = this.transformLosersGainers(this.props.losersGainers || {})
    const { percentChange = null } = losersGainers[currency] || {}
    return (
      <>
        <Box isFlex={true} $height={48}>
          <FlexItem $pl={5} $pr={5}>
            {this.renderIconAsset(currency.toLowerCase())}
          </FlexItem>
          <FlexItem>
              <Text>{currency}</Text>
              <Text small={true} textColor='light'>{currencyNames[currency]}</Text>
          </FlexItem>
          <FlexItem textAlign='right' flex={1}>
            {optInBTCPrice ? (
              <>
                {(price !== null) && <Text>{this.formattedNum(price)}</Text>}
                {(percentChange !== null) && <Text textColor={percentChange > 0 ? 'green' : 'red'}>{percentChange}%</Text>}
              </>
            ) : (
              <PlainButton onClick={onBtcPriceOptIn} textColor='green' inline={true}>
                {getLocale('cryptoDotComWidgetShowPrice')}
              </PlainButton>
            )}
          </FlexItem>
          <FlexItem $pl={5}>
            <ActionButton onClick={this.onClickBuyTop} small={true} light={true}>
              {getLocale('cryptoDotComWidgetBuy')}
            </ActionButton>
          </FlexItem>
        </Box>
        <Text center={true} $p='1em 0 0.5em' $fontSize={15}>
          {getLocale('cryptoDotComWidgetCopyOne')}
        </Text>
        <Text center={true} $fontSize={15}>
          {getLocale('cryptoDotComWidgetCopyTwo')}
        </Text>
        <ActionAnchor onClick={this.onClickBuyBottom} $m='1em 0'>
          {getLocale('cryptoDotComWidgetBuyBtc')}
        </ActionAnchor>
        <PlainButton textColor='light' onClick={this.handleViewMarketsClick} $m='0 auto'>
          {getLocale('cryptoDotComWidgetViewMarkets')}
        </PlainButton>
      </>
    )
  }

  renderTopMoversView () {
    return (
      <List>
        {this.topMovers.map(currency => {
          const { price = null } = this.props.tickerPrices[currency] || {}
          const losersGainers = this.transformLosersGainers(this.props.losersGainers || {})
          const { percentChange = null } = losersGainers[currency] || {}
          return (
            <ListItem key={currency} isFlex={true} onClick={this.handleAssetDetailClick.bind(this, currency)} $height={48}>
              <FlexItem $pl={5} $pr={5}>
                {this.renderIconAsset(currency.toLowerCase())}
              </FlexItem>
              <FlexItem>
                <Text>{currency}</Text>
                <Text small={true} textColor='light'>{currencyNames[currency]}</Text>
              </FlexItem>
              <FlexItem textAlign='right' flex={1}>
                {(price !== null) && <Text>{this.formattedNum(price)}</Text>}
                {(percentChange !== null) && <Text textColor={percentChange > 0 ? 'green' : 'red'}>{percentChange}%</Text>}
              </FlexItem>
            </ListItem>
          )
        })}
      </List>
    )
  }

  renderAssetDetailView () {
    const { selectedAsset: currency } = this.state
    const { price = null, volume = null } = this.props.tickerPrices[currency] || {}
    const chartData = this.props.charts[currency] || []
    const pairs = this.props.supportedPairs[currency] || []

    const losersGainers = this.transformLosersGainers(this.props.losersGainers || {})
    const { percentChange = null } = losersGainers[currency] || {}

    const chartHeight = 100
    const chartWidth = 309
    return (
      <Box hasPadding={false}>
        <FlexItem
          hasPadding={true}
          isFlex={true}
          isFullWidth={true}
          hasBorder={true}
        >
          <FlexItem>
            <BackArrow>
              <CaratLeftIcon onClick={this.setSelectedAsset.bind(this, '')} />
            </BackArrow>
          </FlexItem>
          <FlexItem $pr={5}>
            {this.renderIconAsset(currency.toLowerCase())}
          </FlexItem>
          <FlexItem flex={1}>
            <Text>{currency}</Text>
            <Text small={true} textColor='light'>
              {currencyNames[currency]}
            </Text>
          </FlexItem>
          <FlexItem $pl={5}>
            <ActionButton onClick={this.onClickBuyPair.bind(this, `${currency}_USDT`)} small={true} light={true}>
              <UpperCaseText>
                {getLocale('cryptoDotComWidgetBuy')}
              </UpperCaseText>
            </ActionButton>
          </FlexItem>
        </FlexItem>
        <FlexItem
          hasPadding={true}
          isFullWidth={true}
          hasBorder={true}
        >
          {(price !== null) && <Text
            inline={true}
            large={true}
            weight={500}
            $mr='0.5rem'
          >
            {this.formattedNum(price)} USDT
          </Text>}
          {(percentChange !== null) && <Text inline={true} textColor={percentChange > 0 ? 'green' : 'red'}>{percentChange}%</Text>}
          <SVG
            chartWidth={chartWidth}
            chartHeight={chartHeight}
          >
            <polyline
              fill='none'
              stroke='#44B0FF'
              strokeWidth='3'
              points={this.plotData({
                data: chartData,
                chartHeight,
                chartWidth
              })}
            />
          </SVG>
        <Text small={true} textColor='xlight'>
          {getLocale('cryptoDotComWidgetGraph')}
        </Text>
        </FlexItem>
        <FlexItem
          hasPadding={true}
          isFullWidth={true}
        >
          <BasicBox $mt='0.2em'>
            <Text small={true} textColor='light' $pb='0.2rem'>
              <UpperCaseText>
                {getLocale('cryptoDotComWidgetVolume')}
              </UpperCaseText>
            </Text>
            {volume && <Text weight={500}>{this.formattedNum(volume)} USDT</Text>}
          </BasicBox>
          <BasicBox $mt='1em'>
            <Text small={true} textColor='light' $pb='0.2rem'>
              <UpperCaseText>
                {getLocale('cryptoDotComWidgetPairs')}
              </UpperCaseText>
            </Text>
            {pairs.map((pair, i) => {
              const pairName = pair.replace('_', '/')
              return (
                <ActionButton onClick={this.onClickBuyPair.bind(this, pairName)} key={pair} small={true} inline={true} $mr={i === 0 ? 5 : 0} $mb={5}>
                  {pairName}
                </ActionButton>
              )
            })}
          </BasicBox>
        </FlexItem>
      </Box>
    )
  }

  renderTitle () {
    const { showContent } = this.props
    return (
      <Header showContent={showContent}>
        <StyledTitle>
          <CryptoDotComIcon>
            <CryptoDotComLogo />
          </CryptoDotComIcon>
          <StyledTitleText>
            {'Crypto.com'}
          </StyledTitleText>
        </StyledTitle>
      </Header>
    )
  }

  renderTitleTab () {
    const { onShowContent, stackPosition } = this.props

    return (
      <StyledTitleTab onClick={onShowContent} stackPosition={stackPosition}>
        {this.renderTitle()}
      </StyledTitleTab>
    )
  }

  renderSelectedView () {
    const { selectedAsset } = this.state
    const { optInMarkets } = this.props

    if (selectedAsset) {
      return this.renderAssetDetailView()
    }

    if (optInMarkets) {
      return this.renderTopMoversView()
    }

    return this.renderIndexView()
  }

  render () {
    const { showContent } = this.props

    if (!showContent) {
      return this.renderTitleTab()
    }

    return (
      <WidgetWrapper tabIndex={0}>
        {this.renderTitle()}
        {this.renderSelectedView()}
      </WidgetWrapper>
    )
  }
}

export const CryptoDotComWidget = createWidget(CryptoDotCom)
