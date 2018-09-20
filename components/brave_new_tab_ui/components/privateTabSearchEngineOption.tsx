/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

// Assets
import { getLocale } from '../../common/locale'

// Feature-specific  Components
import {
  FeatureBlock,
  Paragraph,
  NewTabHeading,
  EmphasizedText,
  LinkText
} from 'brave-ui/features/newTab'
import { SwitchButton } from 'brave-ui/old'

// Assets
const ddgIcon = require('../../img/newtab/private_tab_pagearea_ddgicon.svg')

interface Props {
  isTor: boolean
  useAlternativePrivateSearchEngine: boolean,
  onChangePrivateSearchEngine: (e: React.ChangeEvent<HTMLInputElement>) => void
}

/**
 * Private Tab Search Engine Option
 * Wrapper block around the search engine switch including a toggle and descriptive text
 * @prop {boolean} isTor - whether or not the Tor toggle is ON
 * @prop {boolean} useAlternativePrivateSearchEngine - whether or not user is using an alternative SE
 * @prop {React.MouseEvent<HTMLDivElement>} onChangePrivateSearchEngine - event that fires when user switches the toggle
 */
export default class PrivateTabSearchEngineOption extends React.PureComponent<Props, {}> {
  get searchEngineForTorScreen () {
    return (
      <FeatureBlock>
        <Paragraph>
          {getLocale('defaultSearchEngineDisclaimer')}&nbsp;&nbsp;
          <LinkText href='chrome://settings/search' target='blank' rel='noreferrer noopener'>
            {getLocale('searchPreferences')}
          </LinkText>
      </Paragraph>
    </FeatureBlock>
    )
  }

  get searchEngineDefaultScreen () {
    const { useAlternativePrivateSearchEngine, onChangePrivateSearchEngine } = this.props
    return (
      <FeatureBlock grid={true}>
        <aside>
          <img src={ddgIcon} />
        </aside>
        <article>
          <NewTabHeading
            level={2}
            onClick={onChangePrivateSearchEngine}
          >
            {getLocale('privateSearchWith')}&nbsp;
            <EmphasizedText>DuckDuckGo</EmphasizedText>
          </NewTabHeading>

          <Paragraph>{getLocale('duckduckgoDisclaimer')}</Paragraph>
        </article>
        <aside>
          <SwitchButton
            id='duckduckgo'
            size='large'
            checked={useAlternativePrivateSearchEngine}
            onChange={onChangePrivateSearchEngine}
          />
        </aside>
      </FeatureBlock>
    )
  }
  render () {
    const { isTor } = this.props
    return (
      isTor
        ? this.searchEngineForTorScreen
        : this.searchEngineDefaultScreen
    )
  }
}
