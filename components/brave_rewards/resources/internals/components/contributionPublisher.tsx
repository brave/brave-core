/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

interface Props {
  publisher: RewardsInternals.ContributionPublisher
}

// Utils
import { getLocale } from '../../../../common/locale'

export const ContributionPublisher = (props: Props) => (
  <blockquote>
    <h3>{props.publisher.publisherKey}</h3>
    <div>
      {getLocale('totalAmount')} {props.publisher.totalAmount} {getLocale('bat')}
    </div>
    <div>
      {getLocale('contributedAmount')} {props.publisher.contributedAmount} {getLocale('bat')}
    </div>
  </blockquote>
)
