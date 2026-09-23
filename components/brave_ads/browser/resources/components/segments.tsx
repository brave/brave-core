/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { useAppState, useAppActions } from '../lib/app_context'
import { Campaign } from '../lib/app_store'
import { renderCopyableText } from '../lib/copyable_text'
import { useCopyToClipboard } from './copy_toast'
import { ReactionIcon } from './reaction_icon'
import { TabHeader } from './tab_header'

interface CreativeSetSegments {
  creativeSetId: string
  segments: string[]
}

// A creative set only ever belongs to one campaign, so flattening across
// both ad formats' campaign lists and keying by creative set id (rather than
// nesting under campaign, the way the Campaigns tab does) is enough to list
// every creative set exactly once.
function collectCreativeSetSegments(campaigns: Campaign[]) {
  const bySetId = new Map<string, CreativeSetSegments>()
  for (const campaign of campaigns) {
    for (const creativeSet of campaign['Creative Sets']) {
      bySetId.set(creativeSet['Creative Set ID'], {
        creativeSetId: creativeSet['Creative Set ID'],
        segments: creativeSet['Segments'],
      })
    }
  }
  return [...bySetId.values()]
}

export function Segments() {
  const actions = useAppActions()
  const copy = useCopyToClipboard()
  const notificationAdCampaigns =
    useAppState((state) => state.activeNotificationAdCampaigns)
  const newTabPageAdCampaigns =
    useAppState((state) => state.activeNewTabPageAdCampaigns)
  const likedSegments = useAppState((state) => state.likedSegments)
  const dislikedSegments = useAppState((state) => state.dislikedSegments)
  const adsMarkedAsInappropriate =
    useAppState((state) => state.adsMarkedAsInappropriate)
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

  const creativeSets = React.useMemo(
    () => collectCreativeSetSegments([
      ...(notificationAdsEnabled ? notificationAdCampaigns : []),
      ...(newTabPageAdsEnabled ? newTabPageAdCampaigns : []),
    ]),
    [
      notificationAdCampaigns, newTabPageAdCampaigns, notificationAdsEnabled,
      newTabPageAdsEnabled,
    ],
  )

  return (
    <>
      <TabHeader
        title={
          <>
            Segments{' '}
            <span className='diagnostic-muted'>({creativeSets.length})</span>
          </>
        }
        description='The segments each active creative set is targeting.'
        onRefresh={actions.loadAdsInternals}
      />

      <div className='content-card'>
        <section>
          <table>
            <thead>
              <tr>
                <th className='truncate-cell wide-column'>Creative Set ID</th>
                <th>Segments</th>
              </tr>
            </thead>
            <tbody>
              {creativeSets.map(({ creativeSetId, segments }) => (
                <tr key={creativeSetId}>
                  <td className='truncate-cell wide-column'>
                    <span className='id-with-reaction'>
                      {renderCopyableText(creativeSetId, creativeSetId, copy)}
                      {adsMarkedAsInappropriate.includes(creativeSetId) && (
                        <ReactionIcon type='inappropriate' />
                      )}
                    </span>
                  </td>
                  <td>
                    {segments.length === 0
                      ? <span className='diagnostic-muted'>None</span>
                      : segments.map((segment, index) => (
                        <React.Fragment key={segment}>
                          {index > 0 && ', '}
                          <span className='id-with-reaction'>
                            {segment}
                            {likedSegments.includes(segment) && (
                              <ReactionIcon type='liked' />
                            )}
                            {dislikedSegments.includes(segment) && (
                              <ReactionIcon type='disliked' />
                            )}
                          </span>
                        </React.Fragment>
                      ))}
                  </td>
                </tr>
              ))}
            </tbody>
          </table>
          {creativeSets.length === 0 && (
            <p>No active creative sets are currently targeting your region.</p>
          )}
        </section>
      </div>
    </>
  )
}
