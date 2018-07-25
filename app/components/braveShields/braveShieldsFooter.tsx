/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { Grid, Column } from 'brave-ui/gridSystem'
import UnstyledButton from 'brave-ui/unstyledButton'
import { getMessage } from '../../background/api/localeAPI'
import theme from '../../theme'
import * as tabsAPI from '../../background/api/tabsAPI'

export interface BraveShieldsStatsProps {
  tabId: number
}

export default class BraveShieldsFooter extends React.PureComponent<BraveShieldsStatsProps, {}> {
  constructor (props: BraveShieldsStatsProps) {
    super(props)
    this.openSettings = this.openSettings.bind(this)
    this.reloadShields = this.reloadShields.bind(this)
  }

  openSettings () {
    tabsAPI.createTab({ url: 'chrome://settings' })
      .catch((err) => console.log(err))
  }

  reloadShields () {
    tabsAPI.reloadTab(this.props.tabId, true)
      .catch((err) => console.log(err))
  }

  render () {
    return (
      <Grid id='braveShieldsFooter' theme={theme.braveShieldsFooter}>
        <Column size={9} theme={theme.columnStart}>
          <UnstyledButton
            theme={theme.noUserSelect}
            onClick={this.openSettings}
            text={getMessage('shieldsFooterEditDefault')}
          />
        </Column>
        <Column size={3} theme={theme.columnEnd}>
          <UnstyledButton
            onClick={this.reloadShields}
            theme={theme.noUserSelect}
            text={getMessage('shieldsFooterReload')}
          />
        </Column>
      </Grid>
    )
  }
}
