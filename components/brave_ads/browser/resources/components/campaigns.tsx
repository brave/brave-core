/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { useAppState, useAppActions } from '../lib/app_context'
import { Campaign, CampaignCreative, CampaignCreativeSet } from '../lib/app_store'
import { renderCopyableText } from '../lib/copyable_text'
import {
  formatRelativeDuration,
  formatUnixEpochToLocalTime,
  nowInSeconds,
} from '../lib/format_time'
import { useCopyToClipboard } from './copy_toast'
import { ReactionIcon } from './reaction_icon'
import { TabHeader } from './tab_header'

// 3 decimal places, avoiding raw floating point noise like
// "0.7100000000000002" (same rationale as `valueFormatter` elsewhere).
const passThroughRateFormatter = new Intl.NumberFormat(undefined, {
  minimumFractionDigits: 3,
  maximumFractionDigits: 3,
})

// Column widths are keyed to a fixed per-ad-format column set (below) rather
// than derived from which fields happen to be populated on `data`. Deriving
// per-table would let column widths (and even which columns appear at all)
// drift between creative sets of the same ad format, e.g. if one creative
// set's only creative happens to be missing an Alt.
type AdFormat = 'notificationAds' | 'newTabPageAds'

const AD_FORMAT_COLUMNS: Record<AdFormat, Array<{
  key: keyof CampaignCreative
  label: string
  widthClass: string
}>> = {
  notificationAds: [
    { key: 'Title', label: 'Title', widthClass: 'value-column' },
    { key: 'Body', label: 'Body', widthClass: 'value-column' },
  ],
  newTabPageAds: [
    { key: 'Company Name', label: 'Company Name', widthClass: 'value-column' },
    { key: 'Alt', label: 'Alt', widthClass: 'value-column' },
    { key: 'Dynamic/Static', label: 'Dynamic/Static', widthClass: 'status-column' },
  ],
}

function CreativesTable({ adFormat, data }: {
  adFormat: AdFormat
  data: CampaignCreative[]
}) {
  const copy = useCopyToClipboard()
  const columns = AD_FORMAT_COLUMNS[adFormat]

  return (
    <>
      <table>
        <thead>
          <tr>
            <th className='truncate-cell wide-id-column'>Creative Instance ID</th>
            {columns.map(({ key, label, widthClass }) => (
              <th key={key} className={`truncate-cell ${widthClass}`}>
                {label}
              </th>
            ))}
            <th className='truncate-cell wide-id-column'>Target URL</th>
          </tr>
        </thead>
        <tbody>
          {data.map((creative) => {
            const id = creative['Creative Instance ID']
            return (
              <tr key={id}>
                <td className='truncate-cell wide-id-column'>
                  {renderCopyableText(id, id, copy)}
                </td>
                {columns.map(({ key, widthClass }) => (
                  <td key={key} className={`truncate-cell ${widthClass}`}>
                    {creative[key]}
                  </td>
                ))}
                <td className='truncate-cell wide-id-column'>
                  {renderCopyableText(
                    creative['Target URL'], `${id}-url`, copy)}
                </td>
              </tr>
            )
          })}
        </tbody>
      </table>
      {data.length === 0 && <p>No creatives in this creative set.</p>}
    </>
  )
}

function campaignStatus(campaign: Campaign) {
  const nowSeconds = nowInSeconds()
  if (nowSeconds < campaign['Start At']) {
    return { label: 'Not started', className: 'campaign-not-started' }
  }
  if (nowSeconds >= campaign['End At']) {
    return { label: 'Ended', className: 'campaign-ended' }
  }
  return { label: 'Active', className: 'campaign-active' }
}

// 0 means unlimited (see `CreativeAdInfo`/`CatalogCampaignInfo` in
// creative_ad_info.h/catalog_campaign_info.h). Reaching the cap isn't an
// error (it's the cap doing its job), but it's worth flagging since it
// explains why this isn't currently being served.
function capClassName(cap: number, served: number) {
  if (cap === 0) {
    return 'diagnostic-muted'
  }
  return served >= cap ? 'condition-matcher-no-match' : ''
}

