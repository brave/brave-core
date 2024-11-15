/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Checkbox from '@brave/leo/react/checkbox'
import Icon from '@brave/leo/react/icon'
import Toggle from '@brave/leo/react/toggle'

import { useSearchModel, useSearchState } from '../search_context'
import { useLocale } from '../locale_context'
import { EngineIcon } from '../search/engine_icon'

import { style } from './search_panel.style'

export function SearchPanel() {
  const { getString } = useLocale()
  const model = useSearchModel()

  const [
    showSearchBox,
    searchEngines,
    enabledSearchEngines
  ] = useSearchState((state) => [
    state.showSearchBox,
    state.searchEngines,
    state.enabledSearchEngines
  ])

  return (
    <div {...style}>
      <div className='toggle-row'>
        <label>{getString('showSearchBoxLabel')}</label>
        <Toggle
          size='small'
          checked={showSearchBox}
          onChange={({ checked }) => { model.setShowSearchBox(checked) }}
        />
      </div>
      {
        showSearchBox && <>
          <h4>{getString('enabledSearchEnginesLabel')}</h4>
          <div className='divider' />
          <div className='search-engines'>
            {
              searchEngines.map((engine) => {
                return (
                  <Checkbox
                    key={engine.host}
                    checked={enabledSearchEngines.has(engine.host)}
                    onChange={({ checked }) => {
                      model.setSearchEngineEnabled(engine.host, checked)
                    }}
                  >
                    <span className='engine-name'>{engine.name}</span>
                    <EngineIcon engine={engine} />
                  </Checkbox>
                )
              })
            }
          </div>
          <div className='divider' />
          <div>
            <a
              className='customize-link'
              href='chrome://settings/searchEngines'
              rel='noopener noreferrer'
            >
              {getString('customizeSearchEnginesLink')}
              <Icon name='launch' />
            </a>
          </div>
        </>
      }
    </div>
  )
}
