/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import createWidget from '../widget/index'

import { StyledClock, StyledTime } from './style'

interface TimeComponent {
  type: string,
  value: string
}

export interface ClockState {
  currentTime: Array<TimeComponent>
  date: Date
}

class Clock extends React.PureComponent<{}, ClockState> {
  constructor (props: {}) {
    super(props)
    this.state = this.getClockState(new Date())
  }

  get dateTimeFormat (): any {
    return new Intl.DateTimeFormat([], { hour: '2-digit', minute: '2-digit' })
  }

  get formattedTime () {
    return this.state.currentTime.map((component) => {
      if (component.type === 'dayPeriod') {
        // do not render AM/PM
        return null
      }
      return component.value
    })
  }

  getMinutes (date: any) {
    return Math.floor(date / 1000 / 60)
  }

  maybeUpdateClock () {
    const now = new Date()
    if (this.getMinutes(this.state.date) !== this.getMinutes(now)) {
      this.setState(this.getClockState(now))
    }
  }

  getClockState (now: Date) {
    return {
      date: now,
      currentTime: this.dateTimeFormat.formatToParts(now)
    }
  }

  componentDidMount () {
    window.setInterval(this.maybeUpdateClock.bind(this), 2000)
  }

  render () {
    return (
      <StyledClock>
        <StyledTime>{this.formattedTime}</StyledTime>
      </StyledClock>
    )
  }
}

export const ClockWidget = createWidget(Clock)
