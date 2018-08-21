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

import customStyle from './theme'

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
      <Page customStyle={customStyle.page}>
        <Grid columns={3}>
          <Column size={2}>
            <DataBlock>
              <DataItem
                customStyle={customStyle.trackersBlocked}
                description={locale.trackersBlocked}
                counter={data.trackersBlockedCount}
              />
              <DataItem
                customStyle={customStyle.adsBlocked}
                description={locale.adsBlocked}
                counter={data.adsBlockedCount}
              />
              <DataItem
                customStyle={customStyle.httpsUpgrades}
                description={locale.httpsUpgrades}
                counter={data.httpsUpgradesCount}
              />
              <DataItem
                customStyle={customStyle.estimatedTime}
                description={locale.estimatedTime}
                text={locale.minutes}
                counter={data.estimatedTimeCount}
              />
            </DataBlock>
          </Column>
          <Column size={1} customStyle={customStyle.clockContainer}>
            <Clock customStyle={customStyle.clock} />
          </Column>
        </Grid>
        <BoxedContent customStyle={customStyle.textualContainer}>
          <MediaContent media={privateTabIcon} customStyle={customStyle.media}>
            <Heading level={1} customStyle={customStyle.title} text={locale.title} />
              <Paragraph customStyle={customStyle.text} text={locale.paragraph1} />
              <Paragraph customStyle={customStyle.italicText} text={locale.paragraph2} />
              <BoxedContent customStyle={customStyle.switchContainer}>
                <SwitchButton
                  id='togglePrivateSearchEngine'
                  size='large'
                  checked={false}
                  rightText={locale.switchLabel}
                />
              </BoxedContent>
              <Paragraph customStyle={customStyle.text} text={locale.paragraph3} />
          </MediaContent>
        </BoxedContent>
      </Page>
    )
  }
}

export default NewPrivateTab
