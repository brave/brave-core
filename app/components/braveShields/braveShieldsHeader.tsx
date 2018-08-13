/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { Grid, Column } from 'brave-ui/gridSystem'
import SwitchButton from 'brave-ui/switchButton'
import UnstyledButton from 'brave-ui/unstyledButton'
import TextLabel from 'brave-ui/textLabel'
import * as shieldActions from '../../types/actions/shieldsPanelActions'
import { BlockOptions } from '../../types/other/blockTypes'
import { getMessage } from '../../background/api/localeAPI'
import theme from '../../theme'
import { isHttpOrHttps } from '../../helpers/urlUtils'

export interface BraveShieldsHeaderProps {
  shieldsToggled: shieldActions.ShieldsToggled
  origin: string
  hostname: string
  braveShields: BlockOptions
}

export default class BraveShieldsHeader extends React.PureComponent<BraveShieldsHeaderProps, {}> {
  constructor (props: BraveShieldsHeaderProps) {
    super(props)
    this.onToggleShields = this.onToggleShields.bind(this)
    this.onClosePopup = this.onClosePopup.bind(this)
  }

  onToggleShields (e: HTMLSelectElement) {
    const { origin } = this.props
    if (!isHttpOrHttps(origin)) {
      return
    }
    const shieldsOption: BlockOptions = e.target.checked ? 'allow' : 'block'
    this.props.shieldsToggled(shieldsOption)
  }

  onClosePopup () {
    window.close()
  }

  render () {
    const { braveShields, hostname } = this.props
    return (
      <Grid id='braveShieldsHeader' theme={theme.braveShieldsHeader}>
        <Column size={4} theme={theme.columnVerticalCenter}>
          <TextLabel theme={theme.title} text={getMessage('shieldsHeaderShieldsToggle')} />
        </Column>
        <Column size={6} theme={theme.columnVerticalCenter}>
          <SwitchButton
            id='shieldsToggle'
            leftText={getMessage('shieldsHeaderToggleLeftPosition')}
            rightText={getMessage('shieldsHeaderToggleRightPosition')}
            checked={braveShields !== 'block'}
            onChange={this.onToggleShields}
          />
        </Column>
        <Column size={2} theme={theme.columnVerticalCenterEnd}>
          <UnstyledButton
            theme={theme.closeButton}
            text='&times;'
            onClick={this.onClosePopup}
          />
        </Column>
        <Column>
          <hr style={theme.separator} />
        </Column>
        <Column theme={theme.hostnameContent}>
          <TextLabel text={getMessage('shieldsHeaderForSite')} />
          <TextLabel id='hostname' theme={theme.hostname} text={hostname} />
        </Column>
      </Grid>
    )
  }
}
