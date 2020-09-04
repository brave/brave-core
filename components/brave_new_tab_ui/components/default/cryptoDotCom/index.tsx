/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

// MOCK DATA
const topMovers = [
  { currency: 'BTC', tickerPrice: 18484.10, tickerChange: 210.01 },
  { currency: 'ETH', tickerPrice: 334.54, tickerChange: 152.78 },
  { currency: 'CRO', tickerPrice: 1.20, tickerChange: 89.12 }
]

const currencyData = {
  btc: {
    volume24hr: 1271112419,
    chartData: [
      { x: 1, y: 2 },
      { x: 2, y: 3 },
      { x: 3, y: 5 },
      { x: 4, y: 4 },
      { x: 5, y: 7 }
    ]
  }
}

import * as React from 'react'

import createWidget from '../widget/index'
import { StyledTitleTab } from '../widgetTitleTab'

import currencyNames from './data'

import {
  ActionAnchor,
  ActionButton,
  AssetIcon,
  AssetIconWrapper,
  BackArrow,
  Box,
  CryptoDotComIcon,
  FlexItem,
  Header,
  List,
  ListItem,
  PlainButton,
  StyledTitle,
  StyledTitleText,
  Text,
  WidgetWrapper
} from './style'
// import {
//   SearchIcon,
//   ShowIcon,
//   HideIcon
// } from '../exchangeWidget/shared-assets'
import CryptoDotComLogo from './assets/cryptoDotCom-logo'
import { CaratLeftIcon } from 'brave-ui/components/icons'

// Utils
import cryptoColors from '../exchangeWidget/colors'
// import { getLocale } from '../../../../common/locale'

interface State {
  selectedView: string,
  selectedAsset: string
}

interface Props {
  showContent: boolean
  optInTotal: boolean
  optInBTCPrice: boolean
  tickerPrices: Record<string, string>
  stackPosition: number
  onShowContent: () => void
  onDisableWidget: () => void
  onTotalPriceOptIn: () => void
  onBtcPriceOptIn: () => void
}

class CryptoDotCom extends React.PureComponent<Props, State> {
  constructor (props: Props) {
    super(props)
    this.state = {
      selectedView: 'index',
      selectedAsset: ''
    }
  }

  setSelectedView = (view: string) => {
    this.setState({
      selectedView: view
    })
  }

  setSelectedAsset = (asset: string) => {
    this.setState({
      selectedAsset: asset
    })
  }

