/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { useAppState, useAppActions } from '../lib/app_model_context'

import { style } from './rewards_events.style'

export function RewardsEvents() {
  const actions = useAppActions()
  const events = useAppState((state) => state.rewardsEvents)

  React.useEffect(() => {
    actions.loadRewardsEvents()
  }, [])

  function renderTable() {
    if (events.length === 0) {
      return <p>No events</p>
    }

    return (
      <table>
        <thead>
          <tr>
            <th>Created at</th>
            <th>Key</th>
            <th>Value</th>
          </tr>
        </thead>
        <tbody>
          {events.map((event) => (
            <tr key={event.id}>
              <td>{new Date(event.createdAt).toISOString()}</td>
              <td>{event.key}</td>
              <td>{event.value}</td>
            </tr>
          ))}
        </tbody>
      </table>
    )
  }

  return (
    <div
      className='content-card'
      data-css-scope={style.scope}
    >
      <h4>Events</h4>
      <section>{renderTable()}</section>
    </div>
  )
}
