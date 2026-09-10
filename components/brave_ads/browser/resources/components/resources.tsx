/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Link from '@brave/leo/react/link'

import { useAppState, useAppActions } from '../lib/app_context'
import { Campaign, DiagnosticEntry } from '../lib/app_store'
import { CopyableSpan, renderCopyableText } from '../lib/copyable_text'
import { splitTrailingParenthetical } from '../lib/format_time'
import { useCopyToClipboard } from './copy_toast'
import { TabHeader } from './tab_header'

// brave://components doesn't exist on iOS.
// <if expr="is_ios">
const COMPONENTS_LINK_SUPPORTED = false
// </if>
// <if expr="!is_ios">
const COMPONENTS_LINK_SUPPORTED = true
// </if>

// `entryName` matches the backend `DiagnosticEntry` name; `title` drops the
// "resource" suffix since the tab itself is already "Resources".
const RESOURCES: Array<{ entryName: string, title: string }> = [
  { entryName: 'Purchase intent resource', title: 'Purchase intent' },
  { entryName: 'Anti targeting resource', title: 'Anti targeting' },
]

function ResourceCard({ title, value, componentId }: {
  title: string
  value: string
  componentId: string
}) {
  // An empty `componentId` means `GetComponent()` found no component-updater
  // component registered for the current country/language at all. A
  // non-empty `componentId` with a `value` of "Not loaded" means the
  // component is registered but the resource itself hasn't parsed for this
  // specific country/language; both cases are effectively unavailable for
  // the current region/language, which the enclosing group heading already
  // names.
  const isUnavailable = !componentId || value === 'Not loaded'
  const status = isUnavailable ? 'Unavailable' : value
  const copy = useCopyToClipboard()

  return (
    <div className='content-card'>
      <h4>
        <span className='title'>{title}</span>
      </h4>

      <section className='key-value-list'>
        <div>
          <span>Status</span>
          <span className={isUnavailable ? 'diagnostic-muted' : ''}>
            {status}
          </span>
        </div>
        <div>
          <span>ID</span>
          <span>
            {componentId
              ? renderCopyableText(componentId, componentId, copy)
              : 'N/A'}
          </span>
        </div>
      </section>
    </div>
  )
}

// Past this many codes, showing them all inline would dominate the heading;
// collapse to a count + short preview with a click-to-expand instead.
const REGION_PREVIEW_COUNT = 5

function RegionSummary({ codes, fallback }: {
  codes: string[]
  fallback: string
}) {
  const [expanded, setExpanded] = React.useState(false)

  if (codes.length === 0) {
    return <>({fallback || 'N/A'})</>
  }
  if (expanded || codes.length <= REGION_PREVIEW_COUNT) {
    return <>({codes.join(', ')})</>
  }

  return (
    <>
      ({codes.slice(0, REGION_PREVIEW_COUNT).join(', ')}...{' '}
      <span className='text-link' onClick={() => setExpanded(true)}>
        show all
      </span>)
    </>
  )
}

// Sourced from the ad catalog rather than the component updater, so it
// doesn't fit `ResourceCard`'s Status/ID shape. Only shown when notification
// ads are enabled, same as `RESOURCES` above, since the catalog only feeds
// notification ad campaigns.
function CatalogCard({
  geoTargets,
  fallbackCountryCode,
  diagnosticEntries,
  resourcesDiagnosticEntries,
}: {
  geoTargets: string[]
  fallbackCountryCode: string
  diagnosticEntries: DiagnosticEntry[]
  resourcesDiagnosticEntries: DiagnosticEntry[]
}) {
  const copy = useCopyToClipboard()
  const catalogId =
    diagnosticEntries.find((entry) => entry.name === 'Catalog ID')?.value ?? ''
  const catalogVersion =
    resourcesDiagnosticEntries.find((entry) => entry.name === 'Catalog Version')
      ?.value ?? 'N/A'
  const lastUpdated =
    diagnosticEntries.find((entry) => entry.name === 'Catalog last updated')
      ?.value ?? 'N/A'
  const nextUpdate =
    resourcesDiagnosticEntries.find(
      (entry) => entry.name === 'Catalog next update',
    )?.value ?? 'N/A'
  // Backend appends "(expires in N days)"/"(N days overdue)" to the friendly
  // date/time; flag the overdue case as a problem.
  const isOverdue = lastUpdated.endsWith('overdue)')
  const lastUpdatedSplit = splitTrailingParenthetical(lastUpdated)
  const nextUpdateSplit = splitTrailingParenthetical(nextUpdate)

  return (
    <div className='content-card'>
      <h4>
        <span className='title'>
          Catalog{' '}
          <RegionSummary codes={geoTargets} fallback={fallbackCountryCode} />
        </span>
      </h4>
      <section className='key-value-list'>
        <div>
          <span>ID</span>
          <span>
            {catalogId
              ? (
                <CopyableSpan
                  value={catalogId}
                  className='copyable-text'
                  onCopy={copy}
                />
              )
              : 'N/A'}
          </span>
        </div>
        <div>
          <span>Schema version</span>
          <span>{catalogVersion}</span>
        </div>
        <div>
          <span>Last updated</span>
          <span>
            <span className={isOverdue ? 'diagnostic-problem' : ''}>
              {lastUpdatedSplit.main}
            </span>{' '}
            {lastUpdatedSplit.parenthetical && (
              <span className='diagnostic-muted'>
                {lastUpdatedSplit.parenthetical}
              </span>
            )}
          </span>
        </div>
        <div>
          <span>Next update</span>
          <span>
            {nextUpdateSplit.main}{' '}
            {nextUpdateSplit.parenthetical && (
              <span className='diagnostic-muted'>
                {nextUpdateSplit.parenthetical}
              </span>
            )}
          </span>
        </div>
      </section>
    </div>
  )
}

