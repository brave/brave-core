// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Components
import Stats from './stats'
import TopSitesGrid from './gridSites'
import FooterInfo from '../../components/default/footer/footer'
import SiteRemovalNotification from './notification'
import {
  ClockWidget as Clock,
  RewardsWidget as Rewards,
  BraveTalkWidget as BraveTalk,
  BinanceWidget as Binance,
  GeminiWidget as Gemini,
  CryptoDotComWidget as CryptoDotCom,
  EditTopSite,
  EditCards
} from '../../components/default'
import { FTXWidget as FTX } from '../../widgets/ftx/components'
import * as Page from '../../components/default/page'
import BrandedWallpaperLogo from '../../components/default/brandedWallpaper/logo'
import { brandedWallpaperLogoClicked } from '../../api/wallpaper'
import BraveTodayHint from '../../components/default/braveToday/hint'
import BraveToday, { GetDisplayAdContent } from '../../components/default/braveToday'
import { SponsoredImageTooltip } from '../../components/default/rewards'
import { addNewTopSite, editTopSite } from '../../api/topSites'

// Helpers
import VisibilityTimer from '../../helpers/visibilityTimer'
import {
  fetchCryptoDotComTickerPrices,
  fetchCryptoDotComLosersGainers,
  fetchCryptoDotComCharts,
  fetchCryptoDotComSupportedPairs
} from '../../api/cryptoDotCom'
import { generateQRData } from '../../binance-utils'

// Types
import { GeminiAssetAddress } from '../../actions/gemini_actions'
import { getLocale } from '../../../common/locale'
import currencyData from '../../components/default/binance/data'
import geminiData from '../../components/default/gemini/data'
import { NewTabActions } from '../../constants/new_tab_types'
import { BraveTodayState } from '../../reducers/today'
import { FTXState } from '../../widgets/ftx/ftx_state'

// NTP features
import Settings, { TabType as SettingsTabType } from './settings'
import { MAX_GRID_SIZE } from '../../constants/new_tab_ui'

interface Props {
  newTabData: NewTab.State
  gridSitesData: NewTab.GridSitesState
  todayData: BraveTodayState
  ftx: FTXState
  actions: NewTabActions
  getBraveNewsDisplayAd: GetDisplayAdContent
  saveShowBackgroundImage: (value: boolean) => void
  saveShowStats: (value: boolean) => void
  saveShowToday: (value: boolean) => any
  saveShowRewards: (value: boolean) => void
  saveShowBraveTalk: (value: boolean) => void
  saveShowBinance: (value: boolean) => void
  saveShowGemini: (value: boolean) => void
  saveShowCryptoDotCom: (value: boolean) => void
  saveShowFTX: (value: boolean) => void
  saveBrandedWallpaperOptIn: (value: boolean) => void
  saveSetAllStackWidgets: (value: boolean) => void
}

interface State {
  showSettingsMenu: boolean
  showEditTopSite: boolean
  targetTopSiteForEditing?: NewTab.Site
  backgroundHasLoaded: boolean
  activeSettingsTab: SettingsTabType | null
}

function GetBackgroundImageSrc (props: Props) {
  if (!props.newTabData.showBackgroundImage &&
      (!props.newTabData.brandedWallpaper || props.newTabData.brandedWallpaper.isSponsored)) {
    return undefined
  }
  if (props.newTabData.brandedWallpaper) {
    const wallpaperData = props.newTabData.brandedWallpaper
    if (wallpaperData.wallpaperImageUrl) {
      return wallpaperData.wallpaperImageUrl
    }
  }
  if (props.newTabData.backgroundWallpaper && props.newTabData.backgroundWallpaper.wallpaperImageUrl) {
    return props.newTabData.backgroundWallpaper.wallpaperImageUrl
  }
  return undefined
}

function GetIsShowingBrandedWallpaper (props: Props) {
  const { newTabData } = props
  return !!((newTabData.brandedWallpaper &&
          newTabData.brandedWallpaper.isSponsored))
}

function GetShouldShowBrandedWallpaperNotification (props: Props) {
  return GetIsShowingBrandedWallpaper(props) &&
  !props.newTabData.isBrandedWallpaperNotificationDismissed
}

class NewTabPage extends React.Component<Props, State> {
  state: State = {
    showSettingsMenu: false,
    showEditTopSite: false,
    backgroundHasLoaded: false,
    activeSettingsTab: null
  }

  hasInitBraveToday: boolean = false
  imageSource?: string = undefined
  timerIdForBrandedWallpaperNotification?: number = undefined
  onVisiblityTimerExpired = () => {
    this.dismissBrandedWallpaperNotification(false)
  }

  visibilityTimer = new VisibilityTimer(this.onVisiblityTimerExpired, 4000)

  componentDidMount () {
    // if a notification is open at component mounting time, close it
    this.props.actions.showTilesRemovedNotice(false)
    this.imageSource = GetBackgroundImageSrc(this.props)
    this.trackCachedImage()
    if (GetShouldShowBrandedWallpaperNotification(this.props)) {
      this.trackBrandedWallpaperNotificationAutoDismiss()
    }
    this.checkShouldOpenSettings()
  }

