/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Icon from '@brave/leo/react/icon'
import Toggle from '@brave/leo/react/toggle'

import { TopSitesListKind } from '../../models/top_sites_model'
import { useTopSitesModel, useTopSitesState } from '../top_sites_context'
import { useLocale } from '../locale_context'

import { style } from './top_sites_panel.style'

export function TopSitesPanel() {
  const { getString } = useLocale()
  const model = useTopSitesModel()

  const [
    showTopSites,
    listKind
  ] = useTopSitesState((state) => [
    state.showTopSites,
    state.listKind
  ])

  function listOptionClassName(kind: TopSitesListKind) {
    if (kind === listKind) {
      return kind + ' active'
    }
    return kind
  }

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
    <div {...style}>
      <div className='toggle-row'>
        <label>{getString('showTopSitesLabel')}</label>
        <Toggle
          size='small'
          checked={showTopSites}
          onChange={({ checked }) => { model.setShowTopSites(checked) }}
        />
      </div>
      {
        showTopSites && (
          <div className='list-view-options'>
            <button
              className={listOptionClassName('custom')}
              onClick={() => model.setListKind('custom')}
            >
              <div className='list-view-image'>
                {renderSelectedMarker('custom')}
              </div>
              <h4>{getString('topSitesCustomOptionTitle')}</h4>
              <p>{getString('topSitesCustomOptionText')}</p>
            </button>
            <button
              className={listOptionClassName('most-visited')}
              onClick={() => model.setListKind('most-visited')}
            >
              <div className='list-view-image'>
                {renderSelectedMarker('most-visited')}
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
