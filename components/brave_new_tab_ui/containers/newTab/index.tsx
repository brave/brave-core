// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Components
import getNTPBrowserAPI from '../../api/background'
import { addNewTopSite, editTopSite } from '../../api/topSites'
import { brandedWallpaperLogoClicked } from '../../api/wallpaper'
import {
  BraveTalkWidget as BraveTalk, Clock, EditCards, EditTopSite, OverrideReadabilityColor, RewardsWidget as Rewards, SearchPromotion
} from '../../components/default'
import BrandedWallpaperLogo from '../../components/default/brandedWallpaper/logo'
import BraveToday, { GetDisplayAdContent } from '../../components/default/braveToday'
import FooterInfo from '../../components/default/footer/footer'
import * as Page from '../../components/default/page'
import { SponsoredImageTooltip } from '../../components/default/rewards'
import TopSitesGrid from './gridSites'
import SiteRemovalNotification from './notification'
import Stats from './stats'

// Helpers
import isReadableOnBackground from '../../helpers/colorUtil'
import VisibilityTimer from '../../helpers/visibilityTimer'

// Types
import { getLocale } from '../../../common/locale'
import { NewTabActions } from '../../constants/new_tab_types'
import { BraveTodayState } from '../../reducers/today'

// NTP features
import { MAX_GRID_SIZE } from '../../constants/new_tab_ui'
import Settings, { TabType as SettingsTabType } from './settings'

import { BraveNewsContextProvider } from '../../components/default/braveToday/customize/Context'
import BraveTodayHint from '../../components/default/braveToday/hint'
import GridWidget from './gridWidget'

interface Props {
  newTabData: NewTab.State
  gridSitesData: NewTab.GridSitesState
  todayData: BraveTodayState
  actions: NewTabActions
  getBraveNewsDisplayAd: GetDisplayAdContent
  saveShowBackgroundImage: (value: boolean) => void
  saveShowToday: (value: boolean) => any
  saveShowRewards: (value: boolean) => void
  saveShowBraveTalk: (value: boolean) => void
  saveBrandedWallpaperOptIn: (value: boolean) => void
  saveSetAllStackWidgets: (value: boolean) => void
  chooseNewCustomBackgroundImage: () => void
  setCustomImageBackground: (selectedBackground: string) => void
  removeCustomImageBackground: (background: string) => void
  setBraveBackground: (selectedBackground: string) => void
  setColorBackground: (color: string, useRandomColor: boolean) => void
}

interface State {
  showSettingsMenu: boolean
  showEditTopSite: boolean
  targetTopSiteForEditing?: NewTab.Site
  backgroundHasLoaded: boolean
  activeSettingsTab: SettingsTabType | null
  isPromptingBraveToday: boolean
  showSearchPromotion: boolean
  forceToHideWidget: boolean
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

  if (props.newTabData.backgroundWallpaper?.type === 'image' ||
      props.newTabData.backgroundWallpaper?.type === 'brave') {
    return props.newTabData.backgroundWallpaper.wallpaperImageUrl
  }

  return undefined
}

function GetShouldShowSearchPromotion (props: Props, showSearchPromotion: boolean) {
  if (GetIsShowingBrandedWallpaper(props)) { return false }

  return props.newTabData.searchPromotionEnabled && showSearchPromotion
}

