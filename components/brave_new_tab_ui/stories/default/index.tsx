/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

// Feature-specific components
import { ClockWidget as Clock, RewardsWidget as Rewards } from '../../components/default'
import * as Page from '../../components/default/page'
import BrandedWallpaperLogo from '../../components/default/brandedWallpaper/logo'

import TopSitesList from './topSites/topSitesList'
import Stats from './stats'
import TopSitesNotification from '../../containers/newTab/notification'
import FooterInfo from './footerInfo'

// Assets
import { getRandomBackgroundData } from './helpers'
import { images } from './data/background'
import dummyBrandedWallpaper from './data/brandedWallpaper'

const generateRandomBackgroundData = getRandomBackgroundData(images)
interface State {
  showSettingsMenu: boolean
  showBackgroundImage: boolean
  showStats: boolean
  showClock: boolean
  showTopSites: boolean
  showTopSitesNotification: boolean
  showRewards: boolean
  brandedWallpaperOptIn: boolean
  adsEstimatedEarnings: number
  balance: NewTab.RewardsBalance
  promotions: NewTab.Promotion[]
  enabledAds: boolean
  enabledMain: boolean
  totalContribution: number
  walletCreated: boolean
  walletCreating: boolean
  walletCreateFailed: boolean
  walletCorrupted: boolean,
  isBrandedWallpaperNotificationDismissed: boolean
  brandedWallpaper?: NewTab.BrandedWallpaper
}

interface Props {
  textDirection: string
  showBrandedWallpaper: boolean
  showTopSitesNotification: boolean
  isAdsOn: boolean
  isAdsSupported: boolean
}

export default class NewTabPage extends React.PureComponent<Props, State> {
  constructor (props: Props) {
    super(props)
    this.state = {
      showSettingsMenu: false,
      showBackgroundImage: true,
      showStats: true,
      showClock: true,
      showTopSites: true,
      showTopSitesNotification: props.showTopSitesNotification,
      showRewards: true,
      brandedWallpaperOptIn: true,
      adsEstimatedEarnings: 5,
      enabledAds: false,
      enabledMain: false,
      promotions: [],
      balance: {
        total: 0,
        rates: {},
        wallets: {}
      },
      totalContribution: 0.0,
      walletCreated: false,
      walletCreating: false,
      walletCreateFailed: false,
      walletCorrupted: false,
      isBrandedWallpaperNotificationDismissed: false
    }
  }

  doNothing = (s: string) => {
    console.log('doNothing called:', s)
  }

  toggleShowBackgroundImage = () => {
    this.setState({ showBackgroundImage: !this.state.showBackgroundImage })
  }

  toggleShowClock = () => {
    this.setState({ showClock: !this.state.showClock })
  }

  toggleShowStats = () => {
    this.setState({ showStats: !this.state.showStats })
  }

  toggleShowTopSites = () => {
    this.setState({ showTopSites: !this.state.showTopSites })
  }

  toggleShowRewards = () => {
    this.setState({ showRewards: !this.state.showRewards })
  }

  closeSettings = () => {
    this.setState({ showSettingsMenu: false })
  }

  toggleSettings = () => {
    this.setState({ showSettingsMenu: !this.state.showSettingsMenu })
  }

  enableAds = () => {
    this.setState({ enabledAds: true })
  }

  enableRewards = () => {
    this.setState({ enabledMain: true })
  }

  toggleBrandedWallpaperOptIn = () => {
    this.setState({ brandedWallpaperOptIn: !this.state.brandedWallpaperOptIn })
  }

  hideSiteRemovalNotification = () => {
    this.setState({ showTopSitesNotification: false })
  }

  onDismissBrandedWallpaperNotification = () => {
    this.setState({ isBrandedWallpaperNotificationDismissed: true })
  }

  createWallet = () => {
    this.setState({ walletCreating: true })
    setTimeout(() => {
      this.setState({ walletCreated: true })
      this.enableAds()
      this.enableRewards()
    }, 1000)
  }

  componentDidUpdate (prevProps: Props) {
    if (!prevProps.showBrandedWallpaper && this.props.showBrandedWallpaper) {
      const brandedWallpaper = dummyBrandedWallpaper
      this.setState({ brandedWallpaper })
    }
    if (prevProps.showTopSitesNotification !== this.props.showTopSitesNotification) {
      this.setState({ showTopSitesNotification: this.props.showTopSitesNotification })
    }
  }

