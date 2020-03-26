// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

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

// Types
import * as newTabActions from '../../actions/new_tab_actions'
import * as gridSitesActions from '../../actions/grid_sites_actions'

interface Props {
  actions: typeof newTabActions & typeof gridSitesActions
}

export default class Notification extends React.Component<Props, {}> {
  onUndoRemoveTopSite = () => {
    this.props.actions.undoRemoveGridSite()
    this.props.actions.showGridSiteRemovedNotification(false)
  }

  onUndoRemoveAllTopSites = () => {
    this.props.actions.undoRemoveAllGridSites()
    this.props.actions.showGridSiteRemovedNotification(false)
  }

  onHideSiteRemovalNotification = () => {
    this.props.actions.showGridSiteRemovedNotification(false)
  }

  render () {
    return (
       <SiteRemovalNotification>
          <SiteRemovalText>{getLocale('thumbRemoved')}</SiteRemovalText>
          <SiteRemovalAction
            onClick={this.onUndoRemoveTopSite}
          >
            {getLocale('undoRemoved')}
          </SiteRemovalAction>
         <SiteRemovalAction onClick={this.onUndoRemoveAllTopSites}>
            {getLocale('restoreAll')}
          </SiteRemovalAction>
          <SiteRemovalAction
            onClick={this.onHideSiteRemovalNotification}
            iconOnly={true}
            title={getLocale('close')}
          >
            <CloseStrokeIcon />
          </SiteRemovalAction>
       </SiteRemovalNotification>
    )
  }
}
