/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

// Feature-specific components
import { Clock } from '../../../../src/features/newTab/default/clock'
import { Page, Header, Main, Footer, DynamicBackground, Gradient } from '../../../../src/features/newTab/default'

import Settings from './settings'
import TopSitesList from './topSites/topSitesList'
import Stats from './stats'
import SiteRemovalNotification from './siteRemovalNotification'
import FooterInfo from './footerInfo'

// Assets
import { getRandomBackgroundData } from './helpers'
import { images } from './data/background'

import '../../../assets/fonts/muli.css'
import '../../../assets/fonts/poppins.css'

const generateRandomBackgroundData = getRandomBackgroundData(images)
interface State {
  showSettings: boolean
  showBackgroundImage: boolean
}

export default class NewTabPage extends React.PureComponent<{}, State> {
  constructor (props: {}) {
    super(props)
    this.state = {
      showBackgroundImage: true,
      showSettings: false
    }
  }

  toggleShowBackgroundImage = () => {
    this.setState({ showBackgroundImage: !this.state.showBackgroundImage })
  }

  showSettings = () => {
    this.setState({ showSettings: true })
  }

  closeSettings = () => {
    this.setState({ showSettings: false })
  }

  render () {
    const { showSettings, showBackgroundImage } = this.state
    return (
      <DynamicBackground showBackgroundImage={showBackgroundImage} background={generateRandomBackgroundData.source}>
        {showBackgroundImage && <Gradient />}
        <Page>
          <Header>
            <Stats />
            <Clock />
            <Main>
              <TopSitesList />
              <SiteRemovalNotification />
            </Main>
          </Header>
          {
            showSettings &&
            <Settings
              onClickOutside={this.closeSettings}
              toggleShowBackgroundImage={this.toggleShowBackgroundImage}
              showBackgroundImage={showBackgroundImage}
            />
          }
          <Footer>
            <FooterInfo
              backgroundImageInfo={generateRandomBackgroundData}
              onClickSettings={this.showSettings}
              isSettingsMenuOpen={showSettings}
              showPhotoInfo={showBackgroundImage}
            />
          </Footer>
        </Page>
      </DynamicBackground>
    )
  }
}
