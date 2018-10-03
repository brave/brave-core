/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { SelectBox, Stat, Grid } from '../../../../src/features/shields'
import locale from '../fakeLocale'
import data from '../fakeData'

interface Props {
  enabled: boolean
}

export default class PrivacyControls extends React.PureComponent<Props, {}> {
  render () {
    const { enabled } = this.props
    if (!enabled) {
      return null
    }
    return (
      <>
        {/* cookies select */}
        <Grid>
          <Stat>{data.thirdPartyCookiesBlocked}</Stat>
          <SelectBox disabled={false}>
            <option value='block_third_party'>{locale.block3partyCookies}</option>
            <option value='block'>{locale.blockAllCookies}</option>
            <option value='allow'>{locale.allowAllCookies}</option>
          </SelectBox>
        </Grid>
        {/* scripts select */}
        <Grid>
          <Stat>{data.thirdPartyScriptsBlocked}</Stat>
          <SelectBox disabled={false}>
            <option value='block_third_party'>{locale.block3partyScripts}</option>
            <option value='block'>{locale.blockAllScripts}</option>
            <option value='allow'>{locale.allowAllScripts}</option>
          </SelectBox>
        </Grid>
        {/* fingerprinting select */}
        <Grid>
          <Stat>{data.thirdPartyDeviceRecognitionBlocked}</Stat>
          <SelectBox disabled={false}>
            <option value='block_third_party'>{locale.block3partyFingerprinting}</option>
            <option value='block'>{locale.blockAllFingerprinting}</option>
            <option value='allow'>{locale.allowAllFingerprinting}</option>
          </SelectBox>
        </Grid>
      </>
    )
  }
}
