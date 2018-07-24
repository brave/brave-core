/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { Grid, Column } from '../../../../../src/components/layout/gridList'
import Separator from '../../../../../src/old/separator/index'
import SwitchButton from '../../../../../src/old/switchButton/index'
import UnstyledButton from '../../../../../src/old/unstyledButton/index'
import TextLabel from '../../../../../src/old/textLabel/index'
import locale from '../fakeLocale'
import theme from '../theme'

const doNothing = () => {
  console.log('nothing')
}

class BraveShieldsHeader extends React.PureComponent {
  render () {
    return (
      <Grid
        id='braveShieldsHeader'
        theme={theme.braveShieldsHeader}
      >
        <Column size={4} theme={theme.columnVerticalCenter}>
          <TextLabel theme={theme.title} text={locale.shieldsHeaderShieldsToggle} />
        </Column>
        <Column size={6} theme={theme.columnVerticalCenter}>
          <SwitchButton
            id='shieldsToggle'
            leftText={locale.shieldsHeaderToggleLeftPosition}
            rightText={locale.shieldsHeaderToggleRightPosition}
            checked={true}
            onChange={doNothing}
          />
        </Column>
        <Column size={2} theme={theme.columnVerticalCenterEnd}>
          <UnstyledButton
            theme={theme.closeButton}
            text='&times;'
            onClick={doNothing}
          />
        </Column>
        <Column>
          <Separator />
        </Column>
        <Column theme={theme.hostnameContent}>
          <TextLabel text={locale.shieldsHeaderForSite} />
          <TextLabel theme={theme.hostname} text='Simply Red Fan Club' />
        </Column>
      </Grid>
    )
  }
}

export default BraveShieldsHeader
