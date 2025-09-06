/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Icon from '@brave/leo/react/icon'
import Toggle from '@brave/leo/react/toggle'

import { TopSitesListKind } from '../../state/top_sites_state'
import { useTopSitesState, useTopSitesActions } from '../../context/top_sites_context'
import { getString } from '../../lib/strings'
import classNames from '$web-common/classnames'

import { style } from './top_sites_panel.style'

export function TopSitesPanel() {
  const actions = useTopSitesActions()

  const showTopSites = useTopSitesState((s) => s.showTopSites)
  const listKind = useTopSitesState((s) => s.topSitesListKind)

  function renderSelectedMarker(kind: TopSitesListKind) {
    if (kind === listKind) {
      return (
        <span className='selected-marker'>
          <Icon name='check-normal' />
        </span>
      )
    }
    return null
  }

  return (
    <div data-css-scope={style.scope}>
      <div className='control-row'>
        <label>{getString(S.NEW_TAB_SHOW_TOP_SITES_LABEL)}</label>
        <Toggle
          size='small'
          checked={showTopSites}
          onChange={({ checked }) => { actions.setShowTopSites(checked) }}
        />
      </div>
      {
        showTopSites && (
          <div className='list-view-options'>
            <button
              className={classNames({
                'custom': true,
                'active': listKind === TopSitesListKind.kCustom
              })}
              onClick={() => {
                actions.setTopSitesListKind(TopSitesListKind.kCustom)
              }}
            >
              <div className='list-view-image'>
                {renderSelectedMarker(TopSitesListKind.kCustom)}
              </div>
              <h4>{getString(S.NEW_TAB_TOP_SITES_CUSTOM_OPTION_TITLE)}</h4>
              <p>{getString(S.NEW_TAB_TOP_SITES_CUSTOM_OPTION_TEXT)}</p>
            </button>
            <button
              className={classNames({
                'most-visited': true,
                'active': listKind === TopSitesListKind.kMostVisited
              })}
              onClick={() => {
                actions.setTopSitesListKind(TopSitesListKind.kMostVisited)}
              }
            >
              <div className='list-view-image'>
                {renderSelectedMarker(TopSitesListKind.kMostVisited)}
              </div>
              <h4>
                {getString(S.NEW_TAB_TOP_SITES_MOST_VISITED_OPTION_TITLE)}
              </h4>
              <p>
                {getString(S.NEW_TAB_TOP_SITES_MOST_VISITED_OPTION_TEXT)}
              </p>
            </button>
          </div>
        )
      }
    </div>
  )
}
