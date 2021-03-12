// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'

// DnD utils
import {
  SortableContainer,
  SortEnd,
  SortableContainerProps
} from 'react-sortable-hoc'

// Feature-specific components
import { List, ListProps } from '../../components/default/gridSites'
import createWidget from '../../components/default/widget'

// Component groups
import GridSiteTile from './gridTile'
import AddSiteTile from './addSiteTile'

// Constants
import { MAX_GRID_SIZE } from '../../constants/new_tab_ui'

// Types
import * as newTabActions from '../../actions/new_tab_actions'
import * as gridSitesActions from '../../actions/grid_sites_actions'

interface Props {
  actions: typeof newTabActions & typeof gridSitesActions
  customLinksEnabled: boolean
  gridSites: NewTab.Site[]
  onShowEditTopSite: (targetTopSiteForEditing?: NewTab.Site) => void
}

interface State {
  isDragging: boolean
}

type DynamicListProps = SortableContainerProps & ListProps
const DynamicList = SortableContainer((props: DynamicListProps) => {
  return <List {...props} />
})

class TopSitesList extends React.PureComponent<Props, State> {
  updateBeforeSortStart = () => {
    this.setState({ isDragging: true })
  }

  onSortEnd = ({ oldIndex, newIndex }: SortEnd) => {
    this.setState({ isDragging: false })

    // User can't change order in "Most Visited" mode
    // and they can't change position of super referral tiles
    if (this.props.gridSites[newIndex].defaultSRTopSite ||
        !this.props.customLinksEnabled) {
      return
    }
    this.props.actions.tilesReordered(this.props.gridSites, oldIndex, newIndex)
  }

  constructor (props: Props) {
    super(props)
    this.state = {
      isDragging: false
    }
  }

  render () {
    const { actions, gridSites, onShowEditTopSite, customLinksEnabled } = this.props
    const insertAddSiteTile = customLinksEnabled && gridSites.length < MAX_GRID_SIZE
    let maxGridSize = customLinksEnabled ? MAX_GRID_SIZE : (MAX_GRID_SIZE / 2)

    // In favorites mode, makes widget area fits to tops sites items count + 1 if
    // items is less than 6. If items are more than 6, we don't need to care about
    // empty space in second row.
    // Plus one is for addSite tile.
    if (customLinksEnabled && gridSites.length < 6) {
      maxGridSize = gridSites.length + 1
    }

    // In frecency mode, makes widget area fits to top sites items.
    if (!customLinksEnabled) {
      maxGridSize = Math.min(gridSites.length, maxGridSize)
    }

    return (
      <>
        <DynamicList
          blockNumber={maxGridSize}
          updateBeforeSortStart={this.updateBeforeSortStart}
          onSortEnd={this.onSortEnd}
          axis='xy'
          lockToContainerEdges={true}
          lockOffset={'15%'}
          // Ensure there is some movement from the user side before triggering the
          // draggable handler. Otherwise click events will be swallowed since
          // react-sortable-hoc works via mouseDown event.
          // See https://github.com/clauderic/react-sortable-hoc#click-events-being-swallowed
          distance={2}
        >
          {
            gridSites.slice(0, maxGridSize)
              .map((siteData: NewTab.Site, index: number) => (
                <GridSiteTile
                  key={siteData.id}
                  actions={actions}
                  index={index}
                  siteData={siteData}
                  isDragging={this.state.isDragging}
                  onShowEditTopSite={onShowEditTopSite}
                  // User can't change order in "Most Visited" mode
                  // and they can't change position of super referral tiles
                  disabled={siteData.defaultSRTopSite || !this.props.customLinksEnabled}
                />
              ))
          }
          {
            insertAddSiteTile &&
            <AddSiteTile
              index={gridSites.length}
              disabled={true}
              isDragging={this.state.isDragging}
              showEditTopSite={onShowEditTopSite}
            />
          }
        </DynamicList>
      </>
    )
  }
}

export default createWidget(TopSitesList)
