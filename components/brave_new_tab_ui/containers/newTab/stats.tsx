/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { StatsContainer, StatsItem, createWidget } from '../../components/default'

// Utils
import { getLocale } from '../../../common/locale'

interface Props {
  stats: NewTab.Stats
}

class Stats extends React.Component<Props, {}> {
  get millisecondsPerItem () {
    return 50
  }

  get adblockCount () {
    return this.props.stats.adsBlockedStat || 0
  }

  get estimatedBandwidthSaved () {
    const estimatedBWSaved = this.props.stats.bandwidthSavedStat
    if (estimatedBWSaved) {
      const bytes = estimatedBWSaved < 1024
      const kilobytes = estimatedBWSaved < 1024 * 1024
      const megabytes = estimatedBWSaved < 1024 * 1024 * 1024

      let counter
      let id
      if (bytes) {
        counter = estimatedBWSaved
        id = 'B'
      } else if (kilobytes) {
        counter = (estimatedBWSaved / 1024).toFixed(0)
        id = 'KB'
      } else if (megabytes) {
        counter = (estimatedBWSaved / 1024 / 1024).toFixed(1)
        id = 'MB'
      } else {
        counter = (estimatedBWSaved / 1024 / 1024 / 1024).toFixed(2)
        id = 'GB'
      }

      return {
        id,
        value: counter,
        args: JSON.stringify({ value: counter })
      }
    } else {
      return {
        id: 'B',
        value: 0,
        args: JSON.stringify({ value: 0 })
      }
    }
  }

  get estimatedTimeSaved () {
    const estimatedMillisecondsSaved = this.adblockCount * this.millisecondsPerItem || 0
    const hours = estimatedMillisecondsSaved < 1000 * 60 * 60 * 24
    const minutes = estimatedMillisecondsSaved < 1000 * 60 * 60
    const seconds = estimatedMillisecondsSaved < 1000 * 60
    let counter
    let id

    if (seconds) {
      counter = Math.ceil(estimatedMillisecondsSaved / 1000)
      id = counter === 1 ? 'second' : 'seconds'
    } else if (minutes) {
      counter = Math.ceil(estimatedMillisecondsSaved / 1000 / 60)
      id = counter === 1 ? 'minute' : 'minutes'
    } else if (hours) {
      // Refer to http://stackoverflow.com/a/12830454/2950032 for the detailed reasoning behind the + after
      // toFixed is applied. In a nutshell, + is used to discard unnecessary trailing 0s after calling toFixed
      counter = +((estimatedMillisecondsSaved / 1000 / 60 / 60).toFixed(1))
      id = counter === 1 ? 'hour' : 'hours'
    } else {
      // Otherwise the output is in days
      counter = +((estimatedMillisecondsSaved / 1000 / 60 / 60 / 24).toFixed(2))
      id = counter === 1 ? 'day' : 'days'
    }

    return {
      id,
      value: counter,
      args: JSON.stringify({ value: counter })
    }
  }

  render () {
    const trackedBlockersCount = this.adblockCount.toLocaleString()
    const timeSaved = this.estimatedTimeSaved
    const bandwidthSaved = this.estimatedBandwidthSaved

    return (
      <StatsContainer>
        <StatsItem
          description={getLocale('adsTrackersBlocked')}
          counter={trackedBlockersCount}
        />
        {bandwidthSaved &&
          <StatsItem
            counter={bandwidthSaved.value}
            text={getLocale(bandwidthSaved.id)}
            description={getLocale('estimatedBandwidthSaved')}
          />
        }
        <StatsItem
          counter={timeSaved.value}
          text={getLocale(timeSaved.id)}
          description={getLocale('estimatedTimeSaved')}
        />
      </StatsContainer>
    )
  }
}

export default createWidget(Stats)
