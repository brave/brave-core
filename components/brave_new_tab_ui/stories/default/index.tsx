/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

// Feature-specific components
import Clock from '../../../../src/old/clock'
import { Page, Header, Main, Footer, DynamicBackground, Gradient } from '../../../../src/features/newTab/default'

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

export default class NewTabPage extends React.PureComponent<{}, {}> {
  render () {
    return (
      <DynamicBackground background={generateRandomBackgroundData.source}>
        <Gradient />
        <Page>
          <Header>
            <Stats />
            <Clock />
            <Main>
              <TopSitesList />
              <SiteRemovalNotification />
            </Main>
          </Header>
          <Footer>
            <FooterInfo backgroundImageInfo={generateRandomBackgroundData} />
          </Footer>
        </Page>
      </DynamicBackground>
    )
  }
}
