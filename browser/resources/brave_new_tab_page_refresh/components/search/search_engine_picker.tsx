/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Button from '@brave/leo/react/button'
import ButtonMenu from '@brave/leo/react/buttonMenu'

import { getString } from '../../lib/strings'
import { SearchEngineInfo } from '../../state/search_state'
import { EngineIcon } from './engine_icon'

import { style } from './search_engine_picker.style'

interface Props {
  selectedEngine: SearchEngineInfo | null
  searchEngines: SearchEngineInfo[]
  onSelectEngine: (engine: SearchEngineInfo) => void
  onCustomizeClick: () => void
}

export function SearchEnginePicker(props: Props) {
  const { searchEngines, selectedEngine } = props
  return (
    <div data-css-scope={style.scope}>
      <ButtonMenu>
        <Button
          className='engine-picker-button'
          fab
          kind='plain-faint'
          slot='anchor-content'
        >
          {selectedEngine && <EngineIcon engine={selectedEngine} />}
        </Button>
        {searchEngines.map((engine) => (
          <leo-menu-item
            key={engine.host}
            onClick={() => props.onSelectEngine(engine)}
          >
            <EngineIcon engine={engine} />
            {engine.name}
          </leo-menu-item>
        ))}
        <div className='divider' />
        <leo-menu-item
          onClick={props.onCustomizeClick}
          data-customize='customize'
        >
          {getString(S.NEW_TAB_SEARCH_CUSTOMIZE_ENGINE_LIST_TEXT)}
        </leo-menu-item>
      </ButtonMenu>
    </div>
  )
}
