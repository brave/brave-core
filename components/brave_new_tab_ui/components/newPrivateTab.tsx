/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { getLocale } from '../../common/locale'
import { Grid, Column } from 'brave-ui/components'
import {
  Page,
  MediaContent,
  BoxedContent,
  Heading,
  Paragraph,
  SwitchButton,
  Clock
} from 'brave-ui/old'

// Components
import Stats from './stats'

// Constant
import { theme } from '../constants/theme'

// Assets
const privateTabIcon = require('../../img/newtab/private_tab_pagearea_icon.svg')
require('emptykit.css')

interface Props {
  stats: NewTab.Stats,
  useAlternativePrivateSearchEngine: boolean,
  onChangePrivateSearchEngine: (e: React.ChangeEvent<HTMLInputElement>) => void
}

export default class NewPrivateTab extends React.PureComponent<Props, {}> {
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
      <Page customStyle={theme.newPrivateTab}>
        <Grid columns={3}>
          <Column size={2}>
            <Stats stats={stats} />
          </Column>
          <Column size={1} customStyle={theme.clockContainer}>
            <Clock customStyle={theme.clock} />
          </Column>
        </Grid>
        <BoxedContent customStyle={theme.textualContainer}>
          <MediaContent media={privateTabIcon} customStyle={theme.media}>
            <Heading
              level={1}
              customStyle={theme.title}
              text={getLocale('privateNewTabTitle')}
            />
            <Paragraph
              customStyle={theme.text}
              text={getLocale('privateNewTabDisclaimer1')}
            />
            <Paragraph
              customStyle={theme.italicText}
              text={getLocale('privateNewTabDisclaimer2')}
            />
            <BoxedContent customStyle={theme.switchContainer}>
              <SwitchButton
                id='togglePrivateSearchEngine'
                size='large'
                customStyle={theme.switchButton}
                checked={useAlternativePrivateSearchEngine}
                onChange={onChangePrivateSearchEngine}
                rightText={getLocale('privateNewTabSearchLabel')}
              />
            </BoxedContent>
            <Paragraph customStyle={theme.text} text={getLocale('duckduckGoSearchInfo')} />
          </MediaContent>
        </BoxedContent>
      </Page>
    )
  }
}
