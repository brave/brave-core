/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { StatsContainer, StatsItem } from '../../../../src/features/newTab/default'

export default class Stats extends React.PureComponent<{}, {}> {
  render () {
    return (
      <StatsContainer>
        <StatsItem counter='42' description='Trackers Blocked' />
        <StatsItem counter='105' description='Ads Blocked' />
        <StatsItem counter='5' text='minutes' description='Estimated Time Saved' />
      </StatsContainer>
    )
  }
}
