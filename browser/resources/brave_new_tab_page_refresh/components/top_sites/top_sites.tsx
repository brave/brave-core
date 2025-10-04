/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Icon from '@brave/leo/react/icon'

import { TopSite, TopSitesListKind } from '../../state/top_sites_state'
import { useTopSitesState, useTopSitesActions } from '../../context/top_sites_context'
import { usePersistedJSON } from '$web-common/usePersistedState'
import { getString } from '../../lib/strings'
import { RemoveToast } from './remove_toast'
import { TopSitesGrid } from './top_sites_grid'
import { TopSiteEditModal } from './top_site_edit_modal'
import { Popover } from '../common/popover'

import { style, collapsedTileColumnCount } from './top_sites.style'

export function TopSites() {
  const actions = useTopSitesActions()

  const showTopSites = useTopSitesState((s) => s.showTopSites)
  const listKind = useTopSitesState((s) => s.topSitesListKind)
  const topSites = useTopSitesState((s) => s.topSites)

  // TODO(https://github.com/brave/brave-browser/issues/45697): Use a pref to
  // persist the expanded state.
  const [expanded, setExpanded] =
    usePersistedJSON('ntp-top-sites-expanded', Boolean)

  const [showEditSite, setShowEditSite] = React.useState(false)
  const [editSite, setEditSite] = React.useState<TopSite | null>(null)
  const [showTopSitesMenu, setShowTopSitesMenu] = React.useState(false)
  const [contextMenuSite, setContextMenuSite] =
      React.useState<TopSite | null>(null)
  const [showRemoveToast, setShowRemoveToast] = React.useState(false)

  const rootRef = React.useRef<HTMLDivElement>(null)

  React.useEffect(() => {
    if (showTopSites && expanded) {
      document.body.classList.add('ntp-top-sites-wide')
    }
    return () => document.body.classList.remove('ntp-top-sites-wide')
  }, [showTopSites, expanded])

  const showAddButton = listKind === TopSitesListKind.kCustom
  const tileCount = topSites.length + (showAddButton ? 1 : 0)

  function onTopSiteContextMenu(topSite: TopSite, event: React.MouseEvent) {
    setContextMenuSite(topSite)
    const elem = rootRef.current
    if (elem) {
      elem.style.setProperty('--self-context-menu-x', event.pageX + 'px')
      elem.style.setProperty('--self-context-menu-y', event.pageY + 'px')
    }
  }

  function topSitesMenuAction(fn: () => void) {
    return () => {
      fn()
      setShowTopSitesMenu(false)
    }
  }

  function onAddTopSite() {
    setEditSite(null)
    setShowEditSite(true)
  }

  if (!showTopSites || tileCount === 0) {
    return null
  }

  return (
    <div ref={rootRef} data-css-scope={style.scope}>
      <div className='top-site-context-menu-anchor' />
      <div className='top-sites'>
        <div className='left-spacer' />
        <TopSitesGrid
          expanded={expanded}
          canAddSite={showAddButton}
          canReorderSites={listKind === TopSitesListKind.kCustom}
          onAddTopSite={onAddTopSite}
          onTopSiteContextMenu={onTopSiteContextMenu}
        />
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
              listKind === TopSitesListKind.kCustom &&
                <button onClick={topSitesMenuAction(onAddTopSite)}>
                  <Icon name='browser-add' />
                  {getString(S.NEW_TAB_ADD_TOP_SITE_LABEL)}
                </button>
            }
            {
              tileCount > collapsedTileColumnCount &&
                <button onClick={topSitesMenuAction(() => {
                  setExpanded(!expanded)
                })}>
                  <Icon name={expanded ? 'contract' : 'expand' } />
                  {
                    expanded
                      ? getString(S.NEW_TAB_TOP_SITES_SHOW_LESS_LABEL)
                      : getString(S.NEW_TAB_TOP_SITES_SHOW_MORE_LABEL)
                  }
                </button>
            }
            <div className='menu-divider' />
            {
              listKind === TopSitesListKind.kCustom ?
                <button onClick={topSitesMenuAction(() =>
                  actions.setTopSitesListKind(TopSitesListKind.kMostVisited))
                }>
                  <Icon name='history' />
                  {getString(S.NEW_TAB_TOP_SITES_SHOW_MOST_VISITED_LABEL)}
                </button> :
                <button onClick={topSitesMenuAction(() =>
                  actions.setTopSitesListKind(TopSitesListKind.kCustom))
                }>
                  <Icon name='star-outline' />
                  {getString(S.NEW_TAB_TOP_SITES_SHOW_CUSTOM_LABEL)}
                </button>
            }
            <div className='menu-divider' />
            <button onClick={topSitesMenuAction(() =>
              actions.setShowTopSites(false))
            }>
              <Icon name='eye-off' />
              {getString(S.NEW_TAB_HIDE_TOP_SITES_LABEL)}
            </button>
          </div>
        </Popover>
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
                  {getString(S.NEW_TAB_EDIT_TOP_SITE_LABEL)}
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
              {getString(S.NEW_TAB_REMOVE_TOP_SITE_LABEL)}
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
