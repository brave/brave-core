/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { Grid, Column } from '../../../../../src/components/layout/gridList/index'
import TextLabel from '../../../../../src/old/textLabel/index'
import data from '../fakeData'
import locale from '../fakeLocale'
import customStyle from '../theme'

class BraveShieldsStats extends React.PureComponent {
  render () {
    return (
      <Grid
        id='braveShieldsStats'
        customStyle={customStyle.braveShieldsStats}
        disabled={false}
      >
        <Column customStyle={customStyle.statsNumbers} size={1}>
          <TextLabel
            customStyle={customStyle.totalAdsTrackersBlockedStat}
            text={data.totalAdsTrackersBlocked}
          />
          <TextLabel
            customStyle={customStyle.httpsRedirectedStat}
            text={data.httpsRedirected}
          />
          <TextLabel
            customStyle={customStyle.javascriptBlockedStat}
            text={data.javascriptBlocked}
          />
          <TextLabel
            customStyle={customStyle.fingerprintingBlockedStat}
            text={data.fingerprintingBlocked}
          />
        </Column>
        <Column customStyle={customStyle.statsNames} size={11}>
          <TextLabel
            customStyle={customStyle.totalAdsTrackersBlockedText}
            text={locale.shieldsStatsAdsTrackersBlocked}
          />
          <TextLabel
            customStyle={customStyle.httpsRedirectedText}
            text={locale.shieldsStatsHttpsUpgrades}
          />
          <TextLabel
            customStyle={customStyle.javascriptBlockedText}
            text={locale.shieldsStatsScriptsBlocked}
          />
          <TextLabel
            customStyle={customStyle.fingerprintingBlockedText}
            text={locale.shieldsFingerPrintingBlocked}
          />
        </Column>
      </Grid>
    )
  }
}

export default BraveShieldsStats
