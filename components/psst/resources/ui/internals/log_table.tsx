// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import Collapse from '@brave/leo/react/collapse'
import Icon from '@brave/leo/react/icon'

import * as Mojom from '../api/psst_internals_api'
import {
  formatTabId,
  formatTime,
  levelIcons,
  levelNames,
  sourceNames,
} from './log_entry_format'
import { style } from './log_table.style'

interface Props {
  entries: Mojom.PsstLogEntry[]
  // When set, the table keeps the newest row in view as entries arrive.
  liveTail: boolean
}

function DataCell({ data }: { data: string }) {
  const [isOpen, setIsOpen] = React.useState(false)

  let pretty = data
  try {
    pretty = JSON.stringify(JSON.parse(data), null, 2)
  } catch {
    // Leave the payload as-is if it isn't JSON after all.
  }

  return (
    <Collapse
      className='data'
      title='data'
      isOpen={isOpen}
      onToggle={(e) => setIsOpen(e.open)}
    >
      <pre>{pretty}</pre>
    </Collapse>
  )
}

export function LogTable({ entries, liveTail }: Props) {
  const endRef = React.useRef<HTMLDivElement>(null)

  React.useEffect(() => {
    if (liveTail) {
      endRef.current?.scrollIntoView({ block: 'end' })
    }
  }, [liveTail, entries.length])

  return (
    <div data-css-scope={style.scope}>
      <table>
        <thead>
          <tr>
            <th>Time</th>
            <th>Level</th>
            <th>Source</th>
            <th>Tab</th>
            <th>Site</th>
            <th>Message</th>
          </tr>
        </thead>
        <tbody>
          {entries.map((entry, index) => {
            const level = levelNames.get(entry.level) ?? 'info'
            return (
              <tr key={`${entry.timestampMs}-${index}`}>
                <td className='time'>{formatTime(entry.timestampMs)}</td>
                <td>
                  <span className={`level level-${level}`}>
                    <Icon name={levelIcons.get(entry.level) ?? 'info-filled'} />
                    {level}
                  </span>
                </td>
                <td>{sourceNames.get(entry.source) ?? ''}</td>
                <td className='tab'>{formatTabId(entry.tabId)}</td>
                <td className='site'>{entry.site || '—'}</td>
                <td className='message'>
                  {entry.message}
                  {entry.data && <DataCell data={entry.data} />}
                </td>
              </tr>
            )
          })}
        </tbody>
      </table>
      {entries.length === 0 && (
        <div className='empty'>
          No entries. Run a PSST flow on a supported site.
        </div>
      )}
      <div ref={endRef} />
    </div>
  )
}
