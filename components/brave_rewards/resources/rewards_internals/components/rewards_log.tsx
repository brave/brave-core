/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Button from '@brave/leo/react/button'
import Dialog from '@brave/leo/react/dialog'
import Icon from '@brave/leo/react/icon'
import Toggle from '@brave/leo/react/toggle'

import { useAppState, useAppActions } from '../lib/app_context'

import { style } from './rewards_log.style'

export function RewardsLog() {
  const actions = useAppActions()
  const { getString } = actions
  const log = useAppState((state) => state.rewardsLog)
  const verboseLoggingEnabled = useAppState(
    (state) => state.verboseLoggingEnabled,
  )

  const textAreaRef = React.useRef<HTMLTextAreaElement>(null)
  const [logLoaded, setLogLoaded] = React.useState(log.length > 0)
  const [autoRefresh, setAutoRefresh] = React.useState(false)
  const [showVerboseDialog, setShowVerboseDialog] = React.useState(false)

  React.useEffect(() => {
    actions.loadRewardsLog()
  }, [])

  React.useEffect(() => {
    if (!logLoaded && log.length > 0) {
      setLogLoaded(true)
    }
  }, [log, logLoaded])

  React.useEffect(() => {
    if (!autoRefresh) {
      return
    }
    const interval: any = setInterval(() => {
      actions.loadRewardsLog()
    }, 5000)
    return () => {
      clearInterval(interval)
    }
  }, [autoRefresh])

  React.useEffect(() => {
    const elem = textAreaRef.current
    if (logLoaded && elem) {
      elem.scrollTo({ top: elem.scrollHeight })
    }
  }, [logLoaded])

  function download() {
    actions.fetchFullRewardsLog().then((fullLog) => {
      const content = getString('fullLogDisclaimerText') + '\n\n' + fullLog
      const filename = 'brave_rewards_sensitive_log.txt'
      const element = document.createElement('a')
      element.setAttribute(
        'href',
        'data:text/plain;charset=utf-8,' + encodeURIComponent(content),
      )
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
    <div
      className='content-card'
      data-css-scope={style.scope}
    >
      <h4>
        <span className='title'>Rewards Log</span>
        <Toggle
          size='small'
          checked={autoRefresh}
          onChange={() => setAutoRefresh(!autoRefresh)}
        >
          Auto-refresh
        </Toggle>
        <Toggle
          size='small'
          checked={verboseLoggingEnabled}
          onChange={() => setShowVerboseDialog(true)}
        >
          Verbose mode
        </Toggle>
        <Button
          size='small'
          onClick={download}
        >
          Download
        </Button>
        <Button
          size='small'
          onClick={clearLog}
        >
          Clear
        </Button>
      </h4>

      <textarea
        ref={textAreaRef}
        value={log}
        readOnly
      />

      <Dialog
        isOpen={showVerboseDialog}
        onClose={() => setShowVerboseDialog(false)}
        backdropClickCloses={false}
      >
        <div slot='title'>Brave Rewards Verbose Logging</div>
        <div className='verbose-logging-info'>
          <Icon name='warning-triangle-filled' />
          <div>
            Enables detailed logging of Brave Rewards system events to a log
            file stored on your device. Please note that this log file could
            include information such as browsing history and credentials such as
            passwords and access tokens depending on your activity. Please do
            not share it unless asked to by Brave staff.
          </div>
        </div>
        <div slot='actions'>
          <Button
            kind='plain-faint'
            onClick={() => setShowVerboseDialog(false)}
          >
            Cancel
          </Button>
          <Button
            onClick={() => {
              setShowVerboseDialog(false)
              actions.toggleVerboseLoggingAndRestart()
            }}
          >
            {verboseLoggingEnabled
              ? 'Disable and Restart'
              : 'Enable and Restart'}
          </Button>
        </div>
      </Dialog>
    </div>
  )
}
