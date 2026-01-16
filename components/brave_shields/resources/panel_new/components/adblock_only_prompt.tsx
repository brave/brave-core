/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Button from '@brave/leo/react/button'

import { useShieldsApi } from '../api/shields_api_context'
import { getString } from './strings'
import { formatString } from '$web-common/formatString'

import { style } from './adblock_only_prompt.style'

const learnMoreURL =
  'https://support.brave.app/hc/en-us/articles/38076796692109'

export function AdblockOnlyPrompt() {
  const api = useShieldsApi()
  const { data: siteBlockInfo } = api.useGetSiteBlockInfo()
  const { data: repeatedReloadsDetected } = api.useRepeatedReloadsDetected()

  if (!siteBlockInfo) {
    return null
  }

  const shieldsEnabled = siteBlockInfo.isBraveShieldsEnabled
  const adblockOnlyEnabled = siteBlockInfo.isBraveShieldsAdBlockOnlyModeEnabled
  const showDisabledPrompt =
    siteBlockInfo.showShieldsDisabledAdBlockOnlyModePrompt

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
  const api = useShieldsApi()
  const setAdBlockOnlyModeEnabled = api.setBraveShieldsAdBlockOnlyModeEnabled
  const setAdBlockOnlyModePromptDismissed =
    api.setBraveShieldsAdBlockOnlyModePromptDismissed

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
              <button onClick={() => api.actions.openTab(learnMoreURL)}>
                {content}
              </button>
            ),
          },
        )}
      </p>
      <div className='actions'>
        <Button onClick={() => setAdBlockOnlyModeEnabled([true])}>
          {getString('BRAVE_SHIELDS_ENABLE_AD_BLOCK_ONLY_MODE')}
        </Button>
        <Button
          kind='plain'
          onClick={() => setAdBlockOnlyModePromptDismissed()}
        >
          {getString('BRAVE_SHIELDS_DISMISS_ALERT')}
        </Button>
      </div>
    </div>
  )
}

function AdblockOnlyFeedbackPrompt() {
  const api = useShieldsApi()
  return (
    <div data-css-scope={style.scope}>
      <h4>
        {getString('BRAVE_SHIELDS_IS_THIS_SITE_WORKING_CORRECTLY_NOW_TITLE')}
      </h4>
      <p>
        {getString('BRAVE_SHIELDS_IS_THIS_SITE_WORKING_CORRECTLY_NOW_DESC')}
      </p>
      <div className='actions'>
        <Button onClick={api.actions.closeUI}>
          {getString(
            'BRAVE_SHIELDS_IS_THIS_SITE_WORKING_CORRECTLY_NOW_LOOKS_GOOD',
          )}
        </Button>
        <Button
          kind='plain'
          onClick={api.actions.openWebCompatWindow}
        >
          {getString(
            'BRAVE_SHIELDS_IS_THIS_SITE_WORKING_CORRECTLY_NOW_REPORT_SITE',
          )}
        </Button>
      </div>
    </div>
  )
}
