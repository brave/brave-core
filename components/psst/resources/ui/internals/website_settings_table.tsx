// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import Button from '@brave/nala/react/button'
import Icon from '@brave/nala/react/icon'

import { getLocale } from '$web-common/locale'

import * as Mojom from '../api/psst_internals_api'
import { style } from './website_settings_table.style'

import '../strings'

const kMaskedUserId = '••••••••'

const consentStatusNames = new Map<Mojom.ConsentStatus, string>([
  [Mojom.ConsentStatus.kNone, getLocale(S.PSST_INTERNALS_CONSENT_STATUS_NONE)],
  [Mojom.ConsentStatus.kAsk, getLocale(S.PSST_INTERNALS_CONSENT_STATUS_ASK)],
  [
    Mojom.ConsentStatus.kAllow,
    getLocale(S.PSST_INTERNALS_CONSENT_STATUS_ALLOW),
  ],
  [
    Mojom.ConsentStatus.kBlock,
    getLocale(S.PSST_INTERNALS_CONSENT_STATUS_BLOCK),
  ],
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
            <th>{getLocale(S.PSST_INTERNALS_TABLE_HEADER_SITE)}</th>
            <th>
              {getLocale(S.PSST_INTERNALS_TABLE_HEADER_INFOBAR_SHOWN_COUNT)}
            </th>
            <th>
              {getLocale(S.PSST_INTERNALS_TABLE_HEADER_INFOBAR_DECLINE_COUNT)}
            </th>
            <th>{getLocale(S.PSST_INTERNALS_TABLE_HEADER_CONSENT)}</th>
            <th>{getLocale(S.PSST_INTERNALS_TABLE_HEADER_SCRIPT_VERSION)}</th>
            <th>
              <div className='user-id-header'>
                {getLocale(S.PSST_INTERNALS_TABLE_HEADER_USER_ID)}
                <Button
                  kind='plain-faint'
                  size='small'
                  onClick={() => setShowUserId((show) => !show)}
                >
                  <Icon name={showUserId ? 'eye-off' : 'eye-on'} />
                </Button>
              </div>
            </th>
            <th>{getLocale(S.PSST_INTERNALS_TABLE_HEADER_UIDS_PERFORMED)}</th>
          </tr>
        </thead>
        <tbody>
          {entries.map((entry, index) => (
            <tr key={`${entry.site}-${index}`}>
              <td className='site'>{entry.site}</td>
              <td>{entry.infobarShownCount}</td>
              <td>{entry.infobarDeclineCount}</td>
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
        <div className='empty'>
          {getLocale(S.PSST_INTERNALS_WEBSITE_SETTINGS_TABLE_EMPTY)}
        </div>
      )}
    </div>
  )
}
