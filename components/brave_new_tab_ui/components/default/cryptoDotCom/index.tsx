/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */
import * as React from 'react'
const clipboardCopy = require('clipboard-copy')
import { ThemeProvider } from 'styled-components'

import createWidget from '../widget/index'
import { StyledTitleTab } from '../widgetTitleTab'

const fakeQRImage = "data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAALkAAAC5CAAAAABRxsGAAAABxklEQVR42u3bMZaDMAwFQO5/abbcJjj/x+zmGQ9VEgiMGyHJ9nGuehzk5OTk5OTk5OTk5OTkT5Ef74/f665v/eLsq/+mTyMnJz8rUTWQYDSVhZyc/AwiyjhcjP/RP42cnDy+1+XZy6/jcZGTk/91bJmJPOTk5Hfm5+NwEVz3tcqCnHx5edXjmv70te4cOfl68niq5v1D+ub1v8xwkZOvLA/q4eoNfhlW+nyfnJy8a0xVN6hK66n8nJx8B3k6hnEGPv2VnJz8DNpWafKdZgmB94MKmpz84fJKlF6cLl8c5xDk5ORxyl0VwP1iDXJy8mQmN6h9q0iRZvTzc9Dk5I+UB6MZN6WrRteHewjIybeVVysi0tHMbB8gJyfvpl/7qrqKWvNRkZz8ufJUmW7ISfnpxeTke8vTRLureMM0PB4rOfnW8qC7nGbbaao/v2+OnHwbedX3qhb0z3TFyMnJu31z1W/jt3+X0ZOTby1PI0owwpnYQk5OHtvGHbC+bq4ydXJy8k5edaGrLQA3zUGTk5OnkSde5TTT4yIn30uetqer2d0gN0gY5ORby6s5nLjsve+m5OTbypc4yMnJycnJycnJycnJyVc8fgAH08A/VsAA/QAAAABJRU5ErkJggg=="

import {
  currencyNames,
  // dynamicBuyLink,
  links
} from './data'

import {
  SearchIcon,
  ShowIcon,
  HideIcon
} from '../../default/exchangeWidget/shared-assets'

import QRIcon from './assets/icons/qr-code.png'

