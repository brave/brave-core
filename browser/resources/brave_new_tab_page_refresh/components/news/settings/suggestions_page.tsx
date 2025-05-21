/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import * as mojom from 'gen/brave/components/brave_news/common/brave_news.mojom.m'
import PublisherCard from '../../../../../../components/brave_news/browser/resources/shared/PublisherCard'
import { getString } from '../../../lib/strings'
import { SubpageHeader } from './subpage_header'

interface Props {
  publishers: mojom.Publisher[]
  onBack: () => void
}

export function SuggestionsPage(props: Props) {
  return (
    <main>
      <SubpageHeader
        title={getString('newsSettingsSuggestionsTitle')}
        onBack={props.onBack}
      />
      <div className='source-grid'>
        {
          props.publishers.map((publisher) =>
            <PublisherCard
              key={publisher.publisherId}
              publisherId={publisher.publisherId}
            />
          )
        }
      </div>
    </main>
  )
}
