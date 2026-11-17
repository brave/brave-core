// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import Button from '@brave/nala/react/button'
import Dropdown from '@brave/nala/react/dropdown'
import Icon from '@brave/nala/react/icon'
import Input from '@brave/nala/react/input'
import Toggle from '@brave/nala/react/toggle'

import { getLocale, formatLocale } from '$web-common/locale'

import * as Mojom from '../api/psst_internals_api'
import { type PsstInternalsApi } from '../api/psst_internals_api'
import {
  formatEntriesForClipboard,
  levelNames,
  sourceNames,
} from './log_entry_format'
import { LogTable } from './log_table'
import { WebsiteSettingsTable } from './website_settings_table'
import { style } from './app.style'

import '../strings'

const kAnyFilter = 'any'

export function App({ api }: { api: PsstInternalsApi }) {
  // The mojom module is untyped JS, so annotate what comes back from it.
  const entries: Mojom.PsstLogEntry[] = api.useEntries().data ?? []
  const diagnosticsInfo: Mojom.PsstDiagnosticsInfo | null =
    api.useDiagnosticsInfo().data ?? null
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
        <h1>{getLocale(S.PSST_INTERNALS_PAGE_TITLE)}</h1>
      </header>

      <section>
        <h2>{getLocale(S.PSST_INTERNALS_WEBSITE_SETTINGS_TITLE)}</h2>
        <div className='section-description'>
          {getLocale(S.PSST_INTERNALS_WEBSITE_SETTINGS_DESCRIPTION)}
        </div>

        <div className='count'>
          {formatLocale(S.PSST_INTERNALS_INFOBAR_COUNT, {
            $1: String(diagnosticsInfo?.infobarShownCount ?? 0),
            $2: String(diagnosticsInfo?.infobarDeclineCount ?? 0),
          })}
        </div>

        <WebsiteSettingsTable
          entries={diagnosticsInfo?.websiteSettings ?? []}
        />
      </section>

      <section>
        <h2>{getLocale(S.PSST_INTERNALS_LOG_ENTRIES_TITLE)}</h2>
        <div className='section-description'>
          {getLocale(S.PSST_INTERNALS_LOG_ENTRIES_DESCRIPTION)}
        </div>

        <div className='toolbar'>
          <Input
            className='search'
            placeholder={getLocale(
              S.PSST_INTERNALS_FILTER_MESSAGES_PLACEHOLDER,
            )}
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
            <span slot='label'>
              {getLocale(S.PSST_INTERNALS_MIN_LEVEL_LABEL)}
            </span>
            <leo-option value={kAnyFilter}>
              {getLocale(S.PSST_INTERNALS_ALL_LEVELS_OPTION)}
            </leo-option>
            {[...levelNames].map(([level, name]) => (
              <leo-option
                key={level}
                value={String(level)}
              >
                {formatLocale(S.PSST_INTERNALS_LEVEL_AND_ABOVE, { $1: name })}
              </leo-option>
            ))}
          </Dropdown>

          <Dropdown
            className='source-filter'
            value={source}
            onChange={(e) => setSource(e.value!)}
          >
            <span slot='label'>{getLocale(S.PSST_INTERNALS_SOURCE_LABEL)}</span>
            <leo-option value={kAnyFilter}>
              {getLocale(S.PSST_INTERNALS_ALL_SOURCES_OPTION)}
            </leo-option>
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
            {getLocale(S.PSST_INTERNALS_COPY_BUTTON)}
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
            {getLocale(S.PSST_INTERNALS_CLEAR_BUTTON)}
          </Button>

          <Toggle
            className='live-tail'
            checked={liveTail}
            onChange={(e) => setLiveTail(e.checked)}
          >
            <span slot='label'>
              {getLocale(S.PSST_INTERNALS_FOLLOW_TOGGLE_LABEL)}
            </span>
          </Toggle>
        </div>

        <div className='count'>
          {formatLocale(S.PSST_INTERNALS_SHOWING_ENTRIES_COUNT, {
            $1: String(filtered.length),
            $2: String(entries.length),
          })}
        </div>

        <LogTable
          entries={filtered}
          liveTail={liveTail}
        />
      </section>
    </div>
  )
}
