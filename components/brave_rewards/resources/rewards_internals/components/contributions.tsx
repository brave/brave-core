/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { ContributionInfo } from '../lib/app_state'
import { useAppState, useAppActions } from '../lib/app_model_context'

import { style } from './contributions.style'

export function Contributions() {
  const actions = useAppActions()
  const contributions = useAppState((state) => state.contributions)

  React.useEffect(() => { actions.loadContributions() }, [])

  function renderContribution(contribution: ContributionInfo) {
    return (
      <div key={contribution.id} className='content-card'>
        <h4>{contribution.id}</h4>
        <section>
          <div className='key-value-list'>
            <div>
              <span>Created at:</span>
              <span>{new Date(contribution.createdAt).toISOString()}</span>
            </div>
            <div>
              <span>Type:</span>
              <span>{contribution.type}</span>
            </div>
            <div>
              <span>Amount:</span>
              <span>{contribution.amount}</span>
            </div>
            <div>
              <span>Step:</span>
              <span>{contribution.step}</span>
            </div>
            <div>
              <span>Retry count:</span>
              <span>{contribution.retryCount}</span>
            </div>
            <div>
              <span>Processor:</span>
              <span>{contribution.processor}</span>
            </div>
          </div>
          <table>
            <thead>
              <tr>
                <th>Creator</th>
                <th>Amount</th>
                <th>Contributed</th>
              </tr>
            </thead>
            <tbody>
            {
              contribution.publishers.map((publisher) => (
                <tr key={publisher.id}>
                  <td>{publisher.id}</td>
                  <td>{publisher.totalAmount}</td>
                  <td>{publisher.contributedAmount}</td>
                </tr>
              ))
            }
            </tbody>
          </table>
        </section>
      </div>
    )
  }

  return (
    <div data-css-scope={style.scope}>
      {
        contributions.length > 0
            ? contributions.map(renderContribution)
            : <p>No contributions</p>
      }
    </div>
  )
}
