/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { ContributionPublisher } from './contributionPublisher'

interface Props {
  items: RewardsInternals.ContributionPublisher[]
}

export const ContributionPublishers = (props: Props) => {
  if (!props.items || props.items.length === 0) {
    return null
  }

  return (
    <>
      {props.items.map((item, index) => (
        <div key={item.contributionId}>
          <ContributionPublisher publisher={item || ''} />
          {(index !== props.items.length - 1) ? <hr/> : null}
        </div>
      ))}
    </>
  )
}
