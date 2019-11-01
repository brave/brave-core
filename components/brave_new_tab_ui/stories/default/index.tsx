/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

// Feature-specific components
import { Page, Header, Footer, App, PosterBackground, Gradient, ClockWidget as Clock, RewardsWidget as Rewards } from '../../components/default'

import TopSitesList from './topSites/topSitesList'
import Stats from './stats'
import SiteRemovalNotification from './siteRemovalNotification'
import FooterInfo from './footerInfo'

// Assets
import { getRandomBackgroundData } from './helpers'
import { images } from './data/background'

const generateRandomBackgroundData = getRandomBackgroundData(images)
interface State {
  showSettingsMenu: boolean
  showBackgroundImage: boolean
  showStats: boolean
  showClock: boolean
  showTopSites: boolean
  showRewards: boolean
  adsEstimatedEarnings: number
  balance: NewTab.RewardsBalance
  promotions: NewTab.Promotion[]
  enabledAds: boolean
  enabledMain: boolean
  totalContribution: string
  walletCreated: boolean
  walletCreating: boolean
  walletCreateFailed: boolean
  walletCorrupted: boolean
}

interface Props {
  textDirection: string
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
      showRewards: true,
      adsEstimatedEarnings: 5,
      enabledAds: false,
      enabledMain: false,
      promotions: [],
      balance: {
        total: 0,
        rates: {},
        wallets: {}
      },
      totalContribution: '0.0',
      walletCreated: false,
      walletCreating: false,
      walletCreateFailed: false,
      walletCorrupted: false
    }
  }

  doNothing = (s: string) => {
    /* no-op */
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

  createWallet = () => {
    this.setState({ walletCreating: true })
    setTimeout(() => {
      this.setState({ walletCreated: true })
      this.enableAds()
      this.enableRewards()
    }, 1000)
  }

  render () {
    const { showSettingsMenu, showBackgroundImage, showClock, showStats, showTopSites, showRewards } = this.state
    const {
      enabledAds,
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

    return (
      <App dataIsReady={true} dir={textDirection}>
      <PosterBackground hasImage={showBackgroundImage} imageHasLoaded={true}>
        {showBackgroundImage && <img src={generateRandomBackgroundData.source} />}
      </PosterBackground>
        {showBackgroundImage && <Gradient imageHasLoaded={true} />}
        <Page>
          <Header>
            <Stats
              textDirection={textDirection}
              showWidget={showStats}
              menuPosition={'right'}
              hideWidget={this.toggleShowStats}
            />
            <Clock
              textDirection={textDirection}
              showWidget={showClock}
              menuPosition={'left'}
              hideWidget={this.toggleShowClock}
            />
            <TopSitesList
              textDirection={textDirection}
              showWidget={showTopSites}
              menuPosition={'right'}
              hideWidget={this.toggleShowTopSites}
            />
            <Rewards
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
              showWidget={showRewards}
              menuPosition={'left'}
              hideWidget={this.toggleShowRewards}
              onDismissNotification={this.doNothing}
              totalContribution={totalContribution}
            />
            <SiteRemovalNotification />
          </Header>
          <Footer>
            <FooterInfo
              textDirection={textDirection}
              onClickOutside={this.closeSettings}
              backgroundImageInfo={generateRandomBackgroundData}
              onClickSettings={this.toggleSettings}
              showSettingsMenu={showSettingsMenu}
              showPhotoInfo={showBackgroundImage}
              toggleShowBackgroundImage={this.toggleShowBackgroundImage}
              toggleShowClock={this.toggleShowClock}
              toggleShowStats={this.toggleShowStats}
              toggleShowTopSites={this.toggleShowTopSites}
              toggleShowRewards={this.toggleShowRewards}
              showBackgroundImage={showBackgroundImage}
              showClock={showClock}
              showStats={showStats}
              showTopSites={showTopSites}
              showRewards={showRewards}
            />
          </Footer>
        </Page>
      </App>
    )
  }
}