  renderIconAsset = (key: string, isDetail: boolean = false) => {
    const iconBgColor = cryptoColors[key] || '#fff'

    return (
      <AssetIconWrapper style={{ background: iconBgColor, color: '#000' }}>
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
    return (
      <>
        <Box isFlex={true}>
          <FlexItem style={{ paddingLeft: 5, paddingRight: 5 }}>
            {this.renderIconAsset(currency.toLowerCase())}
          </FlexItem>
          <FlexItem>
              <Text>{currency}</Text>
              <Text small={true} $color='light'>{currencyNames[currency]}</Text>
          </FlexItem>
          <FlexItem textAlign='right' flex={1}>
            {optInBTCPrice ? (
              <>
                <Text>{new Intl.NumberFormat('en-IN', { style: 'currency', currency: 'USD', currencyDisplay: 'narrowSymbol' }).format(18484.10)}</Text>
                <Text $color='green'>{210.01}%</Text>
              </>
            ) : (
              <PlainButton onClick={onBtcPriceOptIn} textColor='green'>Show Price</PlainButton>
            )}
          </FlexItem>
          <FlexItem style={{ paddingLeft: 5 }}>
            <ActionButton small={true} light={true}>BUY</ActionButton>
          </FlexItem>
        </Box>
        <Text center={true} style={{ marginTop: '1em' }}>Get 2% bonus on deposits</Text>
        <Text center={true}>Stop paying trading fees</Text>
        <ActionAnchor href='#' style={{ margin: '1em 0' }}>Buy Bitcoin Now</ActionAnchor>
        <PlainButton textColor='light' onClick={this.setSelectedView.bind(this, 'topMovers')} style={{ margin: '0 auto' }}>View Crypto.com Markets</PlainButton>
      </>
    )
  }

  renderTopMoversView () {
    return (
      <List>
        {topMovers.map(({ currency, tickerPrice, tickerChange }) => (
          <ListItem isFlex={true} onClick={this.setSelectedAsset.bind(this, currency)}>
            <FlexItem style={{ paddingLeft: 5, paddingRight: 5 }}>
              {this.renderIconAsset(currency.toLowerCase())}
            </FlexItem>
            <FlexItem>
              <Text>{currency}</Text>
              <Text small={true} $color='light'>{currencyNames[currency]}</Text>
            </FlexItem>
            <FlexItem textAlign='right' flex={1}>
              <Text>{new Intl.NumberFormat('en-IN', { style: 'currency', currency: 'USD', currencyDisplay: 'narrowSymbol' }).format(tickerPrice)}</Text>
              <Text $color={tickerChange > 0 ? 'green' : 'red'}>{tickerChange}%</Text>
            </FlexItem>
          </ListItem>
        ))}
      </List>
    )
  }

  renderAssetDetailView () {
    const { selectedAsset: currency } = this.state
    const assetData = currencyData[currency.toLowerCase()]
    return (
      <Box noPadding={true}>
        <FlexItem
          hasPadding={true}
          style={{
            display: 'flex',
            justifyContent: 'space-between',
            alignItems: 'center',
            width: '100%',
            borderBottom: '1px solid #979797'
          }}
        >
          <FlexItem>
            <BackArrow>
              <CaratLeftIcon onClick={this.setSelectedAsset.bind(this, '')} />
            </BackArrow>
          </FlexItem>
          <FlexItem style={{ paddingRight: 5 }}>
            {this.renderIconAsset(currency.toLowerCase())}
          </FlexItem>
          <FlexItem flex={1}>
            <Text>{currency}</Text>
            <Text small={true} $color='light'>
              {currencyNames[currency]}
            </Text>
          </FlexItem>
          <FlexItem style={{ paddingLeft: 5 }}>
            <ActionButton small={true} light={true}>
              BUY
            </ActionButton>
          </FlexItem>
        </FlexItem>
        <FlexItem
          hasPadding={true}
          style={{
            width: '100%',
            borderBottom: '1px solid #979797'
          }}
        >
          {/* <VictoryLine
            padding={0}
            style={{
              data: { stroke: "#44B0FF" }
            }}
            animate={{
              duration: 2000,
              onLoad: { duration: 1000 }
            }}
            data={assetData.chartData}
          /> */}
        </FlexItem>
        <FlexItem
          hasPadding={true}
          style={{
            width: '100%'
          }}
        >
          <div style={{ marginTop: '0.2em' }}>
            <Text small={true} $color='light' style={{ paddingBottom: '0.2rem' }}>
              24HR VOLUME
            </Text>
            <Text weight={500}>{new Intl.NumberFormat('en-IN', { style: 'currency', currency: 'USD', currencyDisplay: 'narrowSymbol' }).format(assetData.volume24hr)} USDT</Text>
          </div>
          <div style={{ marginTop: '1em' }}>
            <Text small={true} $color='light' style={{ paddingBottom: '0.2rem' }}>SUPPORTED PAIRS</Text>
            <ActionButton small={true} light={true} inline={true} style={{ marginRight: 5 }}>BTC/CRO</ActionButton>
            <ActionButton small={true} light={true} inline={true}>BTC/USDT</ActionButton>
          </div>
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
            {'crypto.com'}
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
    const { selectedView } = this.state
    const { selectedAsset } = this.state

    if (selectedAsset) {
      return this.renderAssetDetailView()
    }

    switch (selectedView) {
      case 'topMovers':
        return this.renderTopMoversView()
      default:
        return this.renderIndexView()
    }
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
