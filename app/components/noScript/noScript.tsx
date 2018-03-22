/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { BrowserButton, BrowserText, Column, Grid, Separator, SwitchButton } from 'brave-ui'
import { getMessage } from '../../background/api/localeAPI'

export interface Props {
  blocked: boolean
  blockedOrigins: string[]
  onSubmit: (origins: string[]) => void
}

export interface State {
  allowOrigins: string[]
}

export default class NoScript extends React.Component<Props, State> {
  constructor (props: Props) {
    super(props)
    this.onChangeOriginSetting = this.onChangeOriginSetting.bind(this)
    this.onClick = this.onClick.bind(this)
    this.state = { allowOrigins : [] }
  }

  onChangeOriginSetting (e: any) {
    const origin = e.target.id
    let newAllowOrigins

    if (this.state.allowOrigins.indexOf(origin) > -1) {
      newAllowOrigins = this.state.allowOrigins.filter(s => s !== origin)
    } else {
      newAllowOrigins = [...this.state.allowOrigins, origin]
    }

    this.setState({ allowOrigins : newAllowOrigins })
  }

  onClick () {
    this.props.onSubmit(this.state.allowOrigins)
  }

  render () {
    const originSwitches = this.props.blockedOrigins.map((origin) => (
      <Column key={origin}>
        <SwitchButton
          id={origin}
          checked={this.state.allowOrigins.indexOf(origin) < 0}
          rightText={origin}
          onChange={this.onChangeOriginSetting}
        />
      </Column>))

    return (
      this.props.blocked && this.props.blockedOrigins.length
      ?
      (
        <div>
          <Separator />
          <Grid
            gap='10px 5px'
            padding='10px 0'
          >
            <Column>
              <BrowserText text={getMessage('noScriptSwitches')} />
            </Column>
            {originSwitches}
            <Column align='flex-end'>
              <BrowserButton id='apply' size='10px' onClick={this.onClick}> {getMessage('noScriptApplyOnce')} </BrowserButton>
            </Column>
          </Grid>
        </div>
      )
      : null
    )
  }
}
