/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

// Assets
import { getLocale } from '../../common/locale'

// Feature-specific  Components
import { FeatureBlock, NewTabHeading, SmallText } from 'brave-ui/features/newTab'

/**
 * Private Tab footer content
 * Wrapper block with information about private tabs
 */
export default class PrivateTabFooter extends React.PureComponent<{}, {}> {
  render () {
    return (
      <FeatureBlock>
        <NewTabHeading level={3}>
          {getLocale('moreAboutPrivateTabs')}
        </NewTabHeading>
        <SmallText>{getLocale('privateTabsDisclaimer')}</SmallText>
      </FeatureBlock>
    )
  }
}
