// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import Button from '@brave/leo/react/button'
import Dropdown from '@brave/leo/react/dropdown'
import Icon from '@brave/leo/react/icon'
import Input from '@brave/leo/react/input'
import Toggle from '@brave/leo/react/toggle'

import * as Mojom from '../api/psst_internals_api'
import { type PsstInternalsApi } from '../api/psst_internals_api'
import {
  formatEntriesForClipboard,
  levelNames,
  sourceNames,
} from './log_entry_format'
import { LogTable } from './log_table'
import { style } from './app.style'

const kAnyFilter = 'any'

export function App({ api }: { api: PsstInternalsApi }) {
  // The mojom module is untyped JS, so annotate what comes back from it.
  const entries: Mojom.PsstLogEntry[] = api.useEntries().data ?? []
  const [search, setSearch] = React.useState('')
  const [minLevel, setMinLevel] = React.useState<string>(kAnyFilter)
  const [source, setSource] = React.useState<string>(kAnyFilter)
  const [liveTail, setLiveTail] = React.useState(true)

  const filtered = React.useMemo(() => {
    const needle = search.trim().toLowerCase()
    return entries.filter((entry) => {
      if (minLevel !== kAnyFilter && entry.level < Number(minLevel)) {
        return false
      }
      if (source !== kAnyFilter && entry.source !== Number(source)) {
        return false
      }
      if (!needle) {
        return true
      }
      return (
        entry.message.toLowerCase().includes(needle)
        || !!entry.tabId?.toString().includes(needle)
        || entry.site.toLowerCase().includes(needle)
        || (entry.data ?? '').toLowerCase().includes(needle)
      )
    })
  }, [entries, search, minLevel, source])

  return (
    <div data-css-scope={style.scope}>
      <header>
        <h1>PSST Internals</h1>
        <div className='subtitle'>
          In-memory diagnostics for the Privacy Settings Tuning flow in this
          profile. Entries are not persisted and are lost when the browser
          exits.
        </div>
      </header>

      <div className='toolbar'>
        <Input
          className='search'
          placeholder='Filter messages'
          value={search}
          onInput={(e) => setSearch(e.value)}
        >
          <Icon
            name='search'
            slot='left-icon'
          />
        </Input>

        <Dropdown
          className='level-filter'
          value={minLevel}
          onChange={(e) => setMinLevel(e.value!)}
        >
          <span slot='label'>Min level</span>
          <leo-option value={kAnyFilter}>All levels</leo-option>
          {[...levelNames].map(([level, name]) => (
            <leo-option
              key={level}
              value={String(level)}
            >
              {name} and above
            </leo-option>
          ))}
        </Dropdown>

        <Dropdown
          className='source-filter'
          value={source}
          onChange={(e) => setSource(e.value!)}
        >
          <span slot='label'>Source</span>
          <leo-option value={kAnyFilter}>All sources</leo-option>
          {[...sourceNames].map(([value, name]) => (
            <leo-option
              key={value}
              value={String(value)}
            >
              {name}
            </leo-option>
          ))}
        </Dropdown>

        <Button
          kind='outline'
          size='small'
          onClick={() =>
            navigator.clipboard.writeText(formatEntriesForClipboard(filtered))
          }
        >
          <Icon
            name='copy'
            slot='icon-before'
          />
          Copy
        </Button>

        <Button
          kind='outline'
          size='small'
          onClick={api.clear}
        >
          <Icon
            name='trash'
            slot='icon-before'
          />
          Clear
        </Button>

        <Toggle
          className='live-tail'
          checked={liveTail}
          onChange={(e) => setLiveTail(e.checked)}
        >
          <span slot='label'>Follow</span>
        </Toggle>
      </div>

      <div className='count'>
        Showing {filtered.length} of {entries.length} entries
      </div>

      <LogTable
        entries={filtered}
        liveTail={liveTail}
      />
    </div>
  )
}
