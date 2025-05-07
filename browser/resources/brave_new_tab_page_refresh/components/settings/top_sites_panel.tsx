/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Icon from '@brave/leo/react/icon'
import Toggle from '@brave/leo/react/toggle'

import { TopSitesListKind } from '../../models/top_sites'
import { useAppActions, useAppState } from '../context/app_model_context'
import { useLocale } from '../context/locale_context'
import classNames from '$web-common/classnames'

import { style } from './top_sites_panel.style'

export function TopSitesPanel() {
  const { getString } = useLocale()
  const actions = useAppActions()

  const showTopSites = useAppState((s) => s.showTopSites)
  const listKind = useAppState((s) => s.topSitesListKind)

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
        <label>{getString('showTopSitesLabel')}</label>
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
              <h4>{getString('topSitesCustomOptionTitle')}</h4>
              <p>{getString('topSitesCustomOptionText')}</p>
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
              <h4>{getString('topSitesMostVisitedOptionTitle')}</h4>
              <p>{getString('topSitesMostVisitedOptionText')}</p>
            </button>
          </div>
        )
      }
    </div>
  )
}
