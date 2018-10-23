/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

// Feature-specific components
import Clock from '../../../../src/old/clock'
import { Page, Header, Main, Footer, DynamicBackground, Gradient } from '../../../../src/features/newTab/default'

import TopSitesList from './topSites/topSitesList'
import Stats from './stats'
import FooterInfo from './footerInfo'

// Assets
const fakeBackground = require('../../../assets/img/spacex_1.jpg')
// Additional images are uploaded in img for randomizing functionality

import '../../../assets/fonts/muli.css'
import '../../../assets/fonts/poppins.css'

interface Props {}

export default class NewTabPage extends React.PureComponent<Props, {}> {
  render () {
    return (
      <DynamicBackground background={fakeBackground}>
        <Gradient />
        <Page>
          <Header>
            <Stats />
            <Clock />
            <Main>
              <TopSitesList />
            </Main>
          </Header>
          <Footer>
            <FooterInfo />
          </Footer>
        </Page>
      </DynamicBackground>
    )
  }
}