export function Resources() {
  const actions = useAppActions()
  const copy = useCopyToClipboard()
  const diagnosticEntries = useAppState((state) => state.diagnosticEntries)
  const resourcesDiagnosticEntries =
    useAppState((state) => state.resourcesDiagnosticEntries)
  const countryResourceComponentId =
    useAppState((state) => state.countryResourceComponentId)
  const languageResourceComponentId =
    useAppState((state) => state.languageResourceComponentId)
  const ntpSponsoredImagesComponentId =
    useAppState((state) => state.ntpSponsoredImagesComponentId)
  const ntpSponsoredImagesLoaded =
    useAppState((state) => state.ntpSponsoredImagesLoaded)
  const ntpSponsoredImagesManifestVersion =
    useAppState((state) => state.ntpSponsoredImagesManifestVersion)
  // These resources mainly feed Notification ad targeting, which is
  // Rewards-only; not worth showing to non-Rewards users.
  const rewardsEnabled = useAppState((state) => state.rewardsEnabled)
  const notificationAdsEnabled = rewardsEnabled && diagnosticEntries.find(
    (entry) => entry.name === 'Notification ads enabled',
  )?.value === 'true'
  const activeNotificationAdCampaigns =
    useAppState((state) => state.activeNotificationAdCampaigns)

  const countryCode =
    diagnosticEntries.find((entry) => entry.name === 'Country')?.value ?? ''
  // The catalog's own geo-targeting for the campaigns it actually contains,
  // rather than the device's own locale-derived country; a campaign not
  // being eligible here despite matching the device's country (or vice
  // versa) is exactly the kind of mismatch this page exists to surface.
  // Falls back to the device's own country only when no campaign declares
  // one at all (e.g. no active campaigns yet). Only notification ad
  // campaigns are sourced from the catalog (see campaigns.tsx). New tab
  // page campaigns come from the sponsored images component instead.
  const catalogGeoTargets = React.useMemo(() => Array.from(new Set(
    activeNotificationAdCampaigns.flatMap(
      (campaign: Campaign) => campaign['Geo Targets']),
  )).sort(), [activeNotificationAdCampaigns])
  const languageCode =
    diagnosticEntries.find((entry) => entry.name === 'Language')?.value ?? ''
  // The country resource component (unlike the catalog, which is filtered by
  // device-locale country via subdivision targeting) is actually selected
  // using the Variations/Griffin country. See
  // `AdsServiceDelegate::GetVariationsCountryCode()`.
  const resourceCountryCode = useAppState((state) => state.variationsCountryCode)
  const newTabPageAdsSchemaVersion =
    resourcesDiagnosticEntries.find(
      (entry) => entry.name === 'New Tab Page Ads Schema Version',
    )?.value ?? 'N/A'

  React.useEffect(() => {
    actions.loadDiagnostics()
    actions.loadAdsInternals()
  }, [])

  return (
    <>
      <TabHeader
        title='Resources'
        description={
          <>
            These resources are delivered via Brave's component updater.
            {COMPONENTS_LINK_SUPPORTED && (
              <>
                {' '}See{' '}
                <Link
                  href='brave://components'
                  target='_blank'
                  rel='noopener noreferrer'
                >
                  brave://components
                </Link>{' '}
                to check their install status.
              </>
            )}
          </>
        }
        onRefresh={() => {
          actions.loadDiagnostics()
          actions.loadAdsInternals()
        }}
      />

      {notificationAdsEnabled && (
        <div className='card-group'>
          <div className='content-card'>
            <h4>
              <span className='title'>
                Components ({languageCode || 'N/A'})
              </span>
            </h4>
          </div>
          <ResourceCard
            title='Text classification'
            value={
              resourcesDiagnosticEntries.find(
                (entry) => entry.name === 'Text classification resource',
              )?.value ?? 'Not loaded'
            }
            componentId={languageResourceComponentId}
          />
        </div>
      )}

      <div className='card-group'>
        <div className='content-card'>
          <h4>
            <span className='title'>
              Components ({resourceCountryCode || 'N/A'})
            </span>
          </h4>
        </div>
        <div className='content-card'>
          <h4>
            <span className='title'>New tab page ads</span>
          </h4>

          <section className='key-value-list'>
            <div>
              <span>Status</span>
              <span
                className={ntpSponsoredImagesLoaded ? '' : 'diagnostic-muted'}
              >
                {ntpSponsoredImagesLoaded
                  ? `Loaded (${ntpSponsoredImagesManifestVersion || 'unknown version'})`
                  : 'Not loaded'}
              </span>
            </div>
            <div>
              <span>ID</span>
              <span>
                {ntpSponsoredImagesComponentId
                  ? renderCopyableText(
                      ntpSponsoredImagesComponentId,
                      ntpSponsoredImagesComponentId,
                      copy,
                    )
                  : 'N/A'}
              </span>
            </div>
            <div>
              <span>Schema version</span>
              <span>{newTabPageAdsSchemaVersion}</span>
            </div>
          </section>
        </div>
        {notificationAdsEnabled && RESOURCES.map(({ entryName, title }) => (
          <ResourceCard
            key={entryName}
            title={title}
            value={
              resourcesDiagnosticEntries.find(
                (entry) => entry.name === entryName,
              )?.value ?? 'Not loaded'
            }
            componentId={countryResourceComponentId}
          />
        ))}
      </div>

      {notificationAdsEnabled && (
        <div className='card-group'>
          <CatalogCard
            geoTargets={catalogGeoTargets}
            fallbackCountryCode={countryCode}
            diagnosticEntries={diagnosticEntries}
            resourcesDiagnosticEntries={resourcesDiagnosticEntries}
          />
        </div>
      )}
    </>
  )
}
