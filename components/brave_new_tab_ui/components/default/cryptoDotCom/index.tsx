/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

// MOCK DATA
const topMovers = [
  { currency: "BTC", tickerPrice: 18484.10, tickerChange: 210.01 },
  { currency: "ETH", tickerPrice: 334.54, tickerChange: 152.78 },
  { currency: "CRO", tickerPrice: 1.20, tickerChange: 89.12 }
];

import * as React from 'react'

import createWidget from '../widget/index'
import { StyledTitleTab } from '../widgetTitleTab'

import currencyNames from './data'

import {
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
  WidgetWrapper,
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

  componentDidMount () {}

  componentDidUpdate (prevProps: Props) {}

  componentWillUnmount () {}

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
      <AssetIconWrapper style={{ background: iconBgColor, color: "#000" }}>
        <AssetIcon
          isDetail={isDetail}
          className={`crypto-icon icon-${key}`}
        />
      </AssetIconWrapper>
    )
  }

  renderIndexView () {
    const { optInBTCPrice, onBtcPriceOptIn } = this.props;
    const currency = "BTC";
    return <>
      <Box isFlex>
        <FlexItem style={{ paddingLeft: 5, paddingRight: 5 }}>
          {this.renderIconAsset(currency.toLowerCase())}
        </FlexItem>
        <FlexItem>
            <Text>{currency}</Text>
            <Text small $color="light">{currencyNames[currency]}</Text>
        </FlexItem>
        <FlexItem textAlign="right" flex={1}>
          {optInBTCPrice ? (
            <>
              <Text>$18,484.10</Text>
              <Text $color="green">210.01%</Text>
            </>
          ) : (
            <PlainButton onClick={onBtcPriceOptIn} textColor="green">Show Price</PlainButton>
          )}
        </FlexItem>
        <FlexItem style={{ paddingLeft: 5 }}>
          <ActionButton small light>BUY</ActionButton>
        </FlexItem>
      </Box>
      <Text $hasSpacing>Connect to Crypto.com to view buy and trade crypto, view account balance and upcoming events.</Text>
      <ActionButton onClick={() => this.setSelectedView('topMovers')}>View Top Movers</ActionButton>
    </>
  }

  renderTopMoversView () {
    return (
      <List>
        {topMovers.map(({currency, tickerPrice, tickerChange}) => (
          <ListItem isFlex onClick={() => this.setSelectedAsset(currency)}>
            <FlexItem style={{ paddingLeft: 5, paddingRight: 5 }}>
              {this.renderIconAsset(currency.toLowerCase())}
            </FlexItem>
            <FlexItem>
              <Text>{currency}</Text>
              <Text small $color="light">{currencyNames[currency]}</Text>
            </FlexItem>
            <FlexItem textAlign="right" flex={1}>
              <Text>${tickerPrice.toFixed(2)}</Text>
              <Text $color={tickerChange > 0 ? "green" : "red"}>{tickerChange}%</Text>
            </FlexItem>
          </ListItem>
        ))}
      </List>
    );
  }

  renderAssetDetailView () {
    const { selectedAsset: currency } = this.state;
    return (
      <Box isFlex>
        <FlexItem>
          <BackArrow>
            <CaratLeftIcon onClick={() => this.setSelectedAsset('')} />
          </BackArrow>
        </FlexItem>
        <FlexItem style={{ paddingRight: 5 }}>
          {this.renderIconAsset(currency.toLowerCase())}
        </FlexItem>
        <FlexItem flex={1}>
            <Text>{currency}</Text>
            <Text small $color="light">{currencyNames[currency]}</Text>
        </FlexItem>
        <FlexItem style={{ paddingLeft: 5 }}>
          <ActionButton small light>BUY</ActionButton>
        </FlexItem>
      </Box>
    )
  }

  renderTitle () {
    const { showContent } = this.props;
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
    const { selectedView } = this.state;
    const { selectedAsset } = this.state;
    
    if (selectedAsset) {
      return this.renderAssetDetailView();
    }

    switch(selectedView) {
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
