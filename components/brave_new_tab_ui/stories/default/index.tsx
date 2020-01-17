/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

// Feature-specific components
import { Page, App, PosterBackground, Gradient, ClockWidget as Clock, RewardsWidget as Rewards } from '../../components/default'
import * as S from '../../components/default/page'
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
  brandedWallpaper?: NewTab.BrandedWallpaper
}

interface Props {
  textDirection: string
  showBrandedWallpaper: boolean
  showTopSitesNotification: boolean
  isAdsOn: boolean
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
      walletCorrupted: false
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
      <App dataIsReady={true}>
        <PosterBackground
          hasImage={hasImage}
          imageHasLoaded={true}
        >
          {hasImage &&
            <img src={imageSource} />
          }
        </PosterBackground>
        {hasImage &&
          <Gradient
            imageHasLoaded={true}
          />
        }
        <Page>
          {showStats &&
          <S.GridItemStats>
            <Stats
              textDirection={textDirection}
              menuPosition={'right'}
              hideWidget={this.toggleShowStats}
            />
          </S.GridItemStats>
          }
          {showClock &&
          <S.GridItemClock>
            <Clock
              textDirection={textDirection}
              menuPosition={'left'}
              hideWidget={this.toggleShowClock}
            />
          </S.GridItemClock>
          }
          {showTopSites &&
          <S.GridItemTopSites>
            <TopSitesList
              textDirection={textDirection}
              menuPosition={'right'}
              hideWidget={this.toggleShowTopSites}
            />
          </S.GridItemTopSites>
          }
          {this.state.showTopSitesNotification &&
          <S.GridItemNotification>
            <TopSitesNotification
              actions={{
                undoSiteIgnored: () => { console.log('undo site ignored') },
                onUndoAllSiteIgnored: () => { console.log('undo all site ignored') },
                onHideSiteRemovalNotification: () => this.hideSiteRemovalNotification()
              }}
            />
          </S.GridItemNotification>
          }
          {(showRewards || showBrandedWallpaper) &&
          <S.GridItemRewards>
            <Rewards
              adsSupported={true}
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
              totalContribution={totalContribution}
              showBrandedWallpaperNotification={showBrandedWallpaper}
              brandedWallpaperData={this.state.brandedWallpaper}
              isShowingBrandedWallpaper={showBrandedWallpaper}
              isNotification={!showRewards}
              preventFocus={!showRewards}
              onDisableBrandedWallpaper={this.doNothing.bind(this, 'onDisableBrandedWallpaper')}
            />
          </S.GridItemRewards>
          }
          <S.Footer>
            {showBrandedWallpaper && this.state.brandedWallpaper &&
              this.state.brandedWallpaper.logo &&
            <S.GridItemCredits>
              <BrandedWallpaperLogo
                menuPosition={'right'}
                textDirection={textDirection}
                data={this.state.brandedWallpaper.logo}
              />
            </S.GridItemCredits>
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
          </S.Footer>
        </Page>
      </App>
    )
  }
}