  componentDidUpdate (prevProps: Props) {
    const oldImageSource = GetBackgroundImageSrc(prevProps)
    const newImageSource = GetBackgroundImageSrc(this.props)
    this.imageSource = newImageSource
    if (newImageSource && oldImageSource !== newImageSource) {
      this.trackCachedImage()
    }
    if (oldImageSource &&
      !newImageSource) {
      // reset loaded state
      this.setState({ backgroundHasLoaded: false })
    }
    if (!GetShouldShowBrandedWallpaperNotification(prevProps) &&
        GetShouldShowBrandedWallpaperNotification(this.props)) {
      this.trackBrandedWallpaperNotificationAutoDismiss()
    }

    if (GetShouldShowBrandedWallpaperNotification(prevProps) &&
        !GetShouldShowBrandedWallpaperNotification(this.props)) {
      this.stopWaitingForBrandedWallpaperNotificationAutoDismiss()
    }
  }

  trackCachedImage () {
    if (this.state.backgroundHasLoaded) {
      this.setState({ backgroundHasLoaded: false })
    }
    if (this.imageSource) {
      const imgCache = new Image()
      imgCache.src = this.imageSource
      console.timeStamp('image start loading...')
      imgCache.onload = () => {
        console.timeStamp('image loaded')
        this.setState({
          backgroundHasLoaded: true
        })
      }
    }
  }

  trackBrandedWallpaperNotificationAutoDismiss () {
    // Wait until page has been visible for an uninterupted Y seconds and then
    // dismiss the notification.
    this.visibilityTimer.startTracking()
  }

  checkShouldOpenSettings () {
    const params = window.location.search
    const urlParams = new URLSearchParams(params)
    const openSettings = urlParams.get('openSettings')

    if (openSettings) {
      this.setState({ showSettingsMenu: true })
      // Remove settings param so menu doesn't persist on reload
      window.history.pushState(null, '', '/')
    }
  }

  stopWaitingForBrandedWallpaperNotificationAutoDismiss () {
    this.visibilityTimer.stopTracking()
  }

  toggleShowBackgroundImage = () => {
    this.props.saveShowBackgroundImage(
      !this.props.newTabData.showBackgroundImage
    )
  }

  toggleShowClock = () => {
    this.props.actions.clockWidgetUpdated(
      !this.props.newTabData.showClock,
      this.props.newTabData.clockFormat)
  }

  toggleClockFormat = () => {
    const currentFormat = this.props.newTabData.clockFormat
    let newFormat
    // cycle through the available options
    switch (currentFormat) {
      case '': newFormat = '12'; break
      case '12': newFormat = '24'; break
      case '24': newFormat = ''; break
      default: newFormat = ''; break
    }
    this.props.actions.clockWidgetUpdated(
      this.props.newTabData.showClock,
      newFormat)
  }

  toggleShowStats = () => {
    this.props.saveShowStats(
      !this.props.newTabData.showStats
    )
  }

  toggleShowToday = () => {
    this.props.saveShowToday(
      !this.props.newTabData.showToday
    )
  }

  toggleShowTopSites = () => {
    const { showTopSites, customLinksEnabled } = this.props.newTabData
    this.props.actions.setMostVisitedSettings(!showTopSites, customLinksEnabled)
  }

  toggleCustomLinksEnabled = () => {
    const { showTopSites, customLinksEnabled } = this.props.newTabData
    this.props.actions.setMostVisitedSettings(showTopSites, !customLinksEnabled)
  }

  setMostVisitedSettings = (showTopSites: boolean, customLinksEnabled: boolean) => {
    this.props.actions.setMostVisitedSettings(showTopSites, customLinksEnabled)
  }

  toggleShowRewards = () => {
    this.props.saveShowRewards(!this.props.newTabData.showRewards)
  }

  toggleShowBraveTalk = () => {
    this.props.saveShowBraveTalk(!this.props.newTabData.showBraveTalk)
  }

  toggleShowBinance = () => {
    const { showBinance, binanceState } = this.props.newTabData

    this.props.saveShowBinance(!showBinance)

    if (!showBinance) {
      return
    }

    if (binanceState.userAuthed) {
      chrome.binance.revokeToken(() => {
        this.disconnectBinance()
      })
    } else {
      this.disconnectBinance()
    }
  }

  toggleShowGemini = () => {
    const { showGemini, geminiState } = this.props.newTabData

    this.props.saveShowGemini(!showGemini)

    if (!showGemini) {
      return
    }

    if (geminiState.userAuthed) {
      chrome.gemini.revokeToken(() => {
        this.disconnectGemini()
      })
    } else {
      this.disconnectGemini()
    }
  }

  toggleShowCryptoDotCom = () => {
    this.props.saveShowCryptoDotCom(!this.props.newTabData.showCryptoDotCom)
  }

  toggleShowFTX = () => {
    this.props.actions.ftx.disconnect()
    this.props.saveShowFTX(!this.props.newTabData.showFTX)
  }

  onBinanceClientUrl = (clientUrl: string) => {
    this.props.actions.onBinanceClientUrl(clientUrl)
  }

  onGeminiClientUrl = (clientUrl: string) => {
    this.props.actions.onGeminiClientUrl(clientUrl)
  }

  onValidBinanceAuthCode = () => {
    this.props.actions.onValidBinanceAuthCode()
  }

  onValidGeminiAuthCode = () => {
    this.props.actions.onValidGeminiAuthCode()
  }

  setBinanceHideBalance = (hide: boolean) => {
    this.props.actions.setBinanceHideBalance(hide)
  }

  setGeminiHideBalance = (hide: boolean) => {
    this.props.actions.setGeminiHideBalance(hide)
  }

