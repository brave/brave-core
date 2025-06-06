/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Icon from '@brave/leo/react/icon'

import { TopSite, TopSitesListKind } from '../../state/top_sites_state'
import { useTopSitesState, useTopSitesActions } from '../../context/top_sites_context'
import { getString } from '../../lib/strings'
import { inlineCSSVars } from '../../lib/inline_css_vars'
import { RemoveToast } from './remove_toast'
import { TopSitesTile, DropLocation } from './top_site_tile'
import { TopSiteEditModal } from './top_site_edit_modal'
import { Popover } from '../common/popover'
import classNames from '$web-common/classnames'

import {
  style,
  collapsedTileCount,
  maxTileCount } from './top_sites.style'

export function TopSites() {
  const actions = useTopSitesActions()

  const showTopSites = useTopSitesState((s) => s.showTopSites)
  const listKind = useTopSitesState((s) => s.topSitesListKind)
  const topSites = useTopSitesState((s) => s.topSites)

  const [expanded, setExpanded] = React.useState(loadExpandedState())
  const [showEditSite, setShowEditSite] = React.useState(false)
  const [editSite, setEditSite] = React.useState<TopSite | null>(null)
  const [showTopSitesMenu, setShowTopSitesMenu] = React.useState(false)
  const [contextMenuSite, setContextMenuSite] =
      React.useState<TopSite | null>(null)
  const [showRemoveToast, setShowRemoveToast] = React.useState(false)

  const rootRef = React.useRef<HTMLDivElement>(null)

  const showAddButton = listKind === TopSitesListKind.kCustom
  const tileCount = topSites.length + (showAddButton ? 1 : 0)

  function onTopSiteContextMenu(topSite: TopSite) {
    return (event: React.MouseEvent) => {
      setContextMenuSite(topSite)
      const elem = rootRef.current
      if (elem) {
        elem.style.setProperty('--self-context-menu-x', event.pageX + 'px')
        elem.style.setProperty('--self-context-menu-y', event.pageY + 'px')
      }
    }
  }

  function topSitesMenuAction(fn: () => void) {
    return () => {
      fn()
      setShowTopSitesMenu(false)
    }
  }

  function onTileDrop(position: number) {
    return (url: string, location: DropLocation) => {
      const current = topSites.findIndex((item) => item.url === url)
      if (current < 0) {
        return
      }
      let index = position;
      if (location === 'after' && index < current) {
        index += 1
      }
      actions.setTopSitePosition(url, index)
    }
  }

  return (
    <div
      ref={rootRef}
      className={classNames({
        'expanded': expanded,
        'collapsed': !expanded,
        'hidden': !showTopSites || tileCount === 0
       })}
      style={inlineCSSVars({ '--self-tile-count': tileCount })}
      data-css-scope={style.scope}
    >
      <div className='top-site-context-menu-anchor' />
      <div className='top-sites'>
        <div className='tile-drop-indicator' />
        <button
          className='menu-button'
          onClick={() => setShowTopSitesMenu(true)}
        >
          <Icon name='more-vertical' />
        </button>
        <Popover
          isOpen={showTopSitesMenu}
          className='top-sites-menu'
          onClose={() => setShowTopSitesMenu(false)}
        >
          <div className='popover-menu'>
            {
              listKind === TopSitesListKind.kCustom ?
                <button onClick={topSitesMenuAction(() =>
                  actions.setTopSitesListKind(TopSitesListKind.kMostVisited))
                }>
                  <Icon name='history' />
                  {getString('topSitesShowMostVisitedLabel')}
                </button> :
                <button onClick={topSitesMenuAction(() =>
                  actions.setTopSitesListKind(TopSitesListKind.kCustom))
                }>
                  <Icon name='star-outline' />
                  {getString('topSitesShowCustomLabel')}
                </button>
            }
            <button
              onClick={topSitesMenuAction(() =>
                actions.setShowTopSites(false))
              }
            >
              <Icon name='eye-off' />
              {getString('hideTopSitesLabel')}
            </button>
          </div>
        </Popover>
        <div className='top-site-tiles-mask'>
          <div className='top-site-tiles'>
            {
              topSites.map((topSite, i) => {
                if (i > maxTileCount) {
                  return null
                }
                return (
                  <TopSitesTile
                    key={i}
                    topSite={topSite}
                    canDrag={listKind === TopSitesListKind.kCustom}
                    onRightClick={onTopSiteContextMenu(topSite)}
                    onDrop={onTileDrop(i)}
                  />
                )
              })
            }
            {
              showAddButton &&
                <button
                  className='top-site-tile'
                  onClick={() => {
                    setEditSite(null)
                    setShowEditSite(true)
                  }}
                >
                  <span className='top-site-icon'>
                    <Icon name='plus-add' />
                  </span>
                  <span className='top-site-title'>
                    {getString('addTopSiteLabel')}
                  </span>
                </button>
            }
          </div>
        </div>
        <button
          className='expand-button'
          disabled={tileCount <= collapsedTileCount}
          onClick={() => {
            saveExpandedState(!expanded)
            setExpanded(!expanded)}
          }
        >
          <Icon name={expanded ? 'contract' : 'expand' } />
        </button>
        <Popover
          isOpen={Boolean(contextMenuSite)}
          className='top-site-context-menu'
          onClose={() => setContextMenuSite(null)}
        >
          <div className='popover-menu'>
            {
              listKind === TopSitesListKind.kCustom &&
                <button onClick={() => {
                  setEditSite(contextMenuSite)
                  setShowEditSite(true)
                  setContextMenuSite(null)
                }}>
                  <Icon name='edit-pencil' />
                  {getString('editTopSiteLabel')}
                </button>
            }
            <button onClick={() => {
              if (contextMenuSite) {
                actions.removeTopSite(contextMenuSite.url)
                setContextMenuSite(null)
                setShowRemoveToast(true)
              }
            }}>
              <Icon name='trash' />
              {getString('removeTopSiteLabel')}
            </button>
          </div>
        </Popover>
        <TopSiteEditModal
          topSite={editSite}
          isOpen={showEditSite}
          onSave={(url, title) => {
            if (editSite) {
              actions.updateTopSite(editSite.url, url, title)
            } else {
              actions.addTopSite(url, title)
            }
            setShowEditSite(false)
          }}
          onClose={() => { setShowEditSite(false)}}
        />
        <RemoveToast
          isOpen={showRemoveToast}
          onUndo={() => {
            actions.undoRemoveTopSite()
            setShowRemoveToast(false)
          }}
          onClose={() => {
            setShowRemoveToast(false)
          }}
        />
      </div>
    </div>
  )
}

// TODO(https://github.com/brave/brave-browser/issues/45697): Use a pref to
// persist the expanded state.
const expandedStorageKey = 'ntp-top-sites-expanded'

function loadExpandedState(): boolean {
  const value = localStorage.getItem(expandedStorageKey)
  try { return Boolean(JSON.parse(value || '')) }
  catch { return false }
}

function saveExpandedState(expanded: boolean) {
  localStorage.setItem(expandedStorageKey, JSON.stringify(expanded))
}
