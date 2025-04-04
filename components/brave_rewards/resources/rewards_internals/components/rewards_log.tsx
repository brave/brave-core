/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Button from '@brave/leo/react/button'

import { useAppState, useAppActions, useLocale } from '../lib/app_model_context'

import { style } from './rewards_log.style'

export function RewardsLog() {
  const { getString } = useLocale()
  const actions = useAppActions()
  const log = useAppState((state) => state.rewardsLog)

  React.useEffect(() => { actions.loadRewardsLog() }, [])

  function download() {
    actions.fetchFullRewardsLog().then((fullLog) => {
      const content = getString('fullLogDisclaimerText') + '\n\n' + fullLog
      const filename = 'brave_rewards_sensitive_log.txt'
      const element = document.createElement('a')
      element.setAttribute(
          'href',
          'data:text/plain;charset=utf-8,' + encodeURIComponent(content))
      element.setAttribute('download', filename)
      element.style.display = 'none'
      document.body.appendChild(element)
      element.click()
      document.body.removeChild(element)
    })
  }

  function clearLog() {
    actions.clearRewardsLog()
  }

  return (
    <div className='content-card' data-css-scope={style.scope}>
      <h4>
        <span>Rewards Log</span>
        <Button size='small' onClick={download}>Download</Button>
        <Button size='small' onClick={clearLog}>Clear</Button>
      </h4>
      <textarea value={log} readOnly />
    </div>
  )
}
