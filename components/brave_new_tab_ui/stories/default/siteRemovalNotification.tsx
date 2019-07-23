/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

// Feature-specific components
import {
  SiteRemovalNotification,
  SiteRemovalAction,
  SiteRemovalText
} from '../../components/default'

// Icons
import { CloseStrokeIcon } from 'brave-ui/components/icons'

// Helpers
import { getLocale } from '../fakeLocale'

export default class TopSite extends React.PureComponent<{}, {}> {
  render () {
    return (
      <SiteRemovalNotification>
        <SiteRemovalText>{getLocale('thumbRemoved')}</SiteRemovalText>
        <SiteRemovalAction>{getLocale('undoRemoved')}</SiteRemovalAction>
        <SiteRemovalAction>{getLocale('restoreAll')}</SiteRemovalAction>
        <SiteRemovalAction iconOnly={true} title={getLocale('close')}><CloseStrokeIcon /></SiteRemovalAction>
      </SiteRemovalNotification>
    )
  }
}