  disconnectBinance = () => {
    this.props.actions.disconnectBinance()
  }

  setBinanceDisconnectInProgress = () => {
    this.props.actions.setBinanceDisconnectInProgress(true)
  }

  cancelBinanceDisconnect = () => {
    this.props.actions.setBinanceDisconnectInProgress(false)
  }

  disconnectGemini = () => {
    this.props.actions.disconnectGemini()
  }

  setGeminiDisconnectInProgress = () => {
    this.props.actions.setGeminiDisconnectInProgress(true)
  }

  cancelGeminiDisconnect = () => {
    this.props.actions.setGeminiDisconnectInProgress(false)
  }

  connectBinance = () => {
    this.props.actions.connectToBinance()
  }

  connectGemini = () => {
    this.props.actions.connectToGemini()
  }

  buyCrypto = (coin: string, amount: string, fiat: string) => {
    const { userLocale, userTLD } = this.props.newTabData.binanceState
    const refCode = userTLD === 'us' ? '35089877' : '39346846'
    const refParams = `ref=${refCode}&utm_source=brave`

    if (userTLD === 'us') {
      window.open(`https://www.binance.us/en/buy-sell-crypto?crypto=${coin}&amount=${amount}&${refParams}`, '_blank', 'noopener')
    } else {
      window.open(`https://www.binance.com/${userLocale}/buy-sell-crypto?fiat=${fiat}&crypto=${coin}&amount=${amount}&${refParams}`, '_blank', 'noopener')
    }
  }

  onBinanceUserTLD = (userTLD: NewTab.BinanceTLD) => {
    this.props.actions.onBinanceUserTLD(userTLD)
  }

  onBinanceUserLocale = (userLocale: string) => {
    this.props.actions.onBinanceUserLocale(userLocale)
  }

  setBalanceInfo = (info: Record<string, Record<string, string>>) => {
    this.props.actions.onAssetsBalanceInfo(info)
  }

  setAssetDepositInfo = (symbol: string, address: string, url: string) => {
    this.props.actions.onAssetDepositInfo(symbol, address, url)
  }

  disableBrandedWallpaper = () => {
    this.props.saveBrandedWallpaperOptIn(false)
  }

  toggleShowBrandedWallpaper = () => {
    this.props.saveBrandedWallpaperOptIn(
      !this.props.newTabData.brandedWallpaperOptIn
    )
  }

  startRewards = () => {
    chrome.braveRewards.enableRewards()
  }

  dismissBrandedWallpaperNotification = (isUserAction: boolean) => {
    this.props.actions.dismissBrandedWallpaperNotification(isUserAction)
  }

  dismissNotification = (id: string) => {
    this.props.actions.dismissNotification(id)
  }

  closeSettings = () => {
    this.setState({
      showSettingsMenu: false,
      activeSettingsTab: null
    })
  }

  showEditTopSite = (targetTopSiteForEditing?: NewTab.Site) => {
    this.setState({
      showEditTopSite: true,
      targetTopSiteForEditing
    })
  }

  closeEditTopSite = () => {
    this.setState({
      showEditTopSite: false
    })
  }

  saveNewTopSite = (title: string, url: string, newUrl: string) => {
    if (url) {
      editTopSite(title, url, newUrl === url ? '' : newUrl)
    } else {
      addNewTopSite(title, newUrl)
    }
    this.closeEditTopSite()
  }

  openSettings = (activeTab?: SettingsTabType) => {
    this.props.actions.customizeClicked()
    this.setState({
      showSettingsMenu: !this.state.showSettingsMenu,
      activeSettingsTab: activeTab || null
    })
  }

  onClickLogo = () => {
    brandedWallpaperLogoClicked(this.props.newTabData.brandedWallpaper)
  }

  openSettingsEditCards = () => {
    this.openSettings(SettingsTabType.Cards)
  }

  setForegroundStackWidget = (widget: NewTab.StackWidget) => {
    this.props.actions.setForegroundStackWidget(widget)
  }

  setInitialAmount = (amount: string) => {
    this.props.actions.setInitialAmount(amount)
  }

  setInitialFiat = (fiat: string) => {
    this.props.actions.setInitialFiat(fiat)
  }

  setInitialAsset = (asset: string) => {
    this.props.actions.setInitialAsset(asset)
  }

  setUserTLDAutoSet = () => {
    this.props.actions.setUserTLDAutoSet()
  }

  onBraveTodayInteracting = () => {
    if (!this.hasInitBraveToday) {
      this.hasInitBraveToday = true
      this.props.actions.today.interactionBegin()
    }
  }

  learnMoreRewards = () => {
    window.open('https://brave.com/brave-rewards/', '_blank', 'noopener')
  }

  learnMoreBinance = () => [
    window.open('https://brave.com/binance/', '_blank', 'noopener')
  ]

  setAssetDepositQRCodeSrc = (asset: string, src: string) => {
    this.props.actions.onDepositQRForAsset(asset, src)
  }

  setGeminiAssetAddress = (address: string, asset: string, qrCode: string) => {
    const assetAddresses: GeminiAssetAddress[] = [{ asset, address, qrCode }]
    if (asset === 'ETH') {
      // Use ETH's address and qrCode for all other erc tokens.
      geminiData.ercTokens.forEach((ercToken: string) => {
        assetAddresses.push({
          asset: ercToken,
          address,
          qrCode
        })
      })
    }

    this.props.actions.setGeminiAssetAddress(assetAddresses)
  }

