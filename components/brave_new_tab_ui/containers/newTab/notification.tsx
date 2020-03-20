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

// Utils
import { getLocale } from '../../../common/locale'

interface Props {
  actions: any
}

export default class Notification extends React.Component<Props, {}> {
  onUndoIgnoredTopSite = () => {
    this.props.actions.undoSiteIgnored()
  }

  onUndoAllSiteIgnored = () => {
    this.props.actions.undoAllSiteIgnored()
  }

  onHideSiteRemovalNotification = () => {
    this.props.actions.onHideSiteRemovalNotification()
  }

  render () {
    return (
       <SiteRemovalNotification>
         <SiteRemovalText>{getLocale('thumbRemoved')}</SiteRemovalText>
         <SiteRemovalAction onClick={this.onUndoIgnoredTopSite}>{getLocale('undoRemoved')}</SiteRemovalAction>
         <SiteRemovalAction onClick={this.onUndoAllSiteIgnored}>{getLocale('restoreAll')}</SiteRemovalAction>
         <SiteRemovalAction onClick={this.onHideSiteRemovalNotification} iconOnly={true} title={getLocale('close')}>
          <CloseStrokeIcon />
        </SiteRemovalAction>
       </SiteRemovalNotification>
    )
  }
}
