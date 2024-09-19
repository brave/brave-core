// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Components
import getNTPBrowserAPI from '../../api/background'
import { addNewTopSite, editTopSite } from '../../api/topSites'
import { brandedWallpaperLogoClicked } from '../../api/wallpaper'
import {
  BraveTalkWidget as BraveTalk, Clock, EditTopSite, OverrideReadabilityColor, RewardsWidget as Rewards, SearchPromotion
} from '../../components/default'
import BrandedWallpaperLogo from '../../components/default/brandedWallpaper/logo'
import BraveNews, { GetDisplayAdContent } from '../../components/default/braveNews'
import FooterInfo from '../../components/default/footer/footer'
import * as Page from '../../components/default/page'
import TopSitesGrid from './gridSites'
import SiteRemovalNotification from './notification'
import Stats from './stats'

// Helpers
import { getLocale } from '$web-common/locale'
import VisibilityTimer from '$web-common/visibilityTimer'
import { loadTimeData } from '$web-common/loadTimeData'
import isReadableOnBackground from '../../helpers/colorUtil'

// Types
import { NewTabActions } from '../../constants/new_tab_types'
import { BraveNewsState } from '../../reducers/today'

// NTP features
import { MAX_GRID_SIZE } from '../../constants/new_tab_ui'
import Settings, { TabType as SettingsTabType } from './settings'

import { BraveNewsContextProvider } from '../../../brave_news/browser/resources/shared/Context'
import BraveNewsModal from '../../components/default/braveNews/customize/Modal'
import BraveNewsHint from '../../components/default/braveNews/hint'
import SponsoredImageClickArea from '../../components/default/sponsoredImage/sponsoredImageClickArea'
import GridWidget from './gridWidget'

import Icon from '@brave/leo/react/icon'

import * as style from './style'
import { defaultState } from '../../storage/new_tab_storage'

const BraveNewsPeek =  React.lazy(() => import('../../../brave_news/browser/resources/Peek'))
const SearchPlaceholder = React.lazy(() => import('../../components/search/SearchPlaceholder'))

