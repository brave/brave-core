// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

// DnD Kit
import { AutoScrollOptions, DndContext, DragEndEvent, KeyboardSensor, MouseSensor, PointerActivationConstraint, TouchSensor, useDndContext, useSensor, useSensors } from '@dnd-kit/core'
import { SortableContext } from '@dnd-kit/sortable'
import * as React from 'react'
import { useRef } from 'react'
import * as gridSitesActions from '../../actions/grid_sites_actions'
// Types
import * as newTabActions from '../../actions/new_tab_actions'
// Feature-specific components
import { GridPagesContainer, List, PagesContainer } from '../../components/default/gridSites'
import createWidget from '../../components/default/widget'
// Constants
import { MAX_GRID_SIZE } from '../../constants/new_tab_ui'
import AddSiteTile from './addSiteTile'
import { GridPageButtons } from './gridPageButtons'
// Component groups
import GridSiteTile from './gridTile'
import { TopSiteDragOverlay } from './gridTileOverlay'

// Note: to increase the number of pages, you will also need to increase
// kMaxNumCustomLinks in ntp_tiles/constants.cc
const MAX_PAGES = 4
const activationConstraint: PointerActivationConstraint = { distance: 2 }
const autoScrollOptions: AutoScrollOptions = { interval: 500 }

interface Props {
  actions: typeof newTabActions & typeof gridSitesActions
  customLinksEnabled: boolean
  gridSites: NewTab.Site[]
  onShowEditTopSite: (targetTopSiteForEditing?: NewTab.Site) => void
}

function TopSitesPage (props: Props & { maxGridSize: number, page: number }) {
  const start = props.page * props.maxGridSize
  const items = props.gridSites.slice(start, start + props.maxGridSize)
  const { active } = useDndContext()
  return <List>
    {items.map((siteData) => (
      <GridSiteTile
        key={siteData.id}
        actions={props.actions}
        siteData={siteData}
        onShowEditTopSite={props.onShowEditTopSite}
        // User can't change order in "Most Visited" mode
        // and they can't change position of super referral tiles
        isSortable={siteData.defaultSRTopSite || !props.customLinksEnabled}
      />
    ))}
    {start + props.maxGridSize > props.gridSites.length &&
      props.customLinksEnabled &&
      <AddSiteTile
        isDragging={!!active}
        showEditTopSite={props.onShowEditTopSite}
      />}
  </List>
}

function TopSitesList (props: Props) {
  const { gridSites, customLinksEnabled } = props
  const maxGridSize = customLinksEnabled ? MAX_GRID_SIZE : (MAX_GRID_SIZE / 2)

  const gridPagesContainerRef = useRef<HTMLDivElement>()

  const numSites = customLinksEnabled ? gridSites.length + 1 : gridSites.length
  const pageCount = Math.min(MAX_PAGES, Math.ceil(numSites / maxGridSize))
  const pages = [...Array(pageCount).keys()]

  const mouseSensor = useSensor(MouseSensor, { activationConstraint })
  const touchSensor = useSensor(TouchSensor, { activationConstraint })
  const keyboardSensor = useSensor(KeyboardSensor, {})
  const sensors = useSensors(mouseSensor, touchSensor, keyboardSensor)

  const handleDragEnd = (e: DragEndEvent) => {
    e.activatorEvent.preventDefault()

    const draggingIndex = gridSites.findIndex(s => s.id === e.active.id)
    const droppedIndex = gridSites.findIndex(s => s.id === e.over?.id)

    if (draggingIndex === undefined || droppedIndex === undefined) { return }

    if (gridSites[droppedIndex].defaultSRTopSite || !props.customLinksEnabled) { return }

    props.actions.tilesReordered(gridSites, draggingIndex, droppedIndex)
  }

  return <PagesContainer>
    <GridPagesContainer customLinksEnabled={customLinksEnabled} ref={gridPagesContainerRef as any}>
      <DndContext onDragEnd={handleDragEnd} autoScroll={autoScrollOptions} sensors={sensors}>
        <SortableContext items={gridSites}>
          {pages.map(page => <TopSitesPage key={page} page={page} maxGridSize={maxGridSize} {...props} />)}
          <TopSiteDragOverlay sites={gridSites} />
        </SortableContext>
      </DndContext>
    </GridPagesContainer>
    {customLinksEnabled
      && pageCount > 1
      && <GridPageButtons numPages={pageCount} pageContainerRef={gridPagesContainerRef} />}
  </PagesContainer>
}

export default createWidget(TopSitesList)
