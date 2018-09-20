/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

// Feature-specific Components
import { Page,PageWrapper, PageHeader, Clock, NewTabHeading } from 'brave-ui/features/newTab'
import PrivateTabTorOption from './privateTabTorOption'
import PrivateTabSearchEngineOption from './privateTabSearchEngineOption'
import PrivateTabFooter from './privateTabFooter'
import Stats from './stats'

// Assets
import { getLocale } from '../../common/locale'
require('emptykit.css')

interface Props {
  stats: NewTab.Stats,
  useAlternativePrivateSearchEngine: boolean,
  isTor: boolean,
  onChangePrivateSearchEngine: (e: React.ChangeEvent<HTMLInputElement>) => void
}

export default class NewPrivateTab extends React.PureComponent<Props, {}> {
  render () {
    const {
      stats,
      useAlternativePrivateSearchEngine,
      isTor,
      onChangePrivateSearchEngine
    } = this.props

    // do not render if stats aren't loaded yet
    if (!stats) {
      return null
    }
    return (
      <Page>
        <PageWrapper>
          <PageHeader>
            <Stats stats={stats} />
            <Clock />
          </PageHeader>
          <main>
            <NewTabHeading>
              {getLocale(isTor ? 'thisIsAPrivateWindowWithTor' : 'thisIsAPrivateWindow')}
            </NewTabHeading>
            {
              !isTor
                ? <PrivateTabTorOption />
                : null
            }
            <PrivateTabSearchEngineOption
              onChangePrivateSearchEngine={onChangePrivateSearchEngine}
              useAlternativePrivateSearchEngine={useAlternativePrivateSearchEngine}
              isTor={isTor}
            />
            <PrivateTabFooter />
          </main>
        </PageWrapper>
      </Page>
    )
  }
}