function GetShouldForceToHideWidget (props: Props, showSearchPromotion: boolean) {
  if (!GetShouldShowSearchPromotion(props, showSearchPromotion)) {
    return false
  }

  // Avoid promotion popup and other widgets overlap with narrow window.
  return window.innerWidth < 1000
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
    activeSettingsTab: null,
    isPromptingBraveToday: false,
    showSearchPromotion: false,
    forceToHideWidget: false
  }

  braveNewsPromptTimerId: number
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
    // Only prompt for brave today if we're not scrolling down.
    const shouldPromptBraveToday =
      this.props.newTabData.featureFlagBraveNewsPromptEnabled &&
      this.props.newTabData.showToday &&
      !this.props.todayData.articleScrollTo
    if (shouldPromptBraveToday) {
      this.braveNewsPromptTimerId = window.setTimeout(() => {
        if (window.scrollY > 0) {
          // Don't do anything if user has already scrolled
          return
        }
        this.setState({ isPromptingBraveToday: true })
      }, 1700)
    }
    const searchPromotionEnabled = this.props.newTabData.searchPromotionEnabled
    this.setState({
      showSearchPromotion: searchPromotionEnabled,
      forceToHideWidget: GetShouldForceToHideWidget(this.props, searchPromotionEnabled)
    })
    window.addEventListener('resize', this.handleResize.bind(this))
  }

  componentWillUnmount () {
    if (this.braveNewsPromptTimerId) {
      window.clearTimeout(this.braveNewsPromptTimerId)
    }
    window.removeEventListener('resize', this.handleResize.bind(this))
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

  shouldOverrideReadabilityColor (newTabData: NewTab.State) {
    return !newTabData.brandedWallpaper && newTabData.backgroundWallpaper?.type === 'color' && !isReadableOnBackground(newTabData.backgroundWallpaper)
  }

  handleResize () {
    this.setState({
      forceToHideWidget: GetShouldForceToHideWidget(this.props, this.state.showSearchPromotion)
    })
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
    const openSettings = urlParams.get('openSettings') || this.props.newTabData.forceSettingsTab

    if (openSettings) {
      let activeSettingsTab: SettingsTabType | null = null
      const activeSettingsTabRaw = typeof openSettings === 'string'
        ? openSettings
        : this.props.newTabData.forceSettingsTab || null
      if (activeSettingsTabRaw) {
        const allSettingsTabTypes = [...Object.keys(SettingsTabType)]
        if (allSettingsTabTypes.includes(activeSettingsTabRaw)) {
          activeSettingsTab = SettingsTabType[activeSettingsTabRaw]
        }
      }
      this.setState({ showSettingsMenu: true, activeSettingsTab })
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

  disableBrandedWallpaper = () => {
    this.props.saveBrandedWallpaperOptIn(false)
  }

  toggleShowBrandedWallpaper = () => {
    this.props.saveBrandedWallpaperOptIn(
      !this.props.newTabData.brandedWallpaperOptIn
    )
  }

  startRewards = () => {
    const { rewardsState } = this.props.newTabData
    if (!rewardsState.rewardsEnabled) {
      chrome.braveRewards.openRewardsPanel()
    } else {
      chrome.braveRewards.enableAds()
    }
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

  closeSearchPromotion = () => {
    this.setState({
      showSearchPromotion: false,
      forceToHideWidget: false
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

  learnMoreRewards = () => {
    window.open('https://brave.com/brave-rewards/', '_blank', 'noopener')
  }

  getCryptoContent () {
    if (this.props.newTabData.hideAllWidgets) {
      return null
    }
    const {
      widgetStackOrder,
      braveRewardsSupported,
      braveTalkSupported,
      showRewards,
      showBraveTalk
    } = this.props.newTabData
    const lookup = {
      'rewards': {
        display: braveRewardsSupported && showRewards,
        render: this.renderRewardsWidget.bind(this)
      },
      'braveTalk': {
        display: braveTalkSupported && showBraveTalk,
        render: this.renderBraveTalkWidget.bind(this)
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
      braveRewardsSupported,
      braveTalkSupported,
      showRewards,
      showBraveTalk,
      hideAllWidgets
    } = this.props.newTabData
    return hideAllWidgets || [
      braveRewardsSupported && showRewards,
      braveTalkSupported && showBraveTalk
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

  renderSearchPromotion () {
    if (!GetShouldShowSearchPromotion(this.props, this.state.showSearchPromotion)) {
      return null
    }

    const onClose = () => { this.closeSearchPromotion() }
    const onDismiss = () => { getNTPBrowserAPI().pageHandler.dismissBraveSearchPromotion() }
    const onTryBraveSearch = (input: string, openNewTab: boolean) => { getNTPBrowserAPI().pageHandler.tryBraveSearchPromotion(input, openNewTab) }

    return (
      <SearchPromotion textDirection={this.props.newTabData.textDirection} onTryBraveSearch={onTryBraveSearch} onClose={onClose} onDismiss={onDismiss} />
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

    return (
      <Page.BrandedWallpaperNotification>
        <SponsoredImageTooltip
          adsEnabled={rewardsState.enabledAds}
          onEnableAds={this.startRewards}
          onClose={onClose}
        />
      </Page.BrandedWallpaperNotification>
    )
  }

  renderRewardsWidget (showContent: boolean, position: number) {
    const { rewardsState, showRewards, textDirection, braveRewardsSupported } = this.props.newTabData
    if (!braveRewardsSupported || !showRewards) {
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

  render () {
    const { newTabData, gridSitesData, actions } = this.props
    const { showSettingsMenu, showEditTopSite, targetTopSiteForEditing, forceToHideWidget } = this.state

    if (!newTabData) {
      return null
    }

    const hasImage = this.imageSource !== undefined
    const isShowingBrandedWallpaper = !!newTabData.brandedWallpaper

    const hasWallpaperInfo = newTabData.backgroundWallpaper?.type === 'brave'
    const colorForBackground = newTabData.backgroundWallpaper?.type === 'color' ? newTabData.backgroundWallpaper.wallpaperColor : undefined

    let cryptoContent = this.renderCryptoContent()
    const showAddNewSiteMenuItem = newTabData.customLinksNum < MAX_GRID_SIZE

    let { showTopSites, showStats, showClock } = newTabData
    // In favorites mode, add site tile is visible by default if there is no
    // item. In frecency, top sites widget is hidden with empty tiles.
    if (showTopSites && !newTabData.customLinksEnabled) {
      showTopSites = this.props.gridSitesData.gridSites.length !== 0
    }

    if (forceToHideWidget) {
      showTopSites = false
      showStats = false
      showClock = false
      cryptoContent = null
    }

    const BraveNewsContext = newTabData.featureFlagBraveNewsV2Enabled
      ? BraveNewsContextProvider
      : React.Fragment

    return (
      <Page.App
        dataIsReady={newTabData.initialDataLoaded}
        hasImage={hasImage}
        imageSrc={this.imageSource}
        imageHasLoaded={this.state.backgroundHasLoaded}
        colorForBackground={colorForBackground}
        data-show-news-prompt={((this.state.backgroundHasLoaded || colorForBackground) && this.state.isPromptingBraveToday) ? true : undefined}>
        <OverrideReadabilityColor override={ this.shouldOverrideReadabilityColor(this.props.newTabData) } />
        <BraveNewsContext>
        <Page.Page
            hasImage={hasImage}
            imageSrc={this.imageSource}
            imageHasLoaded={this.state.backgroundHasLoaded}
            showClock={showClock}
            showStats={showStats}
            colorForBackground={colorForBackground}
            showCryptoContent={!!cryptoContent}
            showTopSites={showTopSites}
            showBrandedWallpaper={isShowingBrandedWallpaper}
        >
          {this.renderSearchPromotion()}
          <GridWidget
            pref='showStats'
            container={Page.GridItemStats}
            paddingType={'right'}
            widgetTitle={getLocale('statsTitle')}
            textDirection={newTabData.textDirection}
            menuPosition={'right'}>
            <Stats stats={newTabData.stats}/>
          </GridWidget>
          <GridWidget
            pref='showClock'
            container={Page.GridItemClock}
            paddingType='right'
            widgetTitle={getLocale('clockTitle')}
            textDirection={newTabData.textDirection}
            menuPosition='left'>
            <Clock />
          </GridWidget>
          {
            showTopSites &&
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
                  showPhotoInfo={!isShowingBrandedWallpaper && hasWallpaperInfo && newTabData.showBackgroundImage}
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
          isPrompting={this.state.isPromptingBraveToday}
          isUpdateAvailable={this.props.todayData.isUpdateAvailable}
          isOptedIn={this.props.newTabData.isBraveTodayOptedIn}
          onRefresh={this.props.actions.today.refresh}
          onAnotherPageNeeded={this.props.actions.today.anotherPageNeeded}
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
          featureCustomBackgroundEnabled={newTabData.featureCustomBackgroundEnabled}
          onClose={this.closeSettings}
          setActiveTab={this.state.activeSettingsTab || undefined}
          onDisplayTodaySection={this.props.actions.today.ensureSettingsData}
          onClearTodayPrefs={this.props.actions.today.resetTodayPrefsToDefault}
          toggleShowBackgroundImage={this.toggleShowBackgroundImage}
          toggleShowToday={this.toggleShowToday}
          toggleShowTopSites={this.toggleShowTopSites}
          setMostVisitedSettings={this.setMostVisitedSettings}
          toggleBrandedWallpaperOptIn={this.toggleShowBrandedWallpaper}
          chooseNewCustomImageBackground={this.props.chooseNewCustomBackgroundImage}
          setCustomImageBackground={this.props.setCustomImageBackground}
          removeCustomImageBackground={this.props.removeCustomImageBackground}
          setBraveBackground={this.props.setBraveBackground}
          setColorBackground={this.props.setColorBackground}
          showBackgroundImage={newTabData.showBackgroundImage}
          showToday={newTabData.showToday}
          showTopSites={newTabData.showTopSites}
          customLinksEnabled={newTabData.customLinksEnabled}
          showRewards={newTabData.showRewards}
          braveRewardsSupported={newTabData.braveRewardsSupported}
          brandedWallpaperOptIn={newTabData.brandedWallpaperOptIn}
          allowSponsoredWallpaperUI={newTabData.featureFlagBraveNTPSponsoredImagesWallpaper}
          toggleShowRewards={this.toggleShowRewards}
          braveTalkSupported={newTabData.braveTalkSupported}
          toggleShowBraveTalk={this.toggleShowBraveTalk}
          showBraveTalk={newTabData.showBraveTalk}
          todayPublishers={this.props.todayData.publishers}
          cardsHidden={this.allWidgetsHidden()}
          toggleCards={this.props.saveSetAllStackWidgets}
          newTabData={this.props.newTabData}
          onEnableRewards={this.startRewards}
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
        </BraveNewsContext>
      </Page.App>
    )
  }
}

export default NewTabPage
