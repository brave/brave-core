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
import { List } from '../../components/default/gridSites'
import createWidget from '../../components/default/widget'

// Component groups
import GridSiteTile from './gridTile'

// Constants
import { MAX_GRID_SIZE } from '../../constants/new_tab_ui'

import {
  reorderMostVisitedTile
} from '../../api/topSites'

// Types
import * as newTabActions from '../../actions/new_tab_actions'
import * as gridSitesActions from '../../actions/grid_sites_actions'

interface Props {
  actions: typeof newTabActions & typeof gridSitesActions
  gridSites: NewTab.Site[]
}

type DynamicListProps = SortableContainerProps & { blockNumber: number }
const DynamicList = SortableContainer((props: DynamicListProps) => {
  return <List {...props} />
})

class TopSitesList extends React.PureComponent<Props, {}> {
  onSortEnd = ({ oldIndex, newIndex }: SortEnd) => {
    reorderMostVisitedTile(this.props.gridSites[oldIndex].url, newIndex)
  }

  render () {
    const { actions, gridSites } = this.props
    return (
      <>
        <DynamicList
          blockNumber={MAX_GRID_SIZE}
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
            // Grid sites are currently limited to 6 tiles
            gridSites.slice(0, 100)//MAX_GRID_SIZE)
              .map((siteData: NewTab.Site, index: number) => (
                <GridSiteTile
                  key={siteData.id}
                  actions={actions}
                  index={index}
                  siteData={siteData}
                />
          ))}
        </DynamicList>
      </>
    )
  }
}

export default createWidget(TopSitesList)
