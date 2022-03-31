/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { LocaleContext, formatMessage } from '../lib/locale_context'
import { NewTabLink } from './new_tab_link'

import * as urls from '../lib/rewards_urls'

interface Props {
  text?: string
}

export function TermsOfService (props: Props) {
  const { getString } = React.useContext(LocaleContext)
  const message = props.text || getString('onboardingTerms')
  return (
    <span>
      {
        formatMessage(message, {
          tags: {
            $1: (content) => (
              <NewTabLink key='terms' href={urls.termsOfServiceURL}>
                {content}
              </NewTabLink>
            ),
            $3: (content) => (
              <NewTabLink key='privacy' href={urls.privacyPolicyURL}>
                {content}
              </NewTabLink>
            )
          }
        })
      }
    </span>
  )
}
