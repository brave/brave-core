// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'

import { SortableElement, SortableElementProps } from 'react-sortable-hoc'

// Feature-specific components
import {
  Tile,
  TileActionsContainer,
  TileAction,
  TileFavicon
} from '../../components/default'

// Icons
import {
  CloseStrokeIcon
} from 'brave-ui/components/icons'

// Types
import * as newTabActions from '../../actions/new_tab_actions'
import * as gridSitesActions from '../../actions/grid_sites_actions'

interface Props {
  actions: typeof newTabActions & typeof gridSitesActions
  siteData: NewTab.Site
  disabled: boolean
}

function generateGridSiteFavicon (site: NewTab.Site): string {
  if (site.favicon === '') {
    return `chrome://favicon/size/64@1x/${site.url}`
  }
  return site.favicon
}

class TopSite extends React.PureComponent<Props, {}> {
  onIgnoredTopSite (site: NewTab.Site) {
    this.props.actions.tileRemoved(site.url)
  }

  render () {
    const { siteData } = this.props

    return (
      <Tile
        title={siteData.title}
        tabIndex={0}
      >
        <TileActionsContainer>
          <TileAction onClick={this.onIgnoredTopSite.bind(this, siteData)}>
            <CloseStrokeIcon />
          </TileAction>
        </TileActionsContainer>
          <a href={siteData.url}><TileFavicon src={generateGridSiteFavicon(siteData)} /></a>
      </Tile>
    )
  }
}

type TopSiteSortableElementProps = SortableElementProps & Props
export default SortableElement(
  (props: TopSiteSortableElementProps) => <TopSite {...props} />
)
