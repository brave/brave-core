/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Button from '@brave/leo/react/button'

import { useShieldsState, useShieldsActions } from '../lib/shields_context'
import { getString } from '../lib/strings'
import { formatString } from '$web-common/formatString'

import { style } from './adblock_only_prompt.style'

const learnMoreURL =
  'https://support.brave.app/hc/en-us/articles/38076796692109'

export function AdblockOnlyPrompt() {
  const shieldsEnabled = useShieldsState(
    (s) => s.siteBlockInfo.isBraveShieldsEnabled,
  )
  const adblockOnlyEnabled = useShieldsState(
    (s) => s.siteBlockInfo.isBraveShieldsAdBlockOnlyModeEnabled,
  )
  const showDisabledPrompt = useShieldsState(
    (s) => s.siteBlockInfo.showShieldsDisabledAdBlockOnlyModePrompt,
  )
  const repeatedReloadsDetected = useShieldsState(
    (s) => s.repeatedReloadsDetected,
  )

  if (!adblockOnlyEnabled && shieldsEnabled && repeatedReloadsDetected) {
    return <AdblockOnlyEnablePrompt />
  }

  if (!adblockOnlyEnabled && !shieldsEnabled && showDisabledPrompt) {
    return <AdblockOnlyEnablePrompt />
  }

  if (shieldsEnabled && adblockOnlyEnabled) {
    return <AdblockOnlyFeedbackPrompt />
  }

  return null
}

function AdblockOnlyEnablePrompt() {
  const actions = useShieldsActions()
  return (
    <div data-css-scope={style.scope}>
      <h4>
        {getString(
          'BRAVE_SHIELDS_ARE_YOU_EXPERIENCING_ISSUES_WITH_THIS_SITE_TITLE',
        )}
      </h4>
      <p>
        {formatString(
          getString(
            'BRAVE_SHIELDS_ARE_YOU_EXPERIENCING_ISSUES_WITH_THIS_SITE_DESC1',
          ),
          {
            $1: (content) => (
              <button onClick={() => actions.openTab(learnMoreURL)}>
                {content}
              </button>
            ),
          },
        )}
      </p>
      <div className='actions'>
        <Button onClick={() => actions.setAdBlockOnlyModeEnabled(true)}>
          {getString('BRAVE_SHIELDS_ENABLE_AD_BLOCK_ONLY_MODE')}
        </Button>
        <Button
          kind='plain'
          onClick={actions.dismissAdBlockOnlyModePrompt}
        >
          {getString('BRAVE_SHIELDS_DISMISS_ALERT')}
        </Button>
      </div>
    </div>
  )
}

function AdblockOnlyFeedbackPrompt() {
  const actions = useShieldsActions()
  return (
    <div data-css-scope={style.scope}>
      <h4>
        {getString('BRAVE_SHIELDS_IS_THIS_SITE_WORKING_CORRECTLY_NOW_TITLE')}
      </h4>
      <p>
        {getString('BRAVE_SHIELDS_IS_THIS_SITE_WORKING_CORRECTLY_NOW_DESC')}
      </p>
      <div className='actions'>
        <Button onClick={actions.closeUI}>
          {getString(
            'BRAVE_SHIELDS_IS_THIS_SITE_WORKING_CORRECTLY_NOW_LOOKS_GOOD',
          )}
        </Button>
        <Button
          kind='plain'
          onClick={actions.openWebCompatReporter}
        >
          {getString(
            'BRAVE_SHIELDS_IS_THIS_SITE_WORKING_CORRECTLY_NOW_REPORT_SITE',
          )}
        </Button>
      </div>
    </div>
  )
}