  render () {
    const { showSettingsMenu, showBackgroundImage, showClock, showStats, showTopSites, showRewards } = this.state
    const {
      enabledMain,
      adsEstimatedEarnings,
      walletCorrupted,
      walletCreateFailed,
      walletCreated,
      walletCreating,
      promotions,
      balance,
      totalContribution
    } = this.state
    const { textDirection } = this.props

    const enabledAds = this.state.enabledAds && this.props.isAdsOn

    const hasImage = showBackgroundImage
    let imageSource
    if (hasImage) {
      imageSource = this.state.brandedWallpaper ? this.state.brandedWallpaper.wallpaperImageUrl : generateRandomBackgroundData.source
    }

    const showBrandedWallpaper = this.props.showBrandedWallpaper &&
      this.state.brandedWallpaperOptIn

    return (
      <Page.App dataIsReady={true}>
        <Page.PosterBackground
          hasImage={hasImage}
          imageHasLoaded={true}
        >
          {hasImage &&
            <img src={imageSource} />
          }
        </Page.PosterBackground>
        {hasImage &&
          <Page.Gradient
            imageHasLoaded={true}
          />
        }
        <Page.Page
          showClock={showClock}
          showStats={showStats}
          showRewards={showRewards || showBrandedWallpaper}
          showTopSites={showTopSites}
          showBrandedWallpaper={showBrandedWallpaper}
        >
          {showStats &&
          <Page.GridItemStats>
            <Stats
              textDirection={textDirection}
              menuPosition={'right'}
              hideWidget={this.toggleShowStats}
            />
          </Page.GridItemStats>
          }
          {showClock &&
          <Page.GridItemClock>
            <Clock
              textDirection={textDirection}
              menuPosition={'left'}
              hideWidget={this.toggleShowClock}
            />
          </Page.GridItemClock>
          }
          {showTopSites &&
          <Page.GridItemTopSites>
            <TopSitesList
              textDirection={textDirection}
              menuPosition={'right'}
              hideWidget={this.toggleShowTopSites}
            />
          </Page.GridItemTopSites>
          }
          {this.state.showTopSitesNotification &&
          <Page.GridItemNotification>
            <TopSitesNotification
              actions={{
                undoSiteIgnored: () => { console.log('undo site ignored') },
                onUndoAllSiteIgnored: () => { console.log('undo all site ignored') },
                onHideSiteRemovalNotification: () => this.hideSiteRemovalNotification()
              }}
            />
          </Page.GridItemNotification>
          }
          {(showRewards || (showBrandedWallpaper && !this.state.isBrandedWallpaperNotificationDismissed)) &&
          <Page.GridItemRewards>
            <Rewards
              adsSupported={this.props.isAdsSupported}
              promotions={promotions}
              balance={balance}
              enabledAds={enabledAds}
              enabledMain={enabledMain}
              walletCreated={walletCreated}
              walletCorrupted={walletCorrupted}
              walletCreateFailed={walletCreateFailed}
              walletCreating={walletCreating}
              adsEstimatedEarnings={adsEstimatedEarnings}
              onEnableAds={this.enableAds}
              onCreateWallet={this.createWallet}
              onEnableRewards={this.enableRewards}
              textDirection={textDirection}
              menuPosition={'left'}
              hideWidget={this.toggleShowRewards}
              onDismissNotification={this.doNothing}
              onDismissBrandedWallpaperNotification={this.onDismissBrandedWallpaperNotification}
              totalContribution={totalContribution}
              showBrandedWallpaperNotification={!this.state.isBrandedWallpaperNotificationDismissed}
              brandedWallpaperData={this.state.brandedWallpaper}
              isShowingBrandedWallpaper={showBrandedWallpaper}
              isNotification={!showRewards}
              preventFocus={!showRewards}
              onDisableBrandedWallpaper={this.doNothing.bind(this, 'onDisableBrandedWallpaper')}
            />
          </Page.GridItemRewards>
          }
          <Page.Footer>
            {showBrandedWallpaper && this.state.brandedWallpaper &&
              this.state.brandedWallpaper.logo &&
            <Page.GridItemCredits>
              <BrandedWallpaperLogo
                menuPosition={'right'}
                textDirection={textDirection}
                data={this.state.brandedWallpaper.logo}
              />
            </Page.GridItemCredits>
            }
            <FooterInfo
              textDirection={textDirection}
              onClickOutside={this.closeSettings}
              backgroundImageInfo={generateRandomBackgroundData}
              onClickSettings={this.toggleSettings}
              showSettingsMenu={showSettingsMenu}
              showPhotoInfo={!showBrandedWallpaper && showBackgroundImage}
              toggleShowBackgroundImage={this.toggleShowBackgroundImage}
              toggleShowClock={this.toggleShowClock}
              toggleShowStats={this.toggleShowStats}
              toggleShowTopSites={this.toggleShowTopSites}
              toggleShowRewards={this.toggleShowRewards}
              toggleBrandedWallpaperOptIn={this.toggleBrandedWallpaperOptIn}
              showBackgroundImage={showBackgroundImage}
              showClock={showClock}
              showStats={showStats}
              showTopSites={showTopSites}
              showRewards={showRewards}
              brandedWallpaperOptIn={this.state.brandedWallpaperOptIn}
            />
          </Page.Footer>
        </Page.Page>
      </Page.App>
    )
  }
}
