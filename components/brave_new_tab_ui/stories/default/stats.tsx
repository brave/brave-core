/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { StatsContainer, StatsItem } from '../../../../src/features/newTab/default'

// Helpers
import { getLocale } from '../fakeLocale'

export default class Stats extends React.PureComponent<{}, {}> {
  render () {
    return (
      <StatsContainer>
        <StatsItem counter='42' description={getLocale('trackersBlocked')} />
        <StatsItem counter='105' description={getLocale('adsBlocked')} />
        <StatsItem counter='0' description={getLocale('httpsUpgrades')} />
        <StatsItem counter='5' text={getLocale('minutes')} description={getLocale('estimatedTimeSaved')} />
      </StatsContainer>
    )
  }
}
