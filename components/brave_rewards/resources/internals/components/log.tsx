/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

// Components
import { Checkbox, Button } from 'brave-ui/components'
import { LogTextArea, LogControls, ButtonGroup, Notice } from '../style'

// Utils
import { getLocale } from '../../../../common/locale'

interface Props {
  log: string
  fullLog: string
  onClear: () => void
  onGet: () => void
  onFullLog: () => void
  onDownloadCompleted: () => void
}

interface State {
  autoRefresh: boolean
  downloadStarted: boolean
}

export class Log extends React.Component<Props, State> {
  private interval: number

  constructor (props: Props) {
    super(props)
    this.state = {
      autoRefresh: false,
      downloadStarted: false
    }
  }

  componentDidUpdate (prevProps: Props) {
    if (this.props.fullLog.length !== 0) {
      this.downloadFile(this.props.fullLog)
      this.props.onDownloadCompleted()
    }
  }

  autoRefreshToggle = (key: string, selected: boolean) => {
    this.setState({
      autoRefresh: selected
    })

    if (selected) {
      this.interval = window.setInterval(() => {
        this.props.onGet()
      }, 5000)
      return
    }

    window.clearInterval(this.interval)
  }

  downloadFile = (log: string) => {
    const filename = 'brave_rewards_log.txt'
    let element = document.createElement('a')
    element.setAttribute('href', 'data:text/plain;charset=utf-8,' + encodeURIComponent(log))
    element.setAttribute('download', filename)

    element.style.display = 'none'
    document.body.appendChild(element)

    element.click()
    document.body.removeChild(element)
  }

  render () {
    return (
    <>
      <LogControls>
        <Checkbox
            value={{ 'auto': this.state.autoRefresh }}
            size={'small'}
            onChange={this.autoRefreshToggle}
            type={'light'}
        >
          <div data-key='auto'>{getLocale('autoRefresh')}</div>
        </Checkbox>
        <ButtonGroup>
          <Button
             text={getLocale('clearButton')}
             size={'medium'}
             type={'accent'}
             onClick={this.props.onClear}
          />
          <Button
             text={getLocale('refreshButton')}
             size={'medium'}
             type={'accent'}
             onClick={this.props.onGet}
          />
          <Button
             text={getLocale('downloadButton')}
             size={'medium'}
             type={'accent'}
             onClick={this.props.onFullLog}
          />
        </ButtonGroup>
      </LogControls>
      <LogTextArea value={this.props.log} readOnly={true} />
      <Notice>
        {getLocale('logNotice', { numberOfLines: '5,000' })}
      </Notice>
    </>
    )
  }
}
