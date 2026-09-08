/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { useAppState, useAppActions } from '../lib/app_context'
import { getDiagnosticValue, isWalletConnected } from '../lib/diagnostics'
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
  const rewardsEntries = useAppState((state) => state.rewardsDiagnosticEntries)
  const isSponsoredTilesShown = useAppState(
    (state) => state.isSponsoredTilesShown,
  )
  const walletConnected = isWalletConnected(rewardsEntries)

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
          {/* Search result ads are always shown regardless of Rewards state;
              it's only the ad event metrics sent for them that depend on
              whether a wallet is connected. */}
          <div>
            <span>Search result ad metrics</span>
            <MatchIcon isMatch={!walletConnected} />
          </div>
          {TILES_SUPPORTED && (
            <div>
              <span>Tiles shown</span>
              <MatchIcon isMatch={isSponsoredTilesShown && !walletConnected} />
            </div>
          )}
        </section>
      </div>
    </>
  )
}