  setConvertableAssets = (asset: string, assets: string[]) => {
    this.props.actions.onConvertableAssets(asset, assets)
  }

  setBinanceSelectedView = (view: string) => {
    this.props.actions.setBinanceSelectedView(view)
  }

  setGeminiSelectedView = (view: string) => {
    this.props.actions.setGeminiSelectedView(view)
  }

  setGeminiAuthInvalid = () => {
    this.props.actions.setGeminiAuthInvalid(true)
    this.props.actions.disconnectGemini()
  }

  binanceUpdateActions = () => {
    this.fetchBalance()
    this.getConvertAssets()
  }

  binanceRefreshActions = () => {
    this.fetchBalance()
    this.setDepositInfo()
    this.getConvertAssets()
  }

  geminiUpdateActions = () => {
    this.fetchGeminiTickerPrices()
    this.fetchGeminiBalances()
    this.fetchGeminiDepositInfo()
  }

  fetchGeminiTickerPrices = () => {
    geminiData.currencies.map((asset: string) => {
      chrome.gemini.getTickerPrice(`${asset}usd`, (price: string) => {
        this.props.actions.setGeminiTickerPrice(asset, price)
      })
    })
  }

  onCryptoDotComMarketsRequested = async (assets: string[]) => {
    const [tickerPrices, losersGainers] = await Promise.all([
      fetchCryptoDotComTickerPrices(assets),
      fetchCryptoDotComLosersGainers()
    ])
    this.props.actions.cryptoDotComMarketDataUpdate(tickerPrices, losersGainers)
  }

  onCryptoDotComAssetData = async (assets: string[]) => {
    const [charts, pairs] = await Promise.all([
      fetchCryptoDotComCharts(assets),
      fetchCryptoDotComSupportedPairs()
    ])
    this.props.actions.setCryptoDotComAssetData(charts, pairs)
  }

  cryptoDotComUpdateActions = async () => {
    const { supportedPairs, tickerPrices: prices } = this.props.newTabData.cryptoDotComState
    const assets = Object.keys(prices)
    const supportedPairsSet = Object.keys(supportedPairs).length

    const [tickerPrices, losersGainers, charts] = await Promise.all([
      fetchCryptoDotComTickerPrices(assets),
      fetchCryptoDotComLosersGainers(),
      fetchCryptoDotComCharts(assets)
    ])

    // These are rarely updated, so we only need to fetch them
    // in the refresh interval if they aren't set yet (perhaps due to no connection)
    if (!supportedPairsSet) {
      const pairs = await fetchCryptoDotComSupportedPairs()
      this.props.actions.setCryptoDotComSupportedPairs(pairs)
    }

    this.props.actions.onCryptoDotComRefreshData(tickerPrices, losersGainers, charts)
  }

  onBtcPriceOptIn = async () => {
    this.props.actions.onBtcPriceOptIn()
    this.props.actions.onCryptoDotComInteraction()
    await this.onCryptoDotComMarketsRequested(['BTC'])
  }

  onCryptoDotComBuyCrypto = () => {
    this.props.actions.onCryptoDotComBuyCrypto()
  }

  onCryptoDotComInteraction = () => {
    this.props.actions.onCryptoDotComInteraction()
  }

  onCryptoDotComOptInMarkets = (show: boolean) => {
    this.props.actions.onCryptoDotComOptInMarkets(show)
  }

  fetchGeminiBalances = () => {
    chrome.gemini.getAccountBalances((balances: Record<string, string>, authInvalid: boolean) => {
      if (authInvalid) {
        chrome.gemini.refreshAccessToken((success: boolean) => {
          if (!success) {
            this.setGeminiAuthInvalid()
          }
        })
        return
      }

      this.props.actions.setGeminiAccountBalances(balances)
    })
  }

  updateGeminiAssetAddress = (asset: string, address: string) => {
    generateQRData(address, asset, this.setGeminiAssetAddress.bind(this, address))
  }

  fetchGeminiDepositInfo = () => {
    // Remove all ercTokens and update their address when ETH's address is fetched.
    const toRemove = new Set(geminiData.ercTokens)
    const targetAssets = geminiData.currencies.filter(x => !toRemove.has(x))

    targetAssets.forEach((asset: string) => {
      chrome.gemini.getDepositInfo(`${asset.toLowerCase()}`, (address: string) => {
        if (address) {
          this.updateGeminiAssetAddress(asset, address)
        }
      })
    })
  }

  getCurrencyList = () => {
    const { accountBalances, userTLD } = this.props.newTabData.binanceState
    const { usCurrencies, comCurrencies } = currencyData
    const baseList = userTLD === 'us' ? usCurrencies : comCurrencies

    if (!accountBalances) {
      return baseList
    }

    const accounts = Object.keys(accountBalances)
    const nonHoldingList = baseList.filter((symbol: string) => {
      return !accounts.includes(symbol)
    })

    return accounts.concat(nonHoldingList)
  }

  getConvertAssets = () => {
    chrome.binance.getConvertAssets((assets: any) => {
      for (let asset in assets) {
        if (assets[asset]) {
          this.setConvertableAssets(asset, assets[asset])
        }
      }
    })
  }

