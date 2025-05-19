/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Checkbox from '@brave/leo/react/checkbox'
import Icon from '@brave/leo/react/icon'
import Toggle from '@brave/leo/react/toggle'

import { useAppActions, useAppState } from '../context/app_model_context'
import { getString } from '../../lib/strings'
import { EngineIcon } from '../search/engine_icon'
import { Link } from '../common/link'

import { style } from './search_panel.style'

export function SearchPanel() {
  const actions = useAppActions()

  const showSearchBox = useAppState((s) => s.showSearchBox)
  const searchEngines = useAppState((s) => s.searchEngines)
  const enabledSearchEngines = useAppState((s) => s.enabledSearchEngines)

  return (
    <div data-css-scope={style.scope}>
      <div className='control-row'>
        <label>{getString('showSearchBoxLabel')}</label>
        <Toggle
          size='small'
          checked={showSearchBox}
          onChange={({ checked }) => { actions.setShowSearchBox(checked) }}
        />
      </div>
      {
        showSearchBox &&
          <div className='search-engines'>
            <h4>{getString('enabledSearchEnginesLabel')}</h4>
            <div className='search-engine-list'>
              {
                searchEngines.map((engine) => (
                  <Checkbox
                    key={engine.host}
                    checked={enabledSearchEngines.has(engine.host)}
                    onChange={({ checked }) => {
                      actions.setSearchEngineEnabled(engine.host, checked)
                    }}
                  >
                    <span className='engine-name'>{engine.name}</span>
                    <EngineIcon engine={engine} />
                  </Checkbox>
                ))
              }
            </div>
            <div>
              <Link
                className='customize-link'
                url='chrome://settings/searchEngines'
              >
                {getString('customizeSearchEnginesLink')}
                <Icon name='launch' />
              </Link>
            </div>
          </div>
      }
    </div>
  )
}
