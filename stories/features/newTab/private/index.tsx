/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

// Components
import Page from '../../../../src/old/page/index'
import { Grid, Column } from '../../../../src/components/layout/gridList/index'
import { DataBlock, DataItem } from '../../../../src/old/dataBlock/index'
import Clock from '../../../../src/old/clock/index'
import MediaContent from '../../../../src/old/mediaContent/index'
import BoxedContent from '../../../../src/old/boxedContent/index'
import { Heading } from '../../../../src/old/headings/index'
import Paragraph from '../../../../src/old/paragraph/index'
import SwitchButton from '../../../../src/old/switchButton/index'

import theme from './theme'

// Assets
import locale from './fakeLocale'
import data from './fakeData'
import '../../../assets/fonts/muli.css'
import '../../../assets/fonts/poppins.css'

// Images
const privateTabIcon = require('../../../assets/img/private_tab_pagearea_icon.svg')

class NewPrivateTab extends React.PureComponent {
  render () {
    return (
      <Page theme={theme.page}>
        <Grid columns={3}>
          <Column size={2}>
            <DataBlock>
              <DataItem
                theme={theme.trackersBlocked}
                description={locale.trackersBlocked}
                counter={data.trackersBlockedCount} />
              <DataItem
                theme={theme.adsBlocked}
                description={locale.adsBlocked}
                counter={data.adsBlockedCount} />
              <DataItem
                theme={theme.httpsUpgrades}
                description={locale.httpsUpgrades}
                counter={data.httpsUpgradesCount} />
              <DataItem
                theme={theme.estimatedTime}
                description={locale.estimatedTime}
                text={locale.minutes}
                counter={data.estimatedTimeCount} />
            </DataBlock>
          </Column>
          <Column size={1} theme={theme.clockContainer}>
            <Clock theme={theme.clock} />
          </Column>
        </Grid>
        <BoxedContent theme={theme.textualContainer}>
          <MediaContent media={privateTabIcon} theme={theme.media}>
            <Heading level={1} theme={theme.title} text={locale.title} />
              <Paragraph theme={theme.text} text={locale.paragraph1} />
              <Paragraph theme={theme.italicText} text={locale.paragraph2} />
              <BoxedContent theme={theme.switchContainer}>
                <SwitchButton
                  id='togglePrivateSearchEngine'
                  size='large'
                  checked={false}
                  rightText={locale.switchLabel} />
              </BoxedContent>
              <Paragraph theme={theme.text} text={locale.paragraph3} />
          </MediaContent>
        </BoxedContent>
      </Page>
    )
  }
}

export default NewPrivateTab
