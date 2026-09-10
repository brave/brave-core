/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { useAppState, useAppActions } from '../lib/app_context'
import { getDiagnosticValue } from '../lib/diagnostics'
import { MatchIcon } from './match_icon'
import { TabHeader } from './tab_header'

// There's no NTP "Top Sites" tiles surface on iOS, so this row would always
// read "No" there regardless of the underlying pref.
// <if expr="is_ios">
const TILES_SUPPORTED = false
// </if>
// <if expr="!is_ios">
const TILES_SUPPORTED = true
// </if>

export function AdFormats() {
  const actions = useAppActions()
  const rawEntries = useAppState((state) => state.diagnosticEntries)
  const rewardsEnabled = useAppState((state) => state.rewardsEnabled)
  const isSponsoredTilesShown = useAppState(
    (state) => state.isSponsoredTilesShown,
  )

  React.useEffect(() => {
    actions.loadDiagnostics()
  }, [])

  function isMatch(name: string) {
    return getDiagnosticValue(rawEntries, name) === 'true'
  }

  return (
    <>
      <TabHeader
        title='Ad Formats'
        description='Which ad formats are enabled on this device.'
        onRefresh={actions.loadDiagnostics}
      />
      {rewardsEnabled && (
        <div className='content-card'>
          <h4>
            <span className='title'>Notification</span>
          </h4>
          <section className='key-value-list'>
            <div>
              <span>Enabled</span>
              <MatchIcon isMatch={isMatch('Notification ads enabled')} />
            </div>
          </section>
        </div>
      )}
      <div className='content-card'>
        <h4>
          <span className='title'>Sponsored</span>
        </h4>
        <section className='key-value-list'>
          <div>
            <span>Enabled</span>
            <MatchIcon isMatch={isMatch('Sponsored ads enabled')} />
          </div>
          <hr className='diagnostic-divider' />
          <div>
            <span>New tab page ads shown</span>
            <MatchIcon isMatch={isMatch('New tab page ads shown')} />
          </div>
          {/* Unlike new tab page ads, search result ads don't need a
              connected wallet or Rewards at all, so this mirrors "Enabled"
              directly rather than a separate diagnostic entry. */}
          <div>
            <span>Search result ads shown</span>
            <MatchIcon isMatch={isMatch('Sponsored ads enabled')} />
          </div>
          {TILES_SUPPORTED && (
            <div>
              <span>Tiles shown</span>
              <MatchIcon isMatch={isSponsoredTilesShown} />
            </div>
          )}
        </section>
      </div>
    </>
  )
}