  fetchBalance = () => {
    const { depositInfoSaved } = this.props.newTabData.binanceState

    chrome.binance.getAccountBalances((balances: Record<string, Record<string, string>>, success: boolean) => {
      const hasBalances = Object.keys(balances).length

      if (!hasBalances) {
        return
      } else if (!success) {
        this.setAuthInvalid()
        return
      }

      this.setBalanceInfo(balances)

      if (!depositInfoSaved) {
        this.setDepositInfo()
      }
    })
  }

  setDepositInfo = () => {
    chrome.binance.getCoinNetworks((networks: Record<string, string>) => {
      const currencies = this.getCurrencyList()
      for (let ticker in networks) {
        if (currencies.includes(ticker)) {
          chrome.binance.getDepositInfo(ticker, networks[ticker], async (address: string, tag: string) => {
            this.setAssetDepositInfo(ticker, address, tag)
            await generateQRData((tag || address), ticker, this.setAssetDepositQRCodeSrc)
          })
        }
      }
      if (Object.keys(networks).length) {
        this.props.actions.setDepositInfoSaved()
      }
    })
  }

  setAuthInvalid = () => {
    this.props.actions.setAuthInvalid(true)
    this.props.actions.disconnectBinance()
  }

  dismissAuthInvalid = () => {
    this.props.actions.setAuthInvalid(false)
  }

  dismissGeminiAuthInvalid = () => {
    this.props.actions.setGeminiAuthInvalid(false)
  }

  getCryptoContent () {
    if (this.props.newTabData.hideAllWidgets) {
      return null
    }
    const {
      widgetStackOrder,
      braveTalkSupported,
      showRewards,
      showBinance,
      showBraveTalk,
      showGemini,
      geminiSupported,
      showCryptoDotCom,
      cryptoDotComSupported,
      showFTX,
      ftxSupported,
      binanceSupported
    } = this.props.newTabData
    const lookup = {
      'rewards': {
        display: showRewards,
        render: this.renderRewardsWidget.bind(this)
      },
      'binance': {
        display: binanceSupported && showBinance,
        render: this.renderBinanceWidget.bind(this)
      },
      'braveTalk': {
        display: braveTalkSupported && showBraveTalk,
        render: this.renderBraveTalkWidget.bind(this)
      },
      'gemini': {
        display: showGemini && geminiSupported,
        render: this.renderGeminiWidget.bind(this)
      },
      'cryptoDotCom': {
        display: showCryptoDotCom && cryptoDotComSupported,
        render: this.renderCryptoDotComWidget.bind(this)
      },
      'ftx': {
        display: showFTX && ftxSupported,
        render: this.renderFTXWidget.bind(this)
      }
    }

    const widgetList = widgetStackOrder.filter((widget: NewTab.StackWidget) => {
      if (!lookup.hasOwnProperty(widget)) {
        return false
      }

      return lookup[widget].display
    })

    return (
      <>
        {widgetList.map((widget: NewTab.StackWidget, i: number) => {
          const isForeground = i === widgetList.length - 1
          return (
            <div key={`widget-${widget}`}>
              {lookup[widget].render(isForeground, i)}
            </div>
          )
        })}
      </>
    )
  }

  allWidgetsHidden = () => {
    const {
      braveTalkSupported,
      showRewards,
      showBinance,
      showBraveTalk,
      geminiSupported,
      showGemini,
      showCryptoDotCom,
      cryptoDotComSupported,
      showFTX,
      ftxSupported,
      binanceSupported,
      hideAllWidgets
    } = this.props.newTabData
    return hideAllWidgets || [
      showRewards,
      braveTalkSupported && showBraveTalk,
      binanceSupported && showBinance,
      geminiSupported && showGemini,
      cryptoDotComSupported && showCryptoDotCom,
      ftxSupported && showFTX
    ].every((widget: boolean) => !widget)
  }

  renderCryptoContent () {
    const { newTabData } = this.props
    const { widgetStackOrder } = newTabData
    const allWidgetsHidden = this.allWidgetsHidden()

    if (!widgetStackOrder.length) {
      return null
    }

    return (
      <Page.GridItemWidgetStack>
        {this.getCryptoContent()}
        {!allWidgetsHidden &&
          <EditCards onEditCards={this.openSettingsEditCards} />
        }
      </Page.GridItemWidgetStack>
    )
  }

  renderBrandedWallpaperNotification () {
    if (!GetShouldShowBrandedWallpaperNotification(this.props)) {
      return null
    }

    const { rewardsState } = this.props.newTabData
    if (!rewardsState.adsSupported) {
      return null
    }

    const onClose = () => { this.dismissBrandedWallpaperNotification(true) }
    const onEnableAds = () => { this.startRewards() }

    return (
      <Page.BrandedWallpaperNotification>
        <SponsoredImageTooltip
          adsEnabled={rewardsState.enabledAds}
          onEnableAds={onEnableAds}
          onClose={onClose}
        />
      </Page.BrandedWallpaperNotification>
    )
  }

  renderRewardsWidget (showContent: boolean, position: number) {
    const { rewardsState, showRewards, textDirection } = this.props.newTabData
    if (!showRewards) {
      return null
    }

    return (
      <Rewards
        {...rewardsState}
        widgetTitle={getLocale('rewardsWidgetBraveRewards')}
        onLearnMore={this.learnMoreRewards}
        menuPosition={'left'}
        isCrypto={true}
        paddingType={'none'}
        isCryptoTab={!showContent}
        isForeground={showContent}
        stackPosition={position}
        textDirection={textDirection}
        preventFocus={false}
        hideWidget={this.toggleShowRewards}
        showContent={showContent}
        onShowContent={this.setForegroundStackWidget.bind(this, 'rewards')}
        onStartRewards={this.startRewards}
        onDismissNotification={this.dismissNotification}
      />
    )
  }

