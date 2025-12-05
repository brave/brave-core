/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Button from '@brave/leo/react/button'

import { formatString } from '$web-common/formatString'
import { useShieldsState, useShieldsActions } from '../lib/shields_context'
import { getString } from '../lib/strings'
import { style } from './footer.style'

export function Footer() {
  const actions = useShieldsActions()
  const shieldsEnabled = useShieldsState(
    (s) => s.siteBlockInfo.isBraveShieldsEnabled,
  )
  const adblockOnlyEnabled = useShieldsState(
    (s) => s.siteBlockInfo.isBraveShieldsAdBlockOnlyModeEnabled,
  )
  const showAdvancedSettings = useShieldsState((s) => s.showAdvancedSettings)

  function renderActions() {
    if (shieldsEnabled && !showAdvancedSettings && !adblockOnlyEnabled) {
      return null
    }
    return (
      <div className='actions'>
        <Button
          kind='outline'
          onClick={() => actions.openTab('chrome://settings/shields/filters')}
        >
          {getString('BRAVE_SHIELDS_CUSTOMIZE_ADBLOCK_LISTS')}
        </Button>
        <Button
          kind='outline'
          onClick={() => actions.openTab('chrome://settings/shields')}
        >
          {getString('BRAVE_SHIELDS_GLOBAL_SETTINGS')}
        </Button>
      </div>
    )
  }

  function renderTryShieldsOffMessage() {
    if (!shieldsEnabled) {
      return null
    }
    return (
      <div className='try-off'>
        <p>{getString('BRAVE_SHIELDS_BROKEN')}</p>
        <p>
          {formatString(getString('BRAVE_SHIELDS_PRIVACY_NOTE'), {
            $1: (content) => (
              <button
                onClick={() => {
                  actions.openTab('https://brave.com/privacy-features/')
                }}
              >
                {content}
              </button>
            ),
          })}
        </p>
      </div>
    )
  }

  return (
    <div data-css-scope={style.scope}>
      {renderActions()}
      {renderTryShieldsOffMessage()}
    </div>
  )
}
