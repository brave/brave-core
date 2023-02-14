/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { useLocaleContext } from '../lib/locale_strings'
import { formatMessage } from '../../shared/lib/locale_context'
import { NewTabLink } from '../../shared/components/new_tab_link'

import * as urls from '../../shared/lib/rewards_urls'

import * as style from './terms.style'

const feeAmount = 0.05

const percentFormatter = new Intl.NumberFormat(undefined, {
  style: 'percent'
})

export function Terms () {
  const { getString } = useLocaleContext()

  return (
    <style.root>
      <div>
        {
          formatMessage(getString('feeNotice'), [
            percentFormatter.format(feeAmount)
          ])
        }
      </div>
      <div>
        {
          formatMessage(getString('termsOfService'), {
            tags: {
              $1: (content) => (
                <NewTabLink key='terms' href={urls.termsOfServiceURL}>
                  {content}
                </NewTabLink>
              ),
              $3: (content) => (
                <NewTabLink key='pp' href={urls.privacyPolicyURL}>
                  {content}
                </NewTabLink>
              )
            }
          })
        }
      </div>
    </style.root>
  )
}
