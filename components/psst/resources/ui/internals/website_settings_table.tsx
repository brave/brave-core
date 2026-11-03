// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import Button from '@brave/leo/react/button'
import Icon from '@brave/leo/react/icon'

import * as Mojom from '../api/psst_internals_api'
import { style } from './website_settings_table.style'

const kMaskedUserId = '••••••••'

const consentStatusNames = new Map<Mojom.ConsentStatus, string>([
  [Mojom.ConsentStatus.kNone, 'none'],
  [Mojom.ConsentStatus.kAsk, 'ask'],
  [Mojom.ConsentStatus.kAllow, 'allow'],
  [Mojom.ConsentStatus.kBlock, 'block'],
])

interface Props {
  entries: Mojom.PsstWebsiteSettingsEntry[]
}

export function WebsiteSettingsTable({ entries }: Props) {
  const [showUserId, setShowUserId] = React.useState(false)

  return (
    <div data-css-scope={style.scope}>
      <table>
        <thead>
          <tr>
            <th>Site</th>
            <th>Consent</th>
            <th>Script version</th>
            <th>
              <div className='user-id-header'>
                User ID
                <Button
                  kind='plain-faint'
                  size='small'
                  onClick={() => setShowUserId((show) => !show)}
                >
                  <Icon name={showUserId ? 'eye-off' : 'eye-on'} />
                </Button>
              </div>
            </th>
            <th>UIDs performed</th>
          </tr>
        </thead>
        <tbody>
          {entries.map((entry, index) => (
            <tr key={`${entry.site}-${index}`}>
              <td className='site'>{entry.site}</td>
              <td>{consentStatusNames.get(entry.consentStatus) ?? ''}</td>
              <td>{entry.scriptVersion}</td>
              <td className='user-id'>
                {showUserId ? entry.userId : kMaskedUserId}
              </td>
              <td>{entry.uidsToPerform.join(', ') || '—'}</td>
            </tr>
          ))}
        </tbody>
      </table>
      {entries.length === 0 && (
        <div className='empty'>No per-site PSST settings recorded.</div>
      )}
    </div>
  )
}
