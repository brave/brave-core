/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

// Feature-specific components
import { Page, Header, Footer, App, PosterBackground, Gradient, ClockWidget as Clock } from '../../components/default'

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
      showTopSites: true
    }
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

  closeSettings = () => {
    this.setState({ showSettingsMenu: false })
  }

  toggleSettings = () => {
    this.setState({ showSettingsMenu: !this.state.showSettingsMenu })
  }

  render () {
    const { showSettingsMenu, showBackgroundImage, showClock, showStats, showTopSites } = this.state
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
              showBackgroundImage={showBackgroundImage}
              showClock={showClock}
              showStats={showStats}
              showTopSites={showTopSites}
            />
          </Footer>
        </Page>
      </App>
    )
  }
}
