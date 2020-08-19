/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

// Components
import { EventTable, EventCell } from '../style'

// Utils
import { getLocale } from '../../../../common/locale'
import { formatDate } from '../utils'

interface Props {
  items: RewardsInternals.EventLog[]
}

export class EventLogs extends React.Component<Props, {}> {
  render () {
    return (
      <EventTable>
        <thead>
          <tr>
            <th>{getLocale('eventLogTime')}</th>
            <th>{getLocale('eventLogKey')}</th>
            <th>{getLocale('eventLogValue')}</th>
          </tr>
        </thead>
        <tbody>
        {this.props.items.map((item) =>
          <tr key={item.id}>
            <EventCell>{formatDate(item.createdAt * 1000)}</EventCell>
            <EventCell>{item.key}</EventCell>
            <EventCell>{item.value}</EventCell>
          </tr>
        )}
        </tbody>
      </EventTable>
    )
  }
}
