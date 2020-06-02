/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

// Components
import { Checkbox, Button } from 'brave-ui/components'
import { LogTextArea, LogControls, ButtonGroup } from '../style'

// Utils
import { getLocale } from '../../../../common/locale'

interface Props {
  log: string
  onClear: () => void
  onGet: () => void
}

interface State {
  autoRefresh: boolean
}

export class Log extends React.Component<Props, State> {
  private interval: number

  constructor (props: Props) {
    super(props)
    this.state = {
      autoRefresh: false
    }
  }

  autoRefreshToggle = (key: string, selected: boolean) => {
    this.setState({
      autoRefresh: selected
    })

    if (selected) {
      this.interval = setInterval(() => {
        this.props.onGet()
      }, 5000)
      return
    }

    clearInterval(this.interval)
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
          </ButtonGroup>
        </LogControls>
        <LogTextArea value={this.props.log} readOnly={true}/>
      </>
    )
  }
}