  renderBraveTalkWidget (showContent: boolean, position: number) {
    const { newTabData } = this.props
    const { showBraveTalk, textDirection, braveTalkSupported } = newTabData

    if (!showBraveTalk || !braveTalkSupported) {
      return null
    }

    return (
      <BraveTalk
        isCrypto={true}
        paddingType={'none'}
        menuPosition={'left'}
        widgetTitle={getLocale('braveTalkWidgetTitle')}
        isForeground={showContent}
        stackPosition={position}
        textDirection={textDirection}
        hideWidget={this.toggleShowBraveTalk}
        showContent={showContent}
        onShowContent={this.setForegroundStackWidget.bind(this, 'braveTalk')}
      />
    )
  }

  renderBinanceWidget (showContent: boolean, position: number) {
    const { newTabData } = this.props
    const { binanceState, showBinance, textDirection, binanceSupported } = newTabData

    if (!showBinance || !binanceSupported) {
      return null
    }

    return (
      <Binance
        {...binanceState}
        isCrypto={true}
        paddingType={'none'}
        isCryptoTab={!showContent}
        menuPosition={'left'}
        widgetTitle={'Binance'}
        isForeground={showContent}
        stackPosition={position}
        textDirection={textDirection}
        preventFocus={false}
        hideWidget={this.toggleShowBinance}
        showContent={showContent}
        onSetHideBalance={this.setBinanceHideBalance}
        onBinanceClientUrl={this.onBinanceClientUrl}
        onConnectBinance={this.connectBinance}
        onDisconnectBinance={this.disconnectBinance}
        onCancelDisconnect={this.cancelBinanceDisconnect}
        onValidAuthCode={this.onValidBinanceAuthCode}
        onBuyCrypto={this.buyCrypto}
        onBinanceUserTLD={this.onBinanceUserTLD}
        onBinanceUserLocale={this.onBinanceUserLocale}
        onShowContent={this.setForegroundStackWidget.bind(this, 'binance')}
        onSetInitialAmount={this.setInitialAmount}
        onSetInitialAsset={this.setInitialAsset}
        onSetInitialFiat={this.setInitialFiat}
        onSetUserTLDAutoSet={this.setUserTLDAutoSet}
        onUpdateActions={this.binanceUpdateActions}
        onDismissAuthInvalid={this.dismissAuthInvalid}
        onSetSelectedView={this.setBinanceSelectedView}
        getCurrencyList={this.getCurrencyList}
        onLearnMore={this.learnMoreBinance}
        onRefreshData={binanceState.userAuthed ? this.binanceRefreshActions : undefined}
        onDisconnect={binanceState.userAuthed ? this.setBinanceDisconnectInProgress : undefined}
      />
    )
  }

  renderGeminiWidget (showContent: boolean, position: number) {
    const { newTabData } = this.props
    const { geminiState, showGemini, textDirection, geminiSupported } = newTabData

    if (!showGemini || !geminiSupported) {
      return null
    }

    return (
      <Gemini
        {...geminiState}
        isCrypto={true}
        paddingType={'none'}
        isCryptoTab={!showContent}
        menuPosition={'left'}
        widgetTitle={'Gemini'}
        isForeground={showContent}
        stackPosition={position}
        textDirection={textDirection}
        preventFocus={false}
        hideWidget={this.toggleShowGemini}
        showContent={showContent}
        onShowContent={this.setForegroundStackWidget.bind(this, 'gemini')}
        onDisableWidget={this.toggleShowGemini}
        onValidAuthCode={this.onValidGeminiAuthCode}
        onConnectGemini={this.connectGemini}
        onGeminiClientUrl={this.onGeminiClientUrl}
        onUpdateActions={this.geminiUpdateActions}
        onSetSelectedView={this.setGeminiSelectedView}
        onSetHideBalance={this.setGeminiHideBalance}
        onCancelDisconnect={this.cancelGeminiDisconnect}
        onDisconnectGemini={this.disconnectGemini}
        onDismissAuthInvalid={this.dismissGeminiAuthInvalid}
        onDisconnect={geminiState.userAuthed ? this.setGeminiDisconnectInProgress : undefined}
        onRefreshData={geminiState.userAuthed ? this.geminiUpdateActions : undefined}
      />
    )
  }

  renderCryptoDotComWidget (showContent: boolean, position: number) {
    const { newTabData } = this.props
    const { cryptoDotComState, showCryptoDotCom, textDirection, cryptoDotComSupported } = newTabData

    if (!showCryptoDotCom || !cryptoDotComSupported) {
      return null
    }

    return (
      <CryptoDotCom
        {...cryptoDotComState}
        isCrypto={true}
        paddingType={'none'}
        isCryptoTab={!showContent}
        menuPosition={'left'}
        widgetTitle={'Crypto.com'}
        isForeground={showContent}
        stackPosition={position}
        textDirection={textDirection}
        preventFocus={false}
        hideWidget={this.toggleShowCryptoDotCom}
        showContent={showContent}
        onShowContent={this.setForegroundStackWidget.bind(this, 'cryptoDotCom')}
        onViewMarketsRequested={this.onCryptoDotComMarketsRequested}
        onSetAssetData={this.onCryptoDotComAssetData}
        onUpdateActions={this.cryptoDotComUpdateActions}
        onDisableWidget={this.toggleShowCryptoDotCom}
        onBtcPriceOptIn={this.onBtcPriceOptIn}
        onBuyCrypto={this.onCryptoDotComBuyCrypto}
        onInteraction={this.onCryptoDotComInteraction}
        onOptInMarkets={this.onCryptoDotComOptInMarkets}
      />
    )
  }

