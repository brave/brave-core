/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { BrowserButton, BrowserText, Column, Grid, Separator, SwitchButton } from 'brave-ui'
import { getMessage } from '../../background/api/localeAPI'
import { NoScriptInfo } from '../../types/other/noScriptInfo'

export interface Props {
  blocked: boolean
  noScriptInfo: NoScriptInfo
  onSubmit: (origins: string[]) => void
  onChangeNoScriptSettings: (origin: string) => void
}

export default class NoScript extends React.Component<Props, Object> {
  constructor (props: Props) {
    super(props)
    this.onChangeOriginSettings = this.onChangeOriginSettings.bind(this)
    this.onClick = this.onClick.bind(this)
  }

  onChangeOriginSettings (e: any) {
    this.props.onChangeNoScriptSettings(e.target.id)
  }

  onClick () {
    const noScriptInfo = this.props.noScriptInfo
    this.props.onSubmit(
      Object.keys(noScriptInfo).filter(key => !noScriptInfo[key].willBlock))
  }

  render () {
    const originSwitches = Object.keys(this.props.noScriptInfo).map((origin) => (
      <Column key={origin}>
        <SwitchButton
          id={origin}
          checked={this.props.noScriptInfo[origin].willBlock}
          rightText={origin}
          onChange={this.onChangeOriginSettings}
        />
      </Column>))

    return (
      this.props.blocked && Object.keys(this.props.noScriptInfo).length
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
              <BrowserButton id='apply' onClick={this.onClick}> {getMessage('noScriptApplyOnce')} </BrowserButton>
            </Column>
          </Grid>
        </div>
      )
      : null
    )
  }
}
