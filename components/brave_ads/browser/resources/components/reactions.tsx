/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Link from '@brave/leo/react/link'

import { useAppState, useAppActions } from '../lib/app_context'
import { renderCopyableText } from '../lib/copyable_text'
import { useCopyToClipboard } from './copy_toast'
import { ReactionIcon } from './reaction_icon'
import { TabHeader } from './tab_header'

// Explains what each icon shown elsewhere in the UI (Campaigns, Segments)
// means, since the icon alone doesn't say which of the lists below it came
// from.
const REACTION_ICON_LEGEND: Array<{
  type: 'liked' | 'disliked' | 'saved' | 'inappropriate'
  label: string
}> = [
  { type: 'liked', label: 'Liked' },
  { type: 'disliked', label: 'Disliked' },
  { type: 'saved', label: 'Saved' },
  { type: 'inappropriate', label: 'Marked as inappropriate' },
]

// Rendered inline within the tab description, e.g. "... Liked, Disliked,
// Saved, and Marked as inappropriate reactions are flagged with these icons
// elsewhere in this UI."
function ReactionIconLegendPhrase() {
  return (
    <>
      {REACTION_ICON_LEGEND.map(({ type, label }, index) => (
        <React.Fragment key={type}>
          {index > 0 && (
            index === REACTION_ICON_LEGEND.length - 1 ? ', and ' : ', '
          )}
          <span className='inline-icon-value'>
            <ReactionIcon type={type} />
            {label}
          </span>
        </React.Fragment>
      ))}
    </>
  )
}

function IdList({ column, emptyMessage, ids }: {
  column: string
  emptyMessage: string
  ids: string[]
}) {
  const copy = useCopyToClipboard()

  return (
    <>
      <table>
        <thead>
          <tr>
            <th className='truncate-cell'>{column}</th>
          </tr>
        </thead>
        <tbody>
          {ids.map((id) => (
            <tr key={id}>
              <td className='truncate-cell'>
                {renderCopyableText(id, id, copy)}
              </td>
            </tr>
          ))}
        </tbody>
      </table>
      {ids.length === 0 && <p>{emptyMessage}</p>}
    </>
  )
}

function ReactionsCard(
  { title, column, emptyMessage, ids }: {
    title: string
    column: string
    emptyMessage: string
    ids: string[]
  },
) {
  return (
    <div className='content-card'>
      <h4>
        <span className='title'>
          {title} <span className='diagnostic-muted'>({ids.length})</span>
        </span>
      </h4>

      <section>
        <IdList
          column={column}
          emptyMessage={emptyMessage}
          ids={ids}
        />
      </section>
    </div>
  )
}

export function Reactions() {
  const actions = useAppActions()
  const likedAds = useAppState((state) => state.likedAds)
  const dislikedAds = useAppState((state) => state.dislikedAds)
  const likedSegments = useAppState((state) => state.likedSegments)
  const dislikedSegments = useAppState((state) => state.dislikedSegments)
  const savedAds = useAppState((state) => state.savedAds)
  const adsMarkedAsInappropriate = useAppState(
    (state) => state.adsMarkedAsInappropriate,
  )
  const retentionDays = useAppState(
    (state) => state.adHistoryRetentionPeriodDays,
  )

  React.useEffect(() => {
    actions.loadAdsInternals()
  }, [])

  return (
    <div className='card-group'>
      <TabHeader
        title='Reactions'
        description={
          <>
            Reactions recorded via the "{retentionDays}-day Ads History"
            button on{' '}
            <Link
              href='brave://rewards'
              target='_blank'
              rel='noopener noreferrer'
            >
              brave://rewards
            </Link>{' '}
            over the last {retentionDays} days.{' '}
            <ReactionIconLegendPhrase />{' '}
            reactions are shown with these icons on other tabs, next to the
            relevant ad ID or segment.
          </>
        }
        onRefresh={actions.loadAdsInternals}
      />
      <ReactionsCard
        title='Liked advertisers'
        column='Advertiser ID'
        emptyMessage='No advertisers have been liked.'
        ids={likedAds}
      />
      <ReactionsCard
        title='Disliked advertisers'
        column='Advertiser ID'
        emptyMessage='No advertisers have been disliked.'
        ids={dislikedAds}
      />
      <ReactionsCard
        title='Liked segments'
        column='Segment'
        emptyMessage='No segments have been liked.'
        ids={likedSegments}
      />
      <ReactionsCard
        title='Disliked segments'
        column='Segment'
        emptyMessage='No segments have been disliked.'
        ids={dislikedSegments}
      />
      <ReactionsCard
        title='Saved ads'
        column='Creative Instance ID'
        emptyMessage='No ads have been saved.'
        ids={savedAds}
      />
      <ReactionsCard
        title='Marked as inappropriate'
        column='Creative Set ID'
        emptyMessage='No ads have been marked as inappropriate.'
        ids={adsMarkedAsInappropriate}
      />
    </div>
  )
}