// Served count is only meaningful next to a real cap; an unlimited cap has
// nothing to be "used up". Muted when the cap isn't reached, but once it is,
// the whole value (including "capped") should read as the problem it is,
// not just the number.
function CapValue({ cap, served }: { cap: number, served: number }) {
  if (cap === 0) {
    return <span className='diagnostic-muted'>Unlimited</span>
  }
  const isCapped = served >= cap
  return (
    <span className={capClassName(cap, served)}>
      {cap}{' '}
      <span className={isCapped ? '' : 'diagnostic-muted'}>
        ({served} served{isCapped ? ', capped' : ''})
      </span>
    </span>
  )
}

function CreativeSetSection({ adFormat, creativeSet }: {
  adFormat: AdFormat
  creativeSet: CampaignCreativeSet
}) {
  const copy = useCopyToClipboard()
  const creativeSetId = creativeSet['Creative Set ID']
  const adsMarkedAsInappropriate =
    useAppState((state) => state.adsMarkedAsInappropriate)
  const isMarkedAsInappropriate =
    adsMarkedAsInappropriate.includes(creativeSetId)

  return (
    <section className='nested-section'>
      <p className='subsection-title'>
        Creative set{' '}
        <span className='id-with-reaction'>
          {renderCopyableText(creativeSetId, creativeSetId, copy)}
          {isMarkedAsInappropriate && <ReactionIcon type='inappropriate' />}
        </span>{' '}
        <span className='diagnostic-muted'>
          ({creativeSet['Creatives'].length})
        </span>
      </p>
      <section className='key-value-list'>
        <div>
          <span>Per Day</span>
          <CapValue
            cap={creativeSet['Per Day']}
            served={creativeSet['Per Day Served']}
          />
        </div>
        <div>
          <span>Per Week</span>
          <CapValue
            cap={creativeSet['Per Week']}
            served={creativeSet['Per Week Served']}
          />
        </div>
        <div>
          <span>Per Month</span>
          <CapValue
            cap={creativeSet['Per Month']}
            served={creativeSet['Per Month Served']}
          />
        </div>
        <div>
          <span>Total Max</span>
          <CapValue
            cap={creativeSet['Total Max']}
            served={creativeSet['Total Max Served']}
          />
        </div>
      </section>
      <CreativesTable adFormat={adFormat} data={creativeSet['Creatives']} />
    </section>
  )
}

function CampaignCard({ adFormat, campaign }: {
  adFormat: AdFormat
  campaign: Campaign
}) {
  const copy = useCopyToClipboard()
  const campaignId = campaign['Campaign ID']
  const advertiserId = campaign['Advertiser ID']
  const status = campaignStatus(campaign)
  const dislikedAds = useAppState((state) => state.dislikedAds)
  const isDisliked = dislikedAds.includes(advertiserId)
  const likedAds = useAppState((state) => state.likedAds)
  const isLiked = likedAds.includes(advertiserId)

  return (
    <div className='content-card'>
      <h4>
        <span className='title title-own-line'>
          Campaign {renderCopyableText(campaignId, campaignId, copy)}
        </span>
        <span className='title title-own-line'>
          Advertiser{' '}
          <span className='id-with-reaction'>
            {renderCopyableText(advertiserId, advertiserId, copy)}
            {isLiked && <ReactionIcon type='liked' />}
            {isDisliked && <ReactionIcon type='disliked' />}
          </span>
        </span>
      </h4>

      <section className='key-value-list'>
        <div>
          <span>Status</span>
          <span className={status.className}>{status.label}</span>
        </div>
        <div>
          <span>Metric Type</span>
          <span>{campaign['Metric Type']}</span>
        </div>
        <div>
          <span>Start At</span>
          <span>
            {formatUnixEpochToLocalTime(campaign['Start At'])}{' '}
            <span className='diagnostic-muted'>
              ({formatRelativeDuration(campaign['Start At'])})
            </span>
          </span>
        </div>
        <div>
          <span>End At</span>
          <span>
            {formatUnixEpochToLocalTime(campaign['End At'])}{' '}
            <span className='diagnostic-muted'>
              ({formatRelativeDuration(campaign['End At'])})
            </span>
          </span>
        </div>
        <div>
          <span>Priority</span>
          <span>{campaign['Priority']}</span>
        </div>
        <div>
          <span>Pass Through Rate</span>
          <span>
            {passThroughRateFormatter.format(campaign['Pass Through Rate'])}
          </span>
        </div>
        <div>
          <span>Daily Cap</span>
          <CapValue
            cap={campaign['Daily Cap']}
            served={campaign['Daily Cap Served']}
          />
        </div>
      </section>

      {campaign['Creative Sets'].map((creativeSet, index) => (
        <React.Fragment key={creativeSet['Creative Set ID']}>
          {index > 0 && <hr className='diagnostic-divider' />}
          <CreativeSetSection adFormat={adFormat} creativeSet={creativeSet} />
        </React.Fragment>
      ))}
    </div>
  )
}