  renderFTXWidget (showContent: boolean, position: number) {
    const { newTabData } = this.props
    const { showFTX, textDirection, ftxSupported } = newTabData

    if (!showFTX || !ftxSupported) {
      return null
    }

    return (
      <FTX
        ftx={this.props.ftx}
        actions={this.props.actions.ftx}
        isCrypto={true}
        paddingType={'none'}
        isCryptoTab={!showContent}
        menuPosition={'left'}
        widgetTitle={'FTX'}
        isForeground={showContent}
        stackPosition={position}
        textDirection={textDirection}
        preventFocus={false}
        hideWidget={this.toggleShowFTX}
        showContent={showContent}
        onShowContent={this.setForegroundStackWidget.bind(this, 'ftx')}
        onDisconnect={this.props.ftx.isConnected ? this.props.actions.ftx.disconnect : undefined}
      />
    )
  }

  render () {
    const { newTabData, gridSitesData, actions } = this.props
    const { showSettingsMenu, showEditTopSite, targetTopSiteForEditing } = this.state

    if (!newTabData) {
      return null
    }

    const hasImage = this.imageSource !== undefined
    const isShowingBrandedWallpaper = !!newTabData.brandedWallpaper
    const cryptoContent = this.renderCryptoContent()
    const showAddNewSiteMenuItem = newTabData.customLinksNum < MAX_GRID_SIZE

    let showTopSites = newTabData.showTopSites
    // In favorites mode, add site tile is visible by default if there is no
    // item. In frecency, top sites widget is hidden with empty tiles.
    if (showTopSites && !newTabData.customLinksEnabled) {
      showTopSites = this.props.gridSitesData.gridSites.length !== 0
    }

    return (
      <Page.App
        dataIsReady={newTabData.initialDataLoaded}
        hasImage={hasImage}
        imageSrc={this.imageSource}
        imageHasLoaded={this.state.backgroundHasLoaded}
      >
        <Page.Page
            hasImage={hasImage}
            imageSrc={this.imageSource}
            imageHasLoaded={this.state.backgroundHasLoaded}
            showClock={newTabData.showClock}
            showStats={newTabData.showStats}
            showRewards={!!cryptoContent}
            showBraveTalk={newTabData.showBraveTalk && newTabData.braveTalkSupported}
            showBinance={newTabData.showBinance}
            showTopSites={showTopSites}
            showBrandedWallpaper={isShowingBrandedWallpaper}
        >
          {newTabData.showStats &&
          <Page.GridItemStats>
            <Stats
              paddingType={'right'}
              widgetTitle={getLocale('statsTitle')}
              textDirection={newTabData.textDirection}
              stats={newTabData.stats}
              hideWidget={this.toggleShowStats}
              menuPosition={'right'}
            />
          </Page.GridItemStats>
          }
          {newTabData.showClock &&
          <Page.GridItemClock>
            <Clock
              paddingType={'right'}
              widgetTitle={getLocale('clockTitle')}
              textDirection={newTabData.textDirection}
              hideWidget={this.toggleShowClock}
              menuPosition={'left'}
              toggleClickFormat={this.toggleClockFormat}
              clockFormat={newTabData.clockFormat}
            />
          </Page.GridItemClock>
          }
          {
            showTopSites
              ? (
              <Page.GridItemTopSites>
                <TopSitesGrid
                  actions={actions}
                  paddingType={'none'}
                  customLinksEnabled={newTabData.customLinksEnabled}
                  onShowEditTopSite={this.showEditTopSite}
                  widgetTitle={getLocale('topSitesTitle')}
                  gridSites={gridSitesData.gridSites}
                  menuPosition={'right'}
                  hideWidget={this.toggleShowTopSites}
                  onAddSite={showAddNewSiteMenuItem ? this.showEditTopSite : undefined}
                  onToggleCustomLinksEnabled={this.toggleCustomLinksEnabled}
                  textDirection={newTabData.textDirection}
                />
              </Page.GridItemTopSites>
              ) : null
          }
          {
            gridSitesData.shouldShowSiteRemovedNotification
            ? (
            <Page.GridItemNotification>
              <SiteRemovalNotification actions={actions} showRestoreAll={!newTabData.customLinksEnabled} />
            </Page.GridItemNotification>
            ) : null
          }
            {cryptoContent}
          <Page.Footer>
            <Page.FooterContent>
            {isShowingBrandedWallpaper && newTabData.brandedWallpaper &&
            newTabData.brandedWallpaper.logo &&
            <Page.GridItemBrandedLogo>
              <BrandedWallpaperLogo
                menuPosition={'right'}
                paddingType={'default'}
                textDirection={newTabData.textDirection}
                onClickLogo={this.onClickLogo}
                data={newTabData.brandedWallpaper.logo}
              />
              {this.renderBrandedWallpaperNotification()}
            </Page.GridItemBrandedLogo>}
            <FooterInfo
              textDirection={newTabData.textDirection}
              supportsBraveTalk={newTabData.braveTalkSupported}
              showBraveTalkPrompt={newTabData.braveTalkPromptAllowed && !newTabData.braveTalkPromptDismissed}
              backgroundImageInfo={newTabData.backgroundWallpaper}
              showPhotoInfo={!isShowingBrandedWallpaper && newTabData.showBackgroundImage}
              onClickSettings={this.openSettings}
              onDismissBraveTalkPrompt={this.props.actions.dismissBraveTalkPrompt}
            />
            </Page.FooterContent>
          </Page.Footer>
          {newTabData.showToday &&
          <Page.GridItemNavigationBraveToday>
            <BraveTodayHint />
          </Page.GridItemNavigationBraveToday>
          }
        </Page.Page>
        { newTabData.showToday && newTabData.featureFlagBraveNewsEnabled &&
        <BraveToday
          feed={this.props.todayData.feed}
          articleToScrollTo={this.props.todayData.articleScrollTo}
          displayAdToScrollTo={this.props.todayData.displayAdToScrollTo}
          displayedPageCount={this.props.todayData.currentPageIndex}
          publishers={this.props.todayData.publishers}
          isFetching={this.props.todayData.isFetching === true}
          hasInteracted={this.props.todayData.hasInteracted}
          isUpdateAvailable={this.props.todayData.isUpdateAvailable}
          isOptedIn={this.props.newTabData.isBraveTodayOptedIn}
          onRefresh={this.props.actions.today.refresh}
          onAnotherPageNeeded={this.props.actions.today.anotherPageNeeded}
          onInteracting={this.onBraveTodayInteracting}
          onDisable={this.toggleShowToday}
          onFeedItemViewedCountChanged={this.props.actions.today.feedItemViewedCountChanged}
          onCustomizeBraveToday={() => { this.openSettings(SettingsTabType.BraveToday) }}
          onReadFeedItem={this.props.actions.today.readFeedItem}
          onPromotedItemViewed={this.props.actions.today.promotedItemViewed}
          onSetPublisherPref={this.props.actions.today.setPublisherPref}
          onCheckForUpdate={this.props.actions.today.checkForUpdate}
          onOptIn={this.props.actions.today.optIn}
          onViewedDisplayAd={this.props.actions.today.displayAdViewed}
          onVisitDisplayAd={this.props.actions.today.visitDisplayAd}
          getDisplayAd={this.props.getBraveNewsDisplayAd}
        />
        }
        <Settings
          actions={actions}
          textDirection={newTabData.textDirection}
          showSettingsMenu={showSettingsMenu}
          featureFlagBraveNewsEnabled={newTabData.featureFlagBraveNewsEnabled}
          onClose={this.closeSettings}
          setActiveTab={this.state.activeSettingsTab || undefined}
          onDisplayTodaySection={this.props.actions.today.ensureSettingsData}
          onClearTodayPrefs={this.props.actions.today.resetTodayPrefsToDefault}
          toggleShowBackgroundImage={this.toggleShowBackgroundImage}
          toggleShowClock={this.toggleShowClock}
          toggleShowStats={this.toggleShowStats}
          toggleShowToday={this.toggleShowToday}
          toggleShowTopSites={this.toggleShowTopSites}
          setMostVisitedSettings={this.setMostVisitedSettings}
          toggleBrandedWallpaperOptIn={this.toggleShowBrandedWallpaper}
          showBackgroundImage={newTabData.showBackgroundImage}
          showClock={newTabData.showClock}
          clockFormat={newTabData.clockFormat}
          showStats={newTabData.showStats}
          showToday={newTabData.showToday}
          showTopSites={newTabData.showTopSites}
          customLinksEnabled={newTabData.customLinksEnabled}
          showRewards={newTabData.showRewards}
          showBinance={newTabData.showBinance}
          brandedWallpaperOptIn={newTabData.brandedWallpaperOptIn}
          allowSponsoredWallpaperUI={newTabData.featureFlagBraveNTPSponsoredImagesWallpaper}
          toggleShowRewards={this.toggleShowRewards}
          toggleShowBinance={this.toggleShowBinance}
          binanceSupported={newTabData.binanceSupported}
          braveTalkSupported={newTabData.braveTalkSupported}
          toggleShowBraveTalk={this.toggleShowBraveTalk}
          showBraveTalk={newTabData.showBraveTalk}
          geminiSupported={newTabData.geminiSupported}
          toggleShowGemini={this.toggleShowGemini}
          showGemini={newTabData.showGemini}
          cryptoDotComSupported={newTabData.cryptoDotComSupported}
          toggleShowCryptoDotCom={this.toggleShowCryptoDotCom}
          showCryptoDotCom={newTabData.showCryptoDotCom}
          ftxSupported={newTabData.ftxSupported}
          toggleShowFTX={this.toggleShowFTX}
          showFTX={newTabData.showFTX}
          todayPublishers={this.props.todayData.publishers}
          cardsHidden={this.allWidgetsHidden()}
          toggleCards={this.props.saveSetAllStackWidgets}
        />
        {
          showEditTopSite
            ? <EditTopSite
              targetTopSiteForEditing={targetTopSiteForEditing}
              textDirection={newTabData.textDirection}
              onClose={this.closeEditTopSite}
              onSave={this.saveNewTopSite}
            /> : null
        }
      </Page.App>
    )
  }
}

export default NewTabPage
