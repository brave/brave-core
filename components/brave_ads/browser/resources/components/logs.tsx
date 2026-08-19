/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Button from '@brave/leo/react/button'
import Dialog from '@brave/leo/react/dialog'
import Icon from '@brave/leo/react/icon'
import Toggle from '@brave/leo/react/toggle'

import { useAppState, useAppActions } from '../lib/app_context'

import { style } from './logs.style'

export function Logs() {
  const actions = useAppActions()
  const log = useAppState((state) => state.log)
  const verboseLoggingEnabled = useAppState(
    (state) => state.verboseLoggingEnabled,
  )

  const textAreaRef = React.useRef<HTMLTextAreaElement>(null)
  const [autoRefresh, setAutoRefresh] = React.useState(false)
  const [showVerboseDialog, setShowVerboseDialog] = React.useState(false)

  React.useEffect(() => {
    actions.loadLog()
  }, [])

  React.useEffect(() => {
    if (!autoRefresh) {
      return
    }
    const interval: any = setInterval(() => {
      actions.loadLog()
    }, 5000)
    return () => {
      clearInterval(interval)
    }
  }, [autoRefresh])

  React.useEffect(() => {
    const elem = textAreaRef.current
    if (elem) {
      elem.scrollTo({ top: elem.scrollHeight })
    }
  }, [log])

  function download() {
    actions.fetchFullLog().then((fullLog) => {
      const content =
        'WARNING: This log file may contain sensitive data. Be careful who ' +
        'you share it with.\n\n' + fullLog
      const filename = 'brave_ads_internals_log.txt'
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

  return (
    <div
      className='content-card'
      data-css-scope={style.scope}
    >
      <h4>
        <span className='title'>Logs</span>
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
          onClick={actions.clearLog}
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
