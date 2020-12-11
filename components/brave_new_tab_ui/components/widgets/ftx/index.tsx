/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/. */
import * as React from 'react'
import { ThemeProvider } from 'styled-components'

import createWidget from '../../default/widget/index'
import { StyledTitleTab } from '../../default/widgetTitleTab'

import { currencyNames } from '../shared/data'

import {
  ActionAnchor,
  ActionButton,
  BackArrow,
  BasicBox,
  Box,
  WidgetIcon,
  FlexItem,
  Header,
  Filters,
  FilterOption,
  List,
  ListItem,
  PlainButton,
  StyledTitle,
  StyledTitleText,
  Text,
  WidgetWrapper,
  UpperCaseText
} from '../shared/styles'
import { Chart } from '../shared'
import { CaratLeftIcon } from 'brave-ui/components/icons'
import icons from '../shared/assets/icons'
import ftxLogo from './ftx-logo.png'
import ftxTheme from './theme'

// Utils

interface State {
  selectedAsset: string
}

interface Props {
  showContent: boolean
  optInMarkets: boolean
  stackPosition: number
  onShowContent: () => void
//  onOptInMarkets: (show: boolean) => void
}

class FTX extends React.PureComponent<Props, State> {
  private refreshInterval: any
  private topMovers: string[] = Object.keys(currencyNames)

  constructor (props: Props) {
    super(props)
    this.state = {
      selectedAsset: ''
    }
  }

  componentDidMount () {
    const { optInMarkets } = this.props

    if (optInMarkets) {
      this.checkSetRefreshInterval()
    }
  }

  componentWillUnmount () {
    this.clearIntervals()
  }

  checkSetRefreshInterval = () => {
    if (!this.refreshInterval) {
      // TODO: do refresh stuff
      // this.refreshInterval = setInterval(async () => {
      // }, 30000)
    }
  }

  clearIntervals = () => {
    clearInterval(this.refreshInterval)
    this.refreshInterval = null
  }

  setSelectedAsset = (asset: string) => {
    this.setState({
      selectedAsset: asset
    })
  }

  handleViewMarketsClick = async () => {
    // TODO: record click
  }

  optInMarkets = (show: boolean) => {
    if (show) {
      this.checkSetRefreshInterval()
    } else {
      this.setState({ selectedAsset: '' })
    }

    // TODO: show markets
  }

  handleAssetDetailClick = async (asset: string) => {
    this.setSelectedAsset(asset)
  //  await this.props.onSetAssetData([asset])
  }

  formattedNum = (price: number) => {
    return new Intl.NumberFormat('en-US', {
      style: 'currency',
      currency: 'USD',
      currencyDisplay: 'narrowSymbol'
    }).format(price)
  }

  renderIconAsset = (key: string) => {
    if (!(key in icons)) {
      return null
    }

    return (
      <>
        <img width={25} src={icons[key]} />
      </>
    )
  }

  renderIndexView () {
    return (
      <>
        <BasicBox isFlex={true} justify='flex-end'>
          <PlainButton weight={600}>.com</PlainButton>
          <PlainButton weight={600} textColor='xlight'>.us</PlainButton>
        </BasicBox>
        <Text $fontSize={13} weight={600} $pb={6}>
          FTX.com
        </Text>
        <Text $fontSize={13} textColor='light' lineHeight={1.5} $pb={21}>
          Connect FTX account to view account balance, explore futures markets, & convert crypto.
        </Text>
        <ActionAnchor>
          View Future Markets
        </ActionAnchor>
        <PlainButton textColor='light' onClick={this.handleViewMarketsClick} $m='0 auto'>
          Connect Account
        </PlainButton>
      </>
    )
  }

  renderTopMoversView () {
    return <>
      <BasicBox isFlex={true} justify="start">
        <PlainButton $pl="0" weight={600} textColor="white">Markets</PlainButton>
        <PlainButton weight={600} textColor="light">Convert</PlainButton>
        <PlainButton weight={600} textColor="light">Summary</PlainButton>
      </BasicBox>
      <Filters>
        <FilterOption isActive={true}>Futures</FilterOption>
        <FilterOption>Special</FilterOption>
      </Filters>
      <List>
        {this.topMovers.map(currency => {
          const { price = null } = null || { price: 1000 }
          const losersGainers = {}
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
    </>
  }

  renderAssetDetailView () {
    const { selectedAsset: currency } = this.state
    const { price = null, volume = null } = null || { price: 1000, volume: 9999 }
    const chartData = null || []

    const losersGainers = {}
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
            <ActionButton small={true} light={true}>
              <UpperCaseText>
                {'ftxWidgetBuy'}
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
          <Chart width={chartWidth} height={chartHeight} data={chartData} />
        <Text small={true} textColor='xlight'>
          {'ftxWidgetGraph'}
        </Text>
        </FlexItem>
        <FlexItem
          hasPadding={true}
          isFullWidth={true}
        >
          <BasicBox $mt='0.2em'>
            <Text small={true} textColor='light' $pb='0.2rem'>
              <UpperCaseText>
                {'ftxWidgetVolume'}
              </UpperCaseText>
            </Text>
            {volume && <Text weight={500}>{this.formattedNum(volume)} USDT</Text>}
          </BasicBox>
          <BasicBox $mt='1em'>
            <Text small={true} textColor='light' $pb='0.2rem'>
              <UpperCaseText>
                {'ftxWidgetPairs'}
              </UpperCaseText>
            </Text>
          </BasicBox>
        </FlexItem>
      </Box>
    )
  }

  renderTitle () {
    const { selectedAsset } = this.state
    const { optInMarkets, showContent } = this.props
    const shouldShowBackArrow = !selectedAsset && showContent && optInMarkets

    return (
      <Header showContent={showContent}>
        <StyledTitle>
          <WidgetIcon>
            <img src={ftxLogo} alt="FTX logo"/>
          </WidgetIcon>
          <StyledTitleText>
            Widget Title
          </StyledTitleText>
          {shouldShowBackArrow &&
            <BackArrow marketView={true}>
              <CaratLeftIcon
                onClick={this.optInMarkets.bind(this, false)}
              />
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

  renderSelectedView () {
    const { selectedAsset } = this.state
    const { optInMarkets } = this.props

    if (selectedAsset) {
      return this.renderAssetDetailView()
    }

    if (optInMarkets || true /* TODO: remove */) {
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
      <ThemeProvider theme={ftxTheme}>
        <WidgetWrapper tabIndex={0}>
          {this.renderTitle()}
          {this.renderSelectedView()}
        </WidgetWrapper>
      </ThemeProvider>
    )
  }
}

export const FTXWidget = createWidget(FTX)
