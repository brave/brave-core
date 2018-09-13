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
  LabelledText,
  LinkText
} from 'brave-ui/features/newTab'

// Assets
const torIcon = require('../../img/newtab/toricon.svg')

// Misc
const torFAQ = 'https://github.com/brave/browser-laptop/wiki/Using-Tor-in-Brave#faq'

/**
 * Private Tab Tor Option
 * Wrapper block around the Tor section including a descriptive text
 */
export default class PrivateTabTorOption extends React.PureComponent<{}, {}> {
  render () {
    return (
      <FeatureBlock grid={true}>
        <aside>
          <img src={torIcon} />
        </aside>
        <article>
          <NewTabHeading level={2}>
            {getLocale('makeThisTabMuchMorePrivateWith')}&nbsp;
            <EmphasizedText>tor</EmphasizedText>
            <LabelledText>beta</LabelledText>
          </NewTabHeading>
          <Paragraph>{getLocale('torDisclaimer')}</Paragraph>
          <LinkText
            href={torFAQ}
            target='_blank'
            rel='noreferrer noopener'
          >
            {getLocale('learnMore')}
          </LinkText>
        </article>
      </FeatureBlock>
    )
  }
}
