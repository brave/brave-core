/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */
 import * as React from 'react'

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
   List,
   ListItem,
   PlainButton,
   StyledTitle,
   StyledTitleText,
   SVG,
   Text,
   WidgetWrapper,
   UpperCaseText
 } from '../shared/styles'
 import { CaratLeftIcon } from 'brave-ui/components/icons'
 import icons from '../shared/assets/icons'

 // Utils
 import { getLocale } from '../../../../common/locale'

 interface State {
   selectedAsset: string
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
  //  optInTotal: boolean
  //  optInBTCPrice: boolean
   optInMarkets: boolean
  //  tickerPrices: Record<string, TickerPrice>
  //  losersGainers: Record<string, AssetRanking[]>
  //  supportedPairs: Record<string, string[]>
  //  charts: Record<string, ChartDataPoint[]>
   stackPosition: number
   onShowContent: () => void
  //  onDisableWidget: () => void
  //  onBtcPriceOptIn: () => void
  //  onUpdateActions: () => Promise<void>
  //  onViewMarketsRequested: (assets: string[]) => Promise<void>
  //  onSetAssetData: (assets: string[]) => Promise<void>
  //  onBuyCrypto: () => void
  //  onInteraction: () => void
  //  onOptInMarkets: (show: boolean) => void
 }
 interface ChartConfig {
   data: Array<any>
   chartHeight: number
   chartWidth: number
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
       this.refreshInterval = setInterval(async () => {
         // TODO: do refresh stuff
       }, 30000)
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

   onClickBuyTopDetail = () => {
     window.open('', '_blank', 'noopener')
    //  this.props.onBuyCrypto()
   }

   onClickBuyTop = () => {
     window.open('', '_blank', 'noopener')
    //  this.props.onBuyCrypto()
   }

   onClickBuyBottom = () => {
     window.open('', '_blank', 'noopener')
    //  this.props.onBuyCrypto()
   }

   onClickBuyPair = (pair: string) => {
     window.open('', '_blank', 'noopener')
    //  this.props.onBuyCrypto()
   }

   formattedNum = (price: number) => {
     return new Intl.NumberFormat('en-US', {
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
     const currency = 'BTC'

     return (
       <>
         <Box isFlex={true} $height={48} hasPadding={true}>
           <FlexItem $pr={5}>
             {this.renderIconAsset(currency.toLowerCase())}
           </FlexItem>
           <FlexItem>
               <Text>{currency}</Text>
               <Text small={true} textColor='light'>{currencyNames[currency]}</Text>
           </FlexItem>
           <FlexItem textAlign='right' flex={1}>
             I'm where the text used to be
           </FlexItem>
           <FlexItem $pl={5}>
             <ActionButton small={true} light={true}>
               {getLocale('ftxWidgetBuy')}
             </ActionButton>
           </FlexItem>
         </Box>
         <Text center={true} $p='1em 0 0.5em' $fontSize={15}>
           {getLocale('ftxWidgetCopyOne')}
         </Text>
         <Text center={true} $fontSize={15}>
           {getLocale('ftxWidgetCopyTwo')}
         </Text>
         <ActionAnchor onClick={this.onClickBuyBottom}>
           {getLocale('ftxWidgetBuyBtc')}
         </ActionAnchor>
         <PlainButton textColor='light' onClick={this.handleViewMarketsClick} $m='0 auto'>
           {getLocale('ftxWidgetViewMarkets')}
         </PlainButton>
       </>
     )
   }

   renderTopMoversView () {
     return (
       <List>
         {this.topMovers.map(currency => {
           const { price = null } = null || {}
           const losersGainers = this.transformLosersGainers(null || {})
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
     const { price = null, volume = null } = null || {}
     const chartData = null || []

     const losersGainers = this.transformLosersGainers(null || {})
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
             <ActionButton onClick={this.onClickBuyTopDetail} small={true} light={true}>
               <UpperCaseText>
                 {getLocale('ftxWidgetBuy')}
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
           {getLocale('ftxWidgetGraph')}
         </Text>
         </FlexItem>
         <FlexItem
           hasPadding={true}
           isFullWidth={true}
         >
           <BasicBox $mt='0.2em'>
             <Text small={true} textColor='light' $pb='0.2rem'>
               <UpperCaseText>
                 {getLocale('ftxWidgetVolume')}
               </UpperCaseText>
             </Text>
             {volume && <Text weight={500}>{this.formattedNum(volume)} USDT</Text>}
           </BasicBox>
           <BasicBox $mt='1em'>
             <Text small={true} textColor='light' $pb='0.2rem'>
               <UpperCaseText>
                 {getLocale('ftxWidgetPairs')}
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
             {/* Logo */}
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

 export const FTXWidget = createWidget(FTX)
