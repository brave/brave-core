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

// Helpers
import {
  isGridSitePinned,
  isGridSiteBookmarked
} from '../../helpers/newTabUtils'

// Icons
import {
  PinIcon,
  PinOIcon,
  BookmarkIcon,
  BookmarkOIcon,
  CloseStrokeIcon
} from 'brave-ui/components/icons'

// Types
import * as newTabActions from '../../actions/new_tab_actions'
import * as gridSitesActions from '../../actions/grid_sites_actions'

interface Props {
  actions: typeof newTabActions & typeof gridSitesActions
  siteData: NewTab.Site
}

class TopSite extends React.PureComponent<Props, {}> {
  onTogglePinnedTopSite (site: NewTab.Site) {
    this.props.actions.toggleGridSitePinned(site)
  }

  onIgnoredTopSite (site: NewTab.Site) {
    this.props.actions.removeGridSite(site)
    this.props.actions.showGridSiteRemovedNotification(true)
  }

  onToggleBookmark (site: NewTab.Site) {
    this.props.actions.toggleGridSiteBookmarkInfo(site)
  }

  render () {
    const { siteData } = this.props

    return (
      <Tile
        title={siteData.title}
        tabIndex={0}
        style={{
          // Visually inform users that dragging a pinned site is not allowed.
          cursor: isGridSitePinned(siteData) ? 'not-allowed' : 'grab'
        }}
      >
        <TileActionsContainer>
          <TileAction onClick={this.onTogglePinnedTopSite.bind(this, siteData)}>
            {isGridSitePinned(siteData) ? <PinIcon /> : <PinOIcon />}
          </TileAction>
          <TileAction onClick={this.onToggleBookmark.bind(this, siteData)}>
            {
              isGridSiteBookmarked(siteData.bookmarkInfo)
                ? <BookmarkIcon />
                : <BookmarkOIcon />
            }
          </TileAction>
          {
            // Disallow removing a pinned site
            isGridSitePinned(siteData)
              ? (
                <TileAction
                  style={{ opacity: 0.25, cursor: 'not-allowed' }}
                >
                  <CloseStrokeIcon />
                </TileAction>
              ) : (
                <TileAction
                  onClick={this.onIgnoredTopSite.bind(this, siteData)}
                >
                  <CloseStrokeIcon />
                </TileAction>
              )
          }
        </TileActionsContainer>
        {
          // Add the permanent pinned icon if site is pinned
          isGridSitePinned(siteData)
            ? (
              <TileAction
                onClick={this.onTogglePinnedTopSite.bind(this, siteData)}
                standalone={true}
              >
                <PinIcon />
              </TileAction>
            ) : null
          }
          <a href={siteData.url}><TileFavicon src={siteData.favicon} /></a>
      </Tile>
    )
  }
}

type TopSiteSortableElementProps = SortableElementProps & Props
export default SortableElement(
  (props: TopSiteSortableElementProps) => <TopSite {...props} />
)