import {
  ActionButton,
  AmountInputField,
  BackArrow,
  Balance,
  BasicBox,
  BlurIcon,
  Box,
  CryptoDotComIcon,
  FlexItem,
  ButtonGroup,
  Header,
  InputField,
  Link,
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
import icons from './assets/icons'

// Utils
import { getLocale } from '../../../../common/locale'

interface TickerPrice {
  price: number
  volume: number
}

interface AssetRanking {
  lastPrice: number
  pair: string
  percentChange: string
}

interface ChartDataPoint {
  c: number
  h: number
  l: number
  o: number
  t: number
  v: number
}

enum MainViews {
  TOP,
  TRADE,
  EVENTS,
  BALANCE
}

enum AssetViews {
  DETAILS,
  TRADE,
  DEPOSIT
}

interface State {
  currentView: MainViews
  currentAssetView: AssetViews
  selectedBase: string
  selectedQuote: string
}

interface Props {
  showContent: boolean
  optInTotal: boolean
  optInBTCPrice: boolean
  optInMarkets: boolean
  tickerPrices: Record<string, TickerPrice>
  losersGainers: Record<string, AssetRanking[]>
  supportedPairs: Record<string, string[]>
  tradingPairs: Array<Record<string, string>>
  newsEvents: Array<Record<string, string>>
  charts: Record<string, ChartDataPoint[]>
  stackPosition: number
  onShowContent: () => void
  onDisableWidget: () => void
  onBtcPriceOptIn: () => Promise<void>
  onUpdateActions: () => Promise<void>
  onViewMarketsRequested: (assets: string[]) => void
  onAssetsDetailsRequested: (assets: string[]) => void
  onBuyCrypto: () => void
  onOptInMarkets: (show: boolean) => void
}
interface ChartConfig {
  data: Array<any>
  chartHeight: number
  chartWidth: number
}

class CryptoDotCom extends React.PureComponent<Props, State> {
  private refreshInterval: any

  constructor (props: Props) {
    super(props)
    this.state = {
      currentView: MainViews.TOP,
      currentAssetView: AssetViews.DETAILS,
      selectedBase: '',
      selectedQuote: ''
    }
  }

  componentDidMount () {
    const { optInBTCPrice, optInMarkets } = this.props

    if (optInBTCPrice || optInMarkets) {
      this.checkSetRefreshInterval()
    }
  }

  componentWillUnmount () {
    this.clearIntervals()
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
    this.refreshInterval = null
  }

  setMainView = (view: MainViews) => {
    this.setState({
      currentView: view
    })
  }

  clearAsset = () => {
    this.setState({
      selectedBase: '',
      selectedQuote: ''
    })
  }

  handleTradeClick = () => {
    const markets = this.props.tradingPairs.map(pair => pair.pair)
    this.props.onViewMarketsRequested(markets)
  }

  optInMarkets = (show: boolean) => {
    if (show) {
      this.checkSetRefreshInterval()
    } else {
      if (!this.props.optInBTCPrice) {
        this.clearIntervals()
      }
      this.setState({ selectedBase: '' })
    }

    this.props.onOptInMarkets(show)
  }

  btcPriceOptIn = () => {
    this.props.onBtcPriceOptIn()
    this.checkSetRefreshInterval()
  }

  handleAssetClick = async (base: string, quote: string, view: AssetViews) => {
    this.setState({
      selectedBase: base,
      selectedQuote: quote || '',
      currentAssetView: view || AssetViews.DETAILS
    })
    await this.props.onAssetsDetailsRequested([base])
  }

  onClickBuyTopDetail = () => {
    window.open(links.buyTopDetail, '_blank', 'noopener')
    this.props.onBuyCrypto()
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
    // TODO: delete code below?
    // window.open(dynamicBuyLink(pair), '_blank', 'noopener')
    // this.props.onBuyCrypto()

    this.setState({
      currentAssetView: AssetViews.TRADE
    })
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

  renderPreOptIn () {
    const { optInBTCPrice } = this.props
    const currency = 'BTC'
    const { price = null } = this.props.tickerPrices[`${currency}_USDT`] || {}

    const losersGainers = transformLosersGainers(this.props.losersGainers || {})
    const { percentChange = null } = losersGainers[currency] || {}
    return (
      <>
        <Box isFlex={true} $height={48}>
          <FlexItem $pr={5}>
            {renderIconAsset(currency.toLowerCase())}
          </FlexItem>
          <FlexItem>
              <Text>{currency}</Text>
              <Text small={true} textColor='light'>{currencyNames[currency]}</Text>
          </FlexItem>
          <FlexItem textAlign='right' flex={1}>
            {optInBTCPrice ? (
              <>
                {(price !== null) && <Text>{formattedNum(price)}</Text>}
                {(percentChange !== null) && <Text textColor={getPercentColor(percentChange)}>{percentChange}%</Text>}
              </>
            ) : (
              <PlainButton onClick={this.btcPriceOptIn} textColor='green' inline={true}>
                {getLocale('cryptoDotComWidgetShowPrice')}
              </PlainButton>
            )}
          </FlexItem>
          <FlexItem $pl={5}>
            <ActionButton onClick={this.onClickBuyTop} small={true} light={true}>
              {getLocale('cryptoDotComWidgetTrade')}
            </ActionButton>
          </FlexItem>
        </Box>
        <Text $pt='1em' $fontSize={14}>
          {getLocale('cryptoDotComWidgetCopyOne')}
        </Text>
        <ActionButton onClick={this.handleTradeClick} $mt={10} $mb={15}>
          {getLocale('cryptoDotComWidgetTradeBtc')}
        </ActionButton>
        <PlainButton textColor='light' $m='0 auto'>
          {getLocale('cryptoDotComWidgetConnect')}
        </PlainButton>
      </>
    )
  }

  renderAssetDetail () {
    const { selectedBase: currency } = this.state
    const { price = null, volume = null } = this.props.tickerPrices[`${currency}_USDT`] || {}
    const chartData = this.props.charts[currency] || []
    const pairs = this.props.supportedPairs[currency] || []

    const losersGainers = transformLosersGainers(this.props.losersGainers || {})
    const { percentChange = null } = losersGainers[currency] || {}

    const chartHeight = 100
    const chartWidth = 309
    return (
      <Box $p={0}>
        <FlexItem
          hasPadding={true}
          isFlex={true}
          isFullWidth={true}
          hasBorder={true}
        >
          <FlexItem>
            <BackArrow onClick={this.clearAsset}>
              <CaratLeftIcon />
            </BackArrow>
          </FlexItem>
          <FlexItem $pr={5}>
            {renderIconAsset(currency.toLowerCase())}
          </FlexItem>
          <FlexItem flex={1}>
            <Text>{currency}</Text>
            <Text small={true} textColor='light'>
              {currencyNames[currency]}
            </Text>
          </FlexItem>
          <FlexItem $pl={5}>
            <ActionButton onClick={this.handleAssetClick.bind(this, currency, null, AssetViews.DEPOSIT)} small={true} light={true}>
              <UpperCaseText>
              {getLocale('cryptoDotComWidgetDeposit')}
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
            {formattedNum(price)} USDT
          </Text>}
          {(percentChange !== null) && <Text inline={true} textColor={getPercentColor(percentChange)}>{percentChange}%</Text>}
          <SVG viewBox={`0 0 ${chartWidth} ${chartHeight}`}>
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
            {volume && <Text weight={500}>{formattedNum(volume)} USDT</Text>}
          </BasicBox>
          <BasicBox $mt='1em'>
            <Text small={true} textColor='light' $pb='0.2rem'>
              <UpperCaseText>
                {getLocale('cryptoDotComWidgetPairs')}
              </UpperCaseText>
            </Text>
            {pairs.map((pair, i) => {
              const [base, quote] = pair.split('_')
              const pairName = pair.replace('_', '/')
              return (
                <ActionButton onClick={this.handleAssetClick.bind(this, base, quote, AssetViews.TRADE)} key={pair} small={true} inline={true} $mr={i === 0 ? 5 : 0} $mb={5}>
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
    const { selectedBase } = this.state
    const { optInMarkets, showContent } = this.props
    const shouldShowBackArrow = !selectedBase && showContent && optInMarkets

    return (
      <Header showContent={showContent}>
        <StyledTitle>
          <CryptoDotComIcon>
            <CryptoDotComLogo />
          </CryptoDotComIcon>
          <StyledTitleText>
            {'Crypto.com'}
          </StyledTitleText>
          {shouldShowBackArrow &&
            <BackArrow marketView={true} onClick={this.optInMarkets.bind(this, false)}>
              <CaratLeftIcon />
            </BackArrow>
          }
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

  renderNav () {
    const { currentView } = this.state
    return (
      <BasicBox isFlex={true} justify="start" $mb={13.5}>
        <PlainButton $pl="0" weight={500} textColor={currentView === MainViews.TOP ? 'white' : 'light'} onClick={this.setMainView.bind(null, MainViews.TOP)}>{getLocale('cryptoDotComWidgetTop')}</PlainButton>
        <PlainButton weight={500} textColor={currentView === MainViews.TRADE ? 'white' : 'light'} onClick={this.setMainView.bind(null, MainViews.TRADE)}>{getLocale('cryptoDotComWidgetTrade')}</PlainButton>
        <PlainButton weight={500} textColor={currentView === MainViews.EVENTS ? 'white' : 'light'} onClick={this.setMainView.bind(null, MainViews.EVENTS)}>{getLocale('cryptoDotComWidgetEvents')}</PlainButton>
        <PlainButton weight={500} textColor={currentView === MainViews.BALANCE ? 'white' : 'light'} onClick={this.setMainView.bind(null, MainViews.BALANCE)}>{getLocale('cryptoDotComWidgetBalance')}</PlainButton>
      </BasicBox>
    )
  }

  renderCurrentView () {
    const { currentView } = this.state
    if (currentView === MainViews.TOP) {
      return <TopMovers
        losersGainers={this.props.losersGainers}
        handleAssetClick={this.handleAssetClick}
      />
    }

    if (currentView === MainViews.TRADE) {
      return <Trade
        tickerPrices={this.props.tickerPrices}
        losersGainers={this.props.losersGainers}
        tradingPairs={this.props.tradingPairs}
        handleAssetClick={this.handleAssetClick}
      />
    }

    if (currentView === MainViews.BALANCE) {
      return <BalanceSummary />
    }

    if (currentView === MainViews.EVENTS) {
      return <Events newsEvents={this.props.newsEvents} />
    }

    return null;
  }

  renderAssetView () {
    const { currentAssetView } = this.state
    
    if (currentAssetView === AssetViews.DETAILS) {
      return this.renderAssetDetail()
    }

    if (currentAssetView === AssetViews.TRADE) {
      return <AssetTrade
        tickerPrices={this.props.tickerPrices}
        availableBalance={10.3671}
        base={this.state.selectedBase}
        quote={this.state.selectedQuote}
        handleBackClick={this.clearAsset}
      />
    }

    if (currentAssetView === AssetViews.DEPOSIT) {
      return <AssetDeposit
        assetAddress={'38pQXo6P9ycSLsPhUViFLdi2UHspwdcUCT'}
        assetQR={fakeQRImage}
        base={this.state.selectedBase}
        handleBackClick={this.clearAsset}
      />
    }

    return null
  }

  renderIndex () {
    const { selectedBase } = this.state

    if (selectedBase) {
      return this.renderAssetView()
    } else {
      return <>
        {this.renderNav()}
        {this.renderCurrentView()}
      </>
    }
  }

  render () {
    const { showContent, optInMarkets } = this.props

    if (!showContent) {
      return this.renderTitleTab()
    }

    return (
      <ThemeProvider theme={{
          secondary: 'rgba(15, 28, 45, 0.7)',
          primary: '#44B0FF',
          danger: 'rgba(234, 78, 92, 1)'
        }}>
        <WidgetWrapper>
          {this.renderTitle()}
          {(optInMarkets) ? (
            this.renderIndex()
          ) : (
            this.renderPreOptIn()
          )}
        </WidgetWrapper>
      </ThemeProvider>
    )
  }
}

export const CryptoDotComWidget = createWidget(CryptoDotCom)

// Supporting Components

function AssetDeposit ({
  assetAddress,
  assetQR, 
  base,
  handleBackClick
}: any) {
  const [showQR, setShowQR] = React.useState(false)

  return showQR
  ? (
    <Box isFlex={true} column={true} $p={10}>
      <img src={assetQR} />
      <ActionButton onClick={() => setShowQR(false)} $mt={10} small={true} light={true} isFullWidth={false}>
        Done
      </ActionButton>
    </Box>
  ) : (
    <Box $p={0}>
      <FlexItem
        hasPadding={true}
        isFlex={true}
        isFullWidth={true}
        hasBorder={true}
      >
        <FlexItem>
          <BackArrow onClick={handleBackClick}>
            <CaratLeftIcon />
          </BackArrow>
        </FlexItem>
        <FlexItem $pr={5}>
          {renderIconAsset(base.toLowerCase())}
        </FlexItem>
        <FlexItem flex={1}>
          <Text>{base}</Text>
          <Text small={true} textColor='light'>
            {currencyNames[base]}
          </Text>
        </FlexItem>
        <FlexItem $pl={5}>
          <PlainButton onClick={() => setShowQR(true)}>
            <img width={25} src={QRIcon} />
          </PlainButton>
        </FlexItem>
      </FlexItem>
      <FlexItem
        $p='0.5em'
        isFullWidth={true}
      >
        <Text $fontSize={13} weight={600}>{base} Address</Text>
        <Text $fontSize={13} breakWord={true}>{assetAddress}</Text>
        <ActionButton onClick={() => copyToClipboard(assetAddress)} $mt={5} $mb={15} small={true} light={true} isFullWidth={false}>
          {getLocale('cryptoDotComWidgetCopyAddress')}
        </ActionButton>
        <Text $fontSize={13} weight={600}>{getLocale('cryptoDotComWidgetSendCaveatHeading', { currency: base })}</Text>
        <Text $fontSize={13}>{getLocale('cryptoDotComWidgetSendCaveatBody', { currency: base })}</Text>
      </FlexItem>
    </Box>
  )
}

function AssetTrade ({
  base,
  quote,
  availableBalance,
  handleBackClick,
  tickerPrices
}: Record<string, any>) {

  enum TradeModes {
    BUY = 'buy',
    SELL = 'sell'
  }

  const tradeModeLocaleStrings = {
    buy: getLocale('cryptoDotComWidgetBuy'),
    sell: getLocale('cryptoDotComWidgetSell')
  }

  enum Percentages {
    twentyFive = 25,
    fifty = 50,
    seventyFive = 75,
    onehundred = 100
  }

  const confirmDelay = 15

  const [tradeMode, setTradeMode] = React.useState(TradeModes.BUY)
  const [tradePercentage, setTradePercentage] = React.useState<number | null>(null)
  const [tradeAmount, setTradeAmount] = React.useState('')
  const [showConfirmScreen, setConfirmScreen] = React.useState(false)
  const [counter, setCounter] = React.useState(confirmDelay)

  let intervalId: any;
  let timerId: any;
  const { price: unitPrice = null } = tickerPrices[`${base}_${quote}`] || {}
  const approxTotal = Number(tradeAmount) * unitPrice

  const handlePercentageClick = (percentage: number) => {
    const amount = (percentage / 100) * availableBalance
    setTradeAmount(`${amount}`)
    setTradePercentage(percentage)
  }

  const handleAmountChange = ({ target }: any) => {
    const { value } = target
    if (value === "." || !Number.isNaN(value * 1)) {
      setTradeAmount(value)
      setTradePercentage(null)
    }
  }

  const getPlaceholderText = () => tradeMode === TradeModes.BUY ? (
    getLocale('cryptoDotComWidgetTradeTo', {
      fromCurrency: quote,
      toCurrency: base
    })
  ) : (
    getLocale('cryptoDotComWidgetTradeTo', {
      fromCurrency: base,
      toCurrency: quote
    })
  )

  React.useEffect(() => {
    if (showConfirmScreen) {
      timerId = counter > 0 && setInterval(() => {
        if (counter > 0) {
          console.log(counter)
          setCounter(counter - 1)
        }
      }, 1000);
    }
    return () => clearInterval(timerId);
  }, [counter, showConfirmScreen]);

  const clearTimers = () => {
    clearInterval(intervalId)
    clearTimeout(timerId)
    setCounter(confirmDelay)
  }

  const handlePurchaseClick = () => {
    setConfirmScreen(true)
  }

  const handleConfirmClick = () => {
    clearTimers()
  }

  const handleCancelClick = () => {
    clearTimers()
    setConfirmScreen(false)
  }

  const buyingString = getLocale('cryptoDotComWidgetBuying')
  const sellingString = getLocale('cryptoDotComWidgetSelling')
  return showConfirmScreen ? (
    <>
      <Box>
        <Text center={true} weight={600} $pb={15}>{getLocale('cryptoDotComWidgetConfirmOrder')}</Text>
        <BasicBox $pb={7}>
          <Text weight={600} textColor='light' $fontSize={12}>{tradeMode === TradeModes.BUY ? buyingString : sellingString}</Text>
          <Text $fontSize={16}>{tradeAmount} {base}</Text>
        </BasicBox>
        <BasicBox $pb={7}>
          <Text weight={600} textColor='light' $fontSize={12}>*{getLocale('cryptoDotComWidgetApproxPrice')}</Text>
          <Text $fontSize={16}>{quote === 'USDT' ? formattedNum(unitPrice) : unitPrice} {base}/{quote}</Text>
        </BasicBox>
        <BasicBox $pb={7}>
          {tradeMode === TradeModes.BUY ? (
            <Text weight={600} textColor='light' $fontSize={12}>*{getLocale('cryptoDotComWidgetApproxTotalSpent')}</Text>
          ) : (
            <Text weight={600} textColor='light' $fontSize={12}>*{getLocale('cryptoDotComWidgetApproxTotalReceived')}</Text>
          )}
          <Text $fontSize={16}>{quote === 'USDT' ? formattedNum(approxTotal) : approxTotal} {quote}</Text>
        </BasicBox>
        <Text textColor='light' $fontSize={12}>* {getLocale('cryptoDotComWidgetApproxFootnote')}</Text>
      </Box>
      <BasicBox $pt={15}>
        <ActionButton onClick={handleConfirmClick}>{getLocale('cryptoDotComWidgetConfirm')} ({counter}s)</ActionButton>
        <PlainButton $pb={5} onClick={handleCancelClick} $pt={10} $m='0 auto' textColor='light'>{getLocale('cryptoDotComWidgetCancel')}</PlainButton>
      </BasicBox>
    </>
  ) : (
    <Box $p={0}>
      <FlexItem
        hasPadding={true}
        isFlex={true}
        isFullWidth={true}
        hasBorder={true}
      >
        <FlexItem>
          <BackArrow onClick={handleBackClick}>
            <CaratLeftIcon />
          </BackArrow>
        </FlexItem>
        <FlexItem $pr={5}>
          {renderIconAsset(base.toLowerCase())}
        </FlexItem>
        <FlexItem flex={1}>
          <Text>{base}</Text>
          <Text small={true} textColor='light'>
            {currencyNames[base]}
          </Text>
        </FlexItem>
        <FlexItem $pl={5}>
          <ButtonGroup>
            <PlainButton onClick={() => setTradeMode(TradeModes.BUY)} textColor='green'>Buy</PlainButton>
            <PlainButton onClick={() => setTradeMode(TradeModes.SELL)} textColor='red'>Sell</PlainButton>
          </ButtonGroup>
        </FlexItem>
      </FlexItem>
      <FlexItem
        hasPadding={true}
        isFullWidth={true}
        hasBorder={true}
      >
        {tradeMode === TradeModes.BUY ? (
          <Text $mt={15} center={true}>{availableBalance} {quote} {getLocale('cryptoDotComWidgetAvailable')}</Text>
        ) : (
          <Text $mt={15} center={true}>{availableBalance} {base} {getLocale('cryptoDotComWidgetAvailable')}</Text>
        )}
        <AmountInputField
          $mt={10} $mb={10}
          placeholder={getPlaceholderText()}
          onChange={handleAmountChange} 
          value={tradeAmount}
        />
        <BasicBox isFlex={true} justify="center" $mb={13.5}>
          {Object.values(Percentages).map(percentage => {
            return (typeof percentage === 'number') && (
              <PlainButton
                key={percentage}
                weight={500}
                textColor={tradePercentage === percentage ? 'white' : 'light'}
                onClick={() => handlePercentageClick(percentage)}
              >
                {percentage}%
              </PlainButton>
            )
          })}
        </BasicBox>
      </FlexItem>
      <FlexItem
        hasPadding={true}
        isFullWidth={true}
      >
        <ActionButton onClick={handlePurchaseClick} disabled={!tradeAmount} textOpacity={tradeAmount ? 1 : 0.6} $bg={tradeMode === TradeModes.BUY ? 'green' : 'red-bright'} upperCase={false}>{tradeModeLocaleStrings[tradeMode]} {base}</ActionButton>
      </FlexItem>
    </Box>
  )
}

function BalanceSummary ({
  availableBalance = 88121.01,
  holdings = [
    { currency: 'BTC', quantity: 10 },
    { currency: 'ETH', quantity: 2 },
    { currency: 'BAT', quantity: 1343 }
  ]
}) {
  const [hideBalance, setHideBalance] = React.useState(true)

  return <>
    <BasicBox isFlex={true} $mb={18}>
      <FlexItem>
        <Text textColor='light' $mb={5} $fontSize={12}>{getLocale('cryptoDotComWidgetAvailableBalance')}</Text>
        <Balance hideBalance={hideBalance}>
          <Text lineHeight={1.15} $fontSize={21}>{formattedNum(availableBalance)}</Text>
        </Balance>
      </FlexItem>
      <FlexItem>
        <BlurIcon onClick={() => setHideBalance(!hideBalance)}>
          {
            hideBalance
            ? <ShowIcon />
            : <HideIcon />
          }
        </BlurIcon>
      </FlexItem>
    </BasicBox>
    <Text textColor='light' $mb={5} $fontSize={12}>{getLocale('cryptoDotComWidgetHoldings')}</Text>
    <List>
      {holdings.map(({currency, quantity}) => {
        return (
          <ListItem key={currency} isFlex={true} $height={40}>
            <FlexItem $pl={5} $pr={5}>
              {renderIconAsset(currency.toLowerCase())}
            </FlexItem>
            <FlexItem>
              <Text>{currency}</Text>
            </FlexItem>
            <FlexItem textAlign='right' flex={1}>
              <Balance hideBalance={hideBalance}>
                {(quantity !== null) && <Text lineHeight={1.15}>{quantity}</Text>}
              </Balance>
            </FlexItem>
          </ListItem>
        )
      })}
    </List>
  </>
}

function TopMovers ({
  losersGainers = {},
  handleAssetClick
}: any) {
  enum FilterValues {
    LOSERS = 'losers',
    GAINERS = 'gainers'
  }  

  const [filter, setFilter] = React.useState(FilterValues.GAINERS);

  const sortTopMovers = (a: Record<string, any>, b: Record<string, any>) => {
    if (filter === FilterValues.GAINERS) {
      return b.percentChange - a.percentChange
    } else {
      return a.percentChange - b.percentChange
    }
  }

  return <>
    <ButtonGroup>
      <PlainButton onClick={() => setFilter(FilterValues.GAINERS)} isActive={filter === FilterValues.GAINERS}>{getLocale('cryptoDotComWidgetGainers')}</PlainButton>
      <PlainButton onClick={() => setFilter(FilterValues.LOSERS)} isActive={filter === FilterValues.LOSERS}>{getLocale('cryptoDotComWidgetLosers')}</PlainButton>
    </ButtonGroup>
    <List>
      {losersGainers[filter] && losersGainers[filter]
        .sort(sortTopMovers)
        .map((asset: Record<string, any>) => {
        const currency = asset.pair.split('_')[0];
        const { percentChange, lastPrice: price } = asset

        return (
          <ListItem key={currency} isFlex={true} onClick={() => handleAssetClick(currency)} $height={48}>
            <FlexItem $pl={5} $pr={5}>
              {renderIconAsset(currency.toLowerCase())}
            </FlexItem>
            <FlexItem>
              <Text>{currency}</Text>
              <Text small={true} textColor='light'>{currencyNames[currency]}</Text>
            </FlexItem>
            <FlexItem textAlign='right' flex={1}>
              {(price !== null) && <Text>{formattedNum(price)}</Text>}
              {(percentChange !== null) && <Text textColor={getPercentColor(percentChange)}>{percentChange}%</Text>}
            </FlexItem>
          </ListItem>
        )
      })}
    </List>
  </>
}

function Trade ({
  tickerPrices = {},
  losersGainers = {},
  tradingPairs = [],
  handleAssetClick
}: any) {
  const assetRankings = transformLosersGainers(losersGainers)

  enum FilterValues {
    BTC = 'BTC',
    CRO = 'CRO',
    USDT = 'USDT'
  }

  const [filter, setFilter] = React.useState(FilterValues.BTC)
  const [searchInput, setSearchInput] = React.useState('')

  const handleSearchChange = ({ target }: any) => {
    const { value } = target
    setSearchInput(value)
  }

  const searchFilter = (pair: Record<string, string>) => {
    if (searchInput) {
      const search = new RegExp(searchInput, 'i')
      return search.test(pair.base)
    } else {
      return pair;
    }
  }

  return <>
    <ButtonGroup>
      <PlainButton onClick={() => setFilter(FilterValues.BTC)} isActive={filter === FilterValues.BTC}>BTC</PlainButton>
      <PlainButton onClick={() => setFilter(FilterValues.CRO)} isActive={filter === FilterValues.CRO}>CRO</PlainButton>
      <PlainButton onClick={() => setFilter(FilterValues.USDT)} isActive={filter === FilterValues.USDT}>USDT</PlainButton>
    </ButtonGroup>
    <Box isFlex={true} $height={30} hasBottomBorder={false}>
      <img width={15} src={SearchIcon} />
      <InputField value={searchInput} onChange={handleSearchChange} placeholder={getLocale('cryptoDotComWidgetSearch')} />
    </Box>
    <List>
      {tradingPairs
        .filter((pair: Record<string, string>) => pair.quote === filter)
        .filter(searchFilter)
        .map((pair: Record<string, string>) => {
          const { price = null } = tickerPrices[pair.pair] || {}
          const { percentChange = null } = assetRankings[pair.base] || {}

          return (
            <ListItem key={pair.pair} isFlex={true} $height={48} onClick={() => handleAssetClick(pair.base, pair.quote, AssetViews.TRADE)}
            >
              <FlexItem $pl={5} $pr={5}>
                {renderIconAsset(pair.base.toLowerCase())}
              </FlexItem>
              <FlexItem>
                <Text>{pair.base}/{pair.quote}</Text>
              </FlexItem>
              <FlexItem textAlign='right' flex={1}>
                {(price !== null) && <Text>
                  {filter === FilterValues.USDT ? formattedNum(price) : price}
                </Text>}
                {(percentChange !== null) && <Text textColor={getPercentColor(percentChange)}>{percentChange}%</Text>}
              </FlexItem>
            </ListItem>
          )
      })}
    </List>
  </>
}

function Events ({
  newsEvents = []
}: any) {
  return <List>
    {newsEvents.map((event: any) => (
      <ListItem $p={10}>
        <Text $fontSize={12} textColor='light'>{event.updated_at}</Text>
        <Text $fontSize={12}>{event.content}</Text>
        <Link $fontSize={12} $mt={5} inlineBlock={true} href={event.redirect_url} target='_blank'>{event.redirect_title}</Link>
      </ListItem>
    ))}
  </List>
}

// Utility functions
function renderIconAsset (key: string) {
  if (!(key in icons)) {
    return null
  }

  return (
    <>
      <img width={25} src={icons[key]} />
    </>
  )
}

function formattedNum (price: number) {
  return new Intl.NumberFormat('en-US', {
    style: 'currency',
    currency: 'USD',
    currencyDisplay: 'narrowSymbol'
  }).format(price)
}

// This is a temporary function only necessary for MVP
// Merges losers/gainers into one table
function transformLosersGainers ({ losers = [], gainers = [] }: Record<string, AssetRanking[]>): Record<string, AssetRanking> {
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

function getPercentColor (percentChange: string) {
  const percentChangeNum = parseFloat(percentChange)
  return percentChangeNum === 0 ? 'light' : (percentChangeNum > 0 ? 'green' : 'red')
}

async function copyToClipboard (address: string) {
  try {
    await clipboardCopy(address)
  } catch (e) {
    console.log(`Could not copy address ${e.toString()}`)
  }
}