/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { LocaleContext, formatMessage } from '../lib/locale_context'

import { NewTabLink } from './new_tab_link'

import * as style from './terms_of_service.style'

export const termsOfServiceURL =
  'https://basicattentiontoken.org/user-terms-of-service'

export const privacyPolicyURL = 'https://brave.com/privacy/#rewards'

export function TermsOfService () {
  const { getString } = React.useContext(LocaleContext)
  return (
    <style.terms>
      {
        formatMessage(getString('onboardingTerms'), {
          tags: {
            $1: (content) => (
              <NewTabLink key='terms' href={termsOfServiceURL}>
                {content}
              </NewTabLink>
            ),
            $3: (content) => (
              <NewTabLink key='privacy' href={privacyPolicyURL}>
                {content}
              </NewTabLink>
            )
          }
        })
      }
    </style.terms>
  )
}