function CampaignsSection({ adFormat, title, source, adCount, campaigns, gracePeriodEndAt }: {
  adFormat: AdFormat
  title: string
  source: string
  adCount: number
  campaigns: Campaign[]
  gracePeriodEndAt?: number | null
}) {
  const sorted = React.useMemo(
    () => [...campaigns].sort((a, b) =>
      a['Priority'] - b['Priority'] || b['Start At'] - a['Start At']),
    [campaigns],
  )
  const nowSeconds = nowInSeconds()
  const isInGracePeriod =
    gracePeriodEndAt != null && nowSeconds < gracePeriodEndAt

  return (
    <div className='card-group'>
      <div className='content-card'>
        <h4>
          <span className='title'>
            Active {title} <span className='diagnostic-muted'>({adCount})</span>
          </span>
        </h4>
        <p>{source}</p>
        {gracePeriodEndAt != null && (
          <p>
            {isInGracePeriod
              ? <>
                  New tab page ads will not be served until the grace period
                  ends on{' '}
                  <span className='campaign-not-started'>
                    {formatUnixEpochToLocalTime(gracePeriodEndAt)}
                  </span>{' '}
                  <span className='diagnostic-muted'>
                    ({formatRelativeDuration(gracePeriodEndAt)})
                  </span>.
                </>
              : <>
                  <span className='campaign-active'>
                    Grace period ended on{' '}
                    {formatUnixEpochToLocalTime(gracePeriodEndAt)}
                  </span>{' '}
                  <span className='diagnostic-muted'>
                    ({formatRelativeDuration(gracePeriodEndAt)})
                  </span>.
                </>}
          </p>
        )}
        {sorted.length === 0 && (
          <p>No active campaigns are currently targeting your region.</p>
        )}
      </div>
      {sorted.map((campaign) => (
        <CampaignCard
          key={campaign['Campaign ID']}
          adFormat={adFormat}
          campaign={campaign}
        />
      ))}
    </div>
  )
}

export function Campaigns() {
  const actions = useAppActions()
  const notificationAdCount = useAppState((state) => state.activeNotificationAdCount)
  const newTabPageAdCount = useAppState((state) => state.activeNewTabPageAdCount)
  const notificationAdCampaigns =
    useAppState((state) => state.activeNotificationAdCampaigns)
  const newTabPageAdCampaigns =
    useAppState((state) => state.activeNewTabPageAdCampaigns)
  const newTabPageAdGracePeriodEndAt =
    useAppState((state) => state.newTabPageAdGracePeriodEndAt)
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
        title='Campaigns'
        description='Every campaign currently loaded, grouped by ad format.'
        onRefresh={actions.loadAdsInternals}
      />
      {notificationAdsEnabled && (
        <CampaignsSection
          adFormat='notificationAds'
          title='notification ads'
          source='Sourced from the ad catalog.'
          adCount={notificationAdCount}
          campaigns={notificationAdCampaigns}
        />
      )}
      {newTabPageAdsEnabled && (
        <CampaignsSection
          adFormat='newTabPageAds'
          title='new tab page ads'
          source='Sourced from the sponsored images component.'
          adCount={newTabPageAdCount}
          campaigns={newTabPageAdCampaigns}
          gracePeriodEndAt={newTabPageAdGracePeriodEndAt}
        />
      )}
    </>
  )
}
