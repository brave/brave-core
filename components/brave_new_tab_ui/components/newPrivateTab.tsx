/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
const { getLocale } = require('../../common/locale')

// Components
const {
  Page,
  Grid,
  Column,
  MediaContent,
  BoxedContent,
  Heading,
  Paragraph,
  SwitchButton,
  Clock
} = require('brave-ui')

const Stats = require('./stats')

// Theme
const theme = require('./theme')
require('emptykit.css')

// Images
const privateTabIcon = require('../../img/newtab/private_tab_pagearea_icon.svg')

class NewPrivateTab extends React.PureComponent {
  render () {
    const {
      stats,
      useAlternativePrivateSearchEngine,
      onChangePrivateSearchEngine
    } = this.props

    // do not render if stats aren't loaded yet
    if (!stats) {
      return null
    }

    return (
      <Page theme={theme.newPrivateTab}>
        <Grid columns={3}>
          <Column size={2}>
            <Stats stats={stats} theme={theme} />
          </Column>
          <Column size={1} theme={theme.clockContainer}>
            <Clock theme={theme.clock} />
          </Column>
        </Grid>
        <BoxedContent theme={theme.textualContainer}>
          <MediaContent media={privateTabIcon} theme={theme.media}>
            <Heading
              level={1}
              theme={theme.title}
              text={getLocale('privateNewTabTitle')} />
            <Paragraph
              theme={theme.text}
              text={getLocale('privateNewTabDisclaimer1')} />
            <Paragraph
              theme={theme.italicText}
              text={getLocale('privateNewTabDisclaimer2')} />
            <BoxedContent theme={theme.switchContainer}>
              <SwitchButton
                id='togglePrivateSearchEngine'
                size='large'
                theme={theme.switchButton}
                checked={useAlternativePrivateSearchEngine}
                onChange={onChangePrivateSearchEngine}
                rightText={getLocale('privateNewTabSearchLabel')} />
            </BoxedContent>
            <Paragraph theme={theme.text} text={getLocale('duckduckGoSearchInfo')} />
          </MediaContent>
        </BoxedContent>
      </Page>
    )
  }
}

module.exports = NewPrivateTab
