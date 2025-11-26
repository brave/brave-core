/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Checkbox from '@brave/leo/react/checkbox'
import Icon from '@brave/leo/react/icon'
import Toggle from '@brave/leo/react/toggle'

import { useSearchState, useSearchActions } from '../../context/search_context'
import { getString } from '../../lib/strings'
import { EngineIcon } from '../search/engine_icon'
import { Link } from '../common/link'

import { style } from './search_panel.style'

export function SearchPanel() {
  const actions = useSearchActions()

  const showSearchBox = useSearchState((s) => s.showSearchBox)
  const searchEngines = useSearchState((s) => s.searchEngines)
  const enabledSearchEngines = useSearchState((s) => s.enabledSearchEngines)

  return (
    <div data-css-scope={style.scope}>
      <div className='control-row'>
        <label>{getString(S.NEW_TAB_SHOW_SEARCH_BOX_LABEL)}</label>
        <Toggle
          size='small'
          checked={showSearchBox}
          onChange={({ checked }) => {
            actions.setShowSearchBox(checked)
          }}
        />
      </div>
      {showSearchBox && (
        <div className='search-engines'>
          <h4>{getString(S.NEW_TAB_ENABLED_SEARCH_ENGINES_LABEL)}</h4>
          <div className='search-engine-list'>
            {searchEngines.map((engine) => (
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
            ))}
          </div>
          <div>
            <Link
              className='customize-link'
              url='chrome://settings/searchEngines'
            >
              {getString(S.NEW_TAB_CUSTOMIZE_SEARCH_ENGINES_LINK)}
              <Icon name='launch' />
            </Link>
          </div>
        </div>
      )}
    </div>
  )
}
