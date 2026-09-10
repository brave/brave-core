/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { useAppState, useAppActions } from '../lib/app_context'
import { Campaign, CampaignDaypart } from '../lib/app_store'
import { renderCopyableText } from '../lib/copyable_text'
import { useCopyToClipboard } from './copy_toast'
import { TabHeader } from './tab_header'

const DAY_LABELS = ['Sun', 'Mon', 'Tue', 'Wed', 'Thu', 'Fri', 'Sat']

// e.g. "13456" (see `CampaignDaypart`) -> "Mon, Wed, Thu, Fri, Sat"; all 7
// digits present is the default (no dayparting configured at all), worth
// calling out explicitly rather than spelling out every day.
function formatDaysOfWeek(daysOfWeek: string) {
  const days = Array.from(daysOfWeek, Number).sort((a, b) => a - b)
  if (days.length === 7) {
    return 'Every day'
  }
  return days.map((day) => DAY_LABELS[day]).join(', ')
}

// Dayparts are stored as minutes since local midnight.
function formatMinuteOfDay(minute: number) {
  const hours = Math.floor(minute / 60)
  const minutes = minute % 60
  return `${String(hours).padStart(2, '0')}:${String(minutes).padStart(2, '0')}`
}

// Start/End are local device time, which isn't obvious from a bare "00:00" -
// shown once in the column headers (e.g. "Start (PST)") rather than
// repeated on every row.
function localTimeZoneAbbreviation() {
  const timeZonePart = new Intl.DateTimeFormat(undefined, {
    timeZoneName: 'short',
  }).formatToParts(new Date()).find((part) => part.type === 'timeZoneName')
  return timeZonePart?.value ?? ''
}

type DaypartRow = CampaignDaypart & { campaignId: string }

function flattenDayparts(campaigns: Campaign[]): DaypartRow[] {
  return campaigns.flatMap((campaign) => campaign['Dayparts'].map(
    (daypart) => ({ ...daypart, campaignId: campaign['Campaign ID'] }),
  ))
}

function DaypartsTable({ data }: { data: Campaign[] }) {
  const copy = useCopyToClipboard()
  const rows = flattenDayparts(data)
  const timeZone = localTimeZoneAbbreviation()

  return (
    <>
      <table>
        <thead>
          <tr>
            <th className='truncate-cell rule-column'>Campaign ID</th>
            <th className='value-column'>Days</th>
            <th className='status-column'>Start {timeZone && `(${timeZone})`}</th>
            <th className='status-column'>End {timeZone && `(${timeZone})`}</th>
          </tr>
        </thead>
        <tbody>
          {rows.map((row, index) => (
            <tr key={index}>
              <td className='truncate-cell rule-column'>
                {renderCopyableText(row.campaignId, `${index}`, copy)}
              </td>
              <td className='value-column'>
                {formatDaysOfWeek(row['Days Of Week'])}
              </td>
              <td className='status-column'>
                {formatMinuteOfDay(row['Start Minute'])}
              </td>
              <td className='status-column'>
                {formatMinuteOfDay(row['End Minute'])}
              </td>
            </tr>
          ))}
        </tbody>
      </table>
      {rows.length === 0 && <p>No active campaigns are currently targeting your region.</p>}
    </>
  )
}

function DaypartsSection({ title, campaigns }: {
  title: string
  campaigns: Campaign[]
}) {
  const rowCount = flattenDayparts(campaigns).length
  return (
    <div className='content-card'>
      <h4>
        <span className='title'>
          {title} <span className='diagnostic-muted'>({rowCount})</span>
        </span>
      </h4>
      <section>
        <DaypartsTable data={campaigns} />
      </section>
    </div>
  )
}

export function DayParts() {
  const actions = useAppActions()
  const notificationAdCampaigns =
    useAppState((state) => state.activeNotificationAdCampaigns)
  const newTabPageAdCampaigns =
    useAppState((state) => state.activeNewTabPageAdCampaigns)
  // Notification ads only run for Rewards users (see resources.tsx).
  const rewardsEnabled = useAppState((state) => state.rewardsEnabled)
  const diagnosticEntries = useAppState((state) => state.diagnosticEntries)
  const notificationAdsEnabled = rewardsEnabled && diagnosticEntries.find(
    (entry) => entry.name === 'Notification ads enabled',
  )?.value === 'true'
  const newTabPageAdsEnabled = diagnosticEntries.find(
    (entry) => entry.name === 'New tab page ads shown',
  )?.value === 'true'

  React.useEffect(() => {
    actions.loadAdsInternals()
    actions.loadDiagnostics()
  }, [])

  return (
    <>
      <TabHeader
        title='Dayparts'
        description='Advertisers can restrict a campaign to specific days and
          hours of the week. This shows the schedule each active campaign is
          currently limited to.'
        onRefresh={actions.loadAdsInternals}
      />
      {notificationAdsEnabled && (
        <DaypartsSection
          title='Notification ads'
          campaigns={notificationAdCampaigns}
        />
      )}
      {newTabPageAdsEnabled && (
        <DaypartsSection
          title='New tab page ads'
          campaigns={newTabPageAdCampaigns}
        />
      )}
    </>
  )
}
