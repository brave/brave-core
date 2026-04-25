// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { useSortable } from '@dnd-kit/sortable'
import { CSS } from '@dnd-kit/utilities'
import { getScrollableParents, useParentScrolled } from '../../helpers/scrolling'
import * as React from 'react'
import { useEffect, useMemo, useRef, useState } from 'react'
import * as ReactDOM from 'react-dom'
import Icon from '@brave/leo/react/icon'
import { getLocale } from '../../../common/locale'
import * as gridSitesActions from '../../actions/grid_sites_actions'
// Types
import * as newTabActions from '../../actions/new_tab_actions'
// Feature-specific components
import {
  Tile, TileAction, TileActionsContainer, TileFavicon,
  TileImageContainer,
  TileMenu,
  TileMenuItem,
  TileTitle
} from '../../components/default'

interface Props {
  actions: typeof newTabActions & typeof gridSitesActions
  siteData: NewTab.Site
  isSortable: boolean
  onShowEditTopSite: (targetTopSiteForEditing?: NewTab.Site) => void
}

function generateGridSiteFavicon (site: NewTab.Site): string {
  if (site.favicon === '') {
    return `chrome://favicon/size/64@1x/${site.url}`
  }
  return site.favicon
}

export function SiteTile (props: { site: NewTab.Site, isMenuShowing?: boolean, children?: React.ReactNode, draggable?: ReturnType<typeof useSortable> }) {
  const { site, isMenuShowing, children, draggable } = props
  const style = useMemo(() => ({
    transform: CSS.Transform.toString(draggable?.transform || null),
    transition: draggable?.transition,
    opacity: draggable?.isDragging ? 0 : 1
  }), [draggable])

  return <Tile
    ref={draggable?.setNodeRef} {...draggable?.attributes} {...draggable?.listeners}
    isDragging={!!draggable?.isDragging}
    isMenuShowing={!!isMenuShowing}
    title={site.title}
    href={site.url}
    style={style}>
    {children}
    <TileImageContainer>
      <TileFavicon src={generateGridSiteFavicon(site)} />
    </TileImageContainer>
    <TileTitle>{site.title}</TileTitle>
  </Tile>
}

function TopSite (props: Props) {
  const { siteData, isSortable } = props

  const tileMenuRef = useRef<any>()
  const sortable = useSortable({ id: siteData.id, disabled: isSortable })
  const [showMenu, setShowMenu] = useState(false)

  useEffect(() => {
    const handleClickOutside = (e: Event) => {
      if (!tileMenuRef.current || tileMenuRef.current.contains(e.target)) { return }
      setShowMenu(false)
    }

    const handleKeyPress = (e: KeyboardEvent) => {
      if (e.key === 'Escape') { setShowMenu(false) }
    }

    document.addEventListener('mousedown', handleClickOutside)
    document.addEventListener('keydown', handleKeyPress)
    return () => {
      document.removeEventListener('mousedown', handleClickOutside)
      document.removeEventListener('keydown', handleKeyPress)
    }
  }, [])

  const [editMenuRef, setEditMenuRef] = useState<HTMLElement | null>(null)
  const [scrollableParent] = getScrollableParents(editMenuRef)

  // Work out the style for the edit menu. Because it's rendered as a child of
  // the scrollable container we need to subtract the scrollable containers
  // position to get the menu to align nicely with the child.
  const editMenuStyle = (() => {
    const bounds = editMenuRef?.getBoundingClientRect()
    const scrollableBounds = scrollableParent?.getBoundingClientRect()
    if (!bounds || !scrollableBounds) return
    return {
      top: bounds.bottom - scrollableBounds.y,
      left: bounds.right - scrollableBounds.x
    }
  })()

  useParentScrolled(editMenuRef, () => {
    setShowMenu(false)
  })

  return <SiteTile site={props.siteData} draggable={sortable} isMenuShowing={showMenu}>
    {!siteData.defaultSRTopSite
      ? <TileActionsContainer>
        <TileAction ref={setEditMenuRef} onClick={(e) => {
          e.preventDefault()
          setShowMenu(true)
        }}>
          <Icon name='edit-pencil' />
        </TileAction>
      </TileActionsContainer>
      : null}
    {showMenu && ReactDOM.createPortal(<TileMenu ref={tileMenuRef} style={editMenuStyle}>
      <TileMenuItem onClick={e => {
        e.preventDefault()
        setShowMenu(false)
        props.onShowEditTopSite(siteData)
      }}>
        <Icon name='edit-pencil' />
        {getLocale('editSiteTileMenuItem')}
      </TileMenuItem>
      <TileMenuItem onClick={e => {
        e.preventDefault()
        setShowMenu(false)
        props.actions.tileRemoved(siteData.url)
      }}>
        <Icon name='trash' />
        {getLocale('removeTileMenuItem')}
      </TileMenuItem>
    </TileMenu>, scrollableParent)}
  </SiteTile>
}

export default TopSite
