// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'
import { shallow } from 'enzyme'

import { VisibleOnlyDuringTimeFrame } from './visible-only-during-timeframe'

describe('<VisibleOnlyDuringTimeFrame />', () => {
  it('renders children when within start/end dates', () => {
    const START_DATE = new Date()
    START_DATE.setTime(START_DATE.getTime() - 1000 * 60 * 60 * 24 * 25) // 25 Days ago

    const END_DATE = new Date()
    END_DATE.setTime(START_DATE.getTime() + 1000 * 60 * 60 * 24 * 35) // 35 Days from start

    const wrapper = shallow((
      <VisibleOnlyDuringTimeFrame
        startDate={START_DATE}
        endDate={END_DATE}
      >
        <div className="unique" />
      </VisibleOnlyDuringTimeFrame>
    ))
    expect(wrapper.contains(<div className="unique" />)).toEqual(true)
  })

  it('does not render children when not within start/end dates', () => {
    const START_DATE = new Date()
    START_DATE.setTime(START_DATE.getTime() - 1000 * 60 * 60 * 24 * 25) // 25 Days ago

    const END_DATE = new Date()
    END_DATE.setTime(START_DATE.getTime() + 1000 * 60 * 60 * 24 * 20) // 20 Days from start (5 days ago)

    const wrapper = shallow((
      <VisibleOnlyDuringTimeFrame
        startDate={START_DATE}
        endDate={END_DATE}
      >
        <div className="unique" />
      </VisibleOnlyDuringTimeFrame>
    ))
    expect(wrapper.contains(<div className="unique" />)).toEqual(false)
  })
})
