/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { Grid, Column } from '../../../../../src/components/layout/gridList/index'
import TextLabel from '../../../../../src/old/textLabel/index'
import data from '../fakeData'
import locale from '../fakeLocale'
import theme from '../theme'

class BraveShieldsStats extends React.PureComponent {
  render () {
    return (
      <Grid
        id='braveShieldsStats'
        theme={theme.braveShieldsStats}
        disabled={false}
      >
        <Column theme={theme.statsNumbers} size={1}>
          <TextLabel
            theme={theme.totalAdsTrackersBlockedStat}
            text={data.totalAdsTrackersBlocked}
          />
          <TextLabel
            theme={theme.httpsRedirectedStat}
            text={data.httpsRedirected}
          />
          <TextLabel
            theme={theme.javascriptBlockedStat}
            text={data.javascriptBlocked}
          />
          <TextLabel
            theme={theme.fingerprintingBlockedStat}
            text={data.fingerprintingBlocked}
          />
        </Column>
        <Column theme={theme.statsNames} size={11}>
          <TextLabel
            theme={theme.totalAdsTrackersBlockedText}
            text={locale.shieldsStatsAdsTrackersBlocked}
          />
          <TextLabel
            theme={theme.httpsRedirectedText}
            text={locale.shieldsStatsHttpsUpgrades}
          />
          <TextLabel
            theme={theme.javascriptBlockedText}
            text={locale.shieldsStatsScriptsBlocked}
          />
          <TextLabel
            theme={theme.fingerprintingBlockedText}
            text={locale.shieldsFingerPrintingBlocked}
          />
        </Column>
      </Grid>
    )
  }
}

export default BraveShieldsStats
