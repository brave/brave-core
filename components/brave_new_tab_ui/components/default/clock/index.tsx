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

interface Props {
  clockFormat: string
  toggleClickFormat: () => void
}

class Clock extends React.PureComponent<Props, ClockState> {
  private updateInterval: number

  constructor (props: any) {
    super(props)
    this.state = this.getClockState(new Date())
  }

  componentDidUpdate (prevProps: Props) {
    if (prevProps.clockFormat !== this.props.clockFormat) {
      this.setState(this.getClockState(new Date()))
    }
  }

  get dateTimeFormat (): any {
    // https://developer.mozilla.org/docs/Web/JavaScript/Reference/Global_Objects/DateTimeFormat
    const options: Intl.DateTimeFormatOptions = { hour: 'numeric', minute: 'numeric' }
    if (this.props.clockFormat === '24') {
      options['hourCycle'] = 'h23'
    } else if (this.props.clockFormat === '12') {
      options['hourCycle'] = 'h12'
    }
    return new Intl.DateTimeFormat(undefined, options)
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
    this.updateInterval = window.setInterval(this.maybeUpdateClock.bind(this), 2000)
  }

  componentWillUnmount () {
    clearInterval(this.updateInterval)
  }

  onDoubleClick = () => {
    if (this.props.toggleClickFormat) {
      this.props.toggleClickFormat()
    }
  }

  render () {
    return (
      <StyledClock onDoubleClick={this.onDoubleClick}>
        <StyledTime>{this.formattedTime}</StyledTime>
      </StyledClock>
    )
  }
}

export const ClockWidget = createWidget(Clock)