interface Props {
  newTabData: NewTab.State
  gridSitesData: NewTab.GridSitesState
  todayData: BraveNewsState
  actions: NewTabActions
  getBraveNewsDisplayAd: GetDisplayAdContent
  saveShowBackgroundImage: (value: boolean) => void
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
  isPromptingBraveNews: boolean
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
    isPromptingBraveNews: false,
    showSearchPromotion: false,
    forceToHideWidget: false
  }

  imgCache: HTMLImageElement
  braveNewsPromptTimerId: number
  hasInitBraveNews: boolean = false
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
    const searchPromotionEnabled = this.props.newTabData.searchPromotionEnabled
    this.setState({
      showSearchPromotion: searchPromotionEnabled,
      forceToHideWidget: GetShouldForceToHideWidget(this.props, searchPromotionEnabled)
    })
    window.addEventListener('resize', this.handleResize)
    window.navigation.addEventListener('currententrychange', this.checkShouldOpenSettings)
  }

  componentWillUnmount () {
    if (this.braveNewsPromptTimerId) {
      window.clearTimeout(this.braveNewsPromptTimerId)
    }
    window.removeEventListener('resize', this.handleResize)
    window.navigation.removeEventListener('currententrychange', this.checkShouldOpenSettings)
  }

  componentDidUpdate (prevProps: Props) {
    this.maybePeekBraveNews()
    const oldImageSource = GetBackgroundImageSrc(prevProps)
    const newImageSource = GetBackgroundImageSrc(this.props)
    this.imageSource = newImageSource
    if (newImageSource && oldImageSource !== newImageSource) {
      this.trackCachedImage()
    }
    if (oldImageSource &&
      !newImageSource) {
      // reset loaded state
      console.debug('reset image loaded state due to removing image source')
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

  maybePeekBraveNews () {
    const hasPromptedBraveNews = !!this.braveNewsPromptTimerId
    const shouldPromptBraveNews =
      !hasPromptedBraveNews && // Don't start a prompt if we already did
      window.scrollY === 0 && // Don't start a prompt if we are scrolled
      this.props.newTabData.featureFlagBraveNewsPromptEnabled &&
      this.props.newTabData.initialDataLoaded && // Wait for accurate showToday
      this.props.newTabData.showToday &&
      // Don't prompt if the user has navigated back and we're going to scroll
      // down to a previous place in the feed.
      !this.props.todayData.articleScrollTo
    if (shouldPromptBraveNews) {
      this.braveNewsPromptTimerId = window.setTimeout(() => {
        if (window.scrollY > 0) {
          // If the user happens to start scrolling whilst waiting for the timer,
          // make sure we cancel the timer otherwise content will shift and provide
          // a poor UX.
          return
        }
        this.setState({ isPromptingBraveNews: true })
      }, 1700)
    }
  }

  shouldOverrideReadabilityColor (newTabData: NewTab.State) {
    return !newTabData.brandedWallpaper && newTabData.backgroundWallpaper?.type === 'color' && !isReadableOnBackground(newTabData.backgroundWallpaper)
  }

  handleResize = () => {
    this.setState({
      forceToHideWidget: GetShouldForceToHideWidget(this.props, this.state.showSearchPromotion)
    })
  }

  trackCachedImage () {
    console.debug('trackCachedImage')
    if (this.state.backgroundHasLoaded) {
      console.debug('Resetting to new image')
      this.setState({ backgroundHasLoaded: false })
    }
    if (this.imageSource) {
      const imgCache = new Image()
      // Store Image in class so it doesn't go out of scope
      this.imgCache = imgCache
      imgCache.src = this.imageSource
      console.debug('image start loading...')
      imgCache.addEventListener('load', () => {
        console.debug('image loaded')
        this.setState({
          backgroundHasLoaded: true
        })
      })
      imgCache.addEventListener('error', (e) => {
        console.debug('image error', e)
      })
    }
  }

  trackBrandedWallpaperNotificationAutoDismiss () {
    // Wait until page has been visible for an uninterupted Y seconds and then
    // dismiss the notification.
    this.visibilityTimer.startTracking()
  }

  checkShouldOpenSettings = () => {
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
    chrome.braveRewards.openRewardsPanel()
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

    if (!widgetStackOrder.length) {
      return null
    }

    return (
      <Page.GridItemWidgetStack>
        {this.getCryptoContent()}
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

    // Previously the NTP would show a Rewards tooltip on top of a sponsored
    // image under certain conditions. We no longer show that tooltip, and there
    // are currently no other "branded wallpaper notifications" defined.
    return null
  }

  renderRewardsWidget (showContent: boolean, position: number) {
    const { rewardsState, showRewards, textDirection, braveRewardsSupported } = this.props.newTabData
    if (!braveRewardsSupported || !showRewards) {
      return null
    }

    const customMenuItems = [
      {
        label: 'rewardsOpenPanel',
        renderIcon: () => {
          return (
            <style.rewardsMenuIcon>
              <Icon name='product-bat-outline' />
            </style.rewardsMenuIcon>
          )
        },
        onClick: () => { chrome.braveRewards.openRewardsPanel() }
      },
      {
        label: 'rewardsSettings',
        renderIcon: () => {
          return (
            <style.rewardsMenuIcon>
              <Icon name='settings' />
            </style.rewardsMenuIcon>
          )
        },
        onClick: () => { window.open('chrome://rewards', '_blank', 'noopener') }
      }
    ]

    const onSelfCustodyInviteDismissed = () => {
      chrome.braveRewards.dismissSelfCustodyInvite()
    }

    const onTosUpdateAccepted = () => {
      chrome.braveRewards.acceptTermsOfServiceUpdate()
    }

    return (
      <Rewards
        {...rewardsState}
        widgetTitle={getLocale('rewardsWidgetBraveRewards')}
        onLearnMore={this.learnMoreRewards}
        menuPosition={'left'}
        isCardWidget
        paddingType={'none'}
        isForeground={showContent}
        stackPosition={position}
        textDirection={textDirection}
        preventFocus={false}
        hideWidget={this.toggleShowRewards}
        showContent={showContent}
        onShowContent={this.setForegroundStackWidget.bind(this, 'rewards')}
        onDismissNotification={this.dismissNotification}
        customMenuItems={customMenuItems}
        onSelfCustodyInviteDismissed={onSelfCustodyInviteDismissed}
        onTermsOfServiceUpdateAccepted={onTosUpdateAccepted}
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
        isCardWidget
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

    // Allow background customization if Super Referrals is not activated.
    const isSuperReferral = newTabData.brandedWallpaper && !newTabData.brandedWallpaper.isSponsored
    const allowBackgroundCustomization = !isSuperReferral

    if (forceToHideWidget) {
      showTopSites = false
      showStats = false
      showClock = false
      cryptoContent = null
    }

    return (
      <Page.App
        dataIsReady={newTabData.initialDataLoaded}
        hasImage={hasImage}
        imageSrc={this.imageSource}
        imageHasLoaded={this.state.backgroundHasLoaded}
        colorForBackground={colorForBackground}
        data-show-news-prompt={((this.state.backgroundHasLoaded || colorForBackground) && this.state.isPromptingBraveNews && !defaultState.featureFlagBraveNewsFeedV2Enabled) ? true : undefined}>
        <OverrideReadabilityColor override={ this.shouldOverrideReadabilityColor(this.props.newTabData) } />
        <BraveNewsContextProvider>
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
            {newTabData.brandedWallpaper?.isSponsored && <Page.GridItemSponsoredImageClickArea otherWidgetsHidden={this.allWidgetsHidden()}>
              <SponsoredImageClickArea onClick={this.onClickLogo}
                sponsoredImageUrl={newTabData.brandedWallpaper.logo.destinationUrl}/>
              </Page.GridItemSponsoredImageClickArea>}
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
                  backgroundImageInfo={newTabData.backgroundWallpaper}
                  showPhotoInfo={!isShowingBrandedWallpaper && hasWallpaperInfo && newTabData.showBackgroundImage}
                  onClickSettings={this.openSettings}
                />
              </Page.FooterContent>
            </Page.Footer>
              <Page.GridItemPageFooter>
                {loadTimeData.getBoolean('featureFlagSearchWidget')
                  && <React.Suspense fallback={null}>
                    <SearchPlaceholder />
                  </React.Suspense>}
                {newTabData.showToday && (defaultState.featureFlagBraveNewsFeedV2Enabled
                  ? <React.Suspense fallback={null}>
                    <BraveNewsPeek/>
                  </React.Suspense>
                  : <BraveNewsHint />)}
              </Page.GridItemPageFooter>
          </Page.Page>
        { newTabData.showToday &&
        <BraveNews
          feed={this.props.todayData.feed}
          articleToScrollTo={this.props.todayData.articleScrollTo}
          displayAdToScrollTo={this.props.todayData.displayAdToScrollTo}
          displayedPageCount={this.props.todayData.currentPageIndex}
          publishers={this.props.todayData.publishers}
          isFetching={this.props.todayData.isFetching === true}
          hasInteracted={this.props.todayData.hasInteracted}
          isPrompting={this.state.isPromptingBraveNews}
          isUpdateAvailable={this.props.todayData.isUpdateAvailable}
          onRefresh={this.props.actions.today.refresh}
          onAnotherPageNeeded={this.props.actions.today.anotherPageNeeded}
          onFeedItemViewedCountChanged={this.props.actions.today.feedItemViewedCountChanged}
          onCustomizeBraveNews={() => { this.openSettings(SettingsTabType.BraveNews) }}
          onReadFeedItem={this.props.actions.today.readFeedItem}
          onPromotedItemViewed={this.props.actions.today.promotedItemViewed}
          onSetPublisherPref={this.props.actions.today.setPublisherPref}
          onCheckForUpdate={this.props.actions.today.checkForUpdate}
          onViewedDisplayAd={this.props.actions.today.displayAdViewed}
          onVisitDisplayAd={this.props.actions.today.visitDisplayAd}
          getDisplayAd={this.props.getBraveNewsDisplayAd}
        />
        }
        <Settings
          textDirection={newTabData.textDirection}
          showSettingsMenu={showSettingsMenu}
          featureCustomBackgroundEnabled={newTabData.featureCustomBackgroundEnabled}
          onClose={this.closeSettings}
          setActiveTab={this.state.activeSettingsTab || undefined}
          toggleShowBackgroundImage={this.toggleShowBackgroundImage}
          toggleShowTopSites={this.toggleShowTopSites}
          setMostVisitedSettings={this.setMostVisitedSettings}
          toggleBrandedWallpaperOptIn={this.toggleShowBrandedWallpaper}
          chooseNewCustomImageBackground={this.props.chooseNewCustomBackgroundImage}
          setCustomImageBackground={this.props.setCustomImageBackground}
          removeCustomImageBackground={this.props.removeCustomImageBackground}
          setBraveBackground={this.props.setBraveBackground}
          setColorBackground={this.props.setColorBackground}
          showBackgroundImage={newTabData.showBackgroundImage}
          showTopSites={newTabData.showTopSites}
          customLinksEnabled={newTabData.customLinksEnabled}
          showRewards={newTabData.showRewards}
          braveRewardsSupported={newTabData.braveRewardsSupported}
          brandedWallpaperOptIn={newTabData.brandedWallpaperOptIn}
          allowBackgroundCustomization={allowBackgroundCustomization}
          toggleShowRewards={this.toggleShowRewards}
          braveTalkSupported={newTabData.braveTalkSupported}
          toggleShowBraveTalk={this.toggleShowBraveTalk}
          showBraveTalk={newTabData.showBraveTalk}
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
        <BraveNewsModal/>
        </BraveNewsContextProvider>
      </Page.App>
    )
  }
}

export default NewTabPage
