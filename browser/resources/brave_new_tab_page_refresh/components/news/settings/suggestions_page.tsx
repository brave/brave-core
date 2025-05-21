/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { getString } from '../../../lib/strings'
import { Publisher } from '../../../state/news_state'
import { PublisherSourceCard } from '../source_card'
import { SubpageHeader } from './subpage_header'

interface Props {
  publishers: Publisher[]
  onBack: () => void
}

export function SuggestionsPage(props: Props) {
  return (
    <main>
      <SubpageHeader
        title={getString(S.BRAVE_NEWS_SUGGESTIONS_TITLE)}
        onBack={props.onBack}
      />
      <div className='source-grid'>
        {props.publishers.map((publisher) => (
          <PublisherSourceCard
            key={publisher.publisherId}
            publisher={publisher}
          />
        ))}
      </div>
    </main>
  )
}
