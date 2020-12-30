/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { LocaleContext, formatMessageParts } from '../../lib/locale_context'

import { NewTabLink } from '../new_tab_link'

export function TermsOfService () {
  const { getString } = React.useContext(LocaleContext)
  return formatMessageParts(getString('onboardingTerms'),
    (before, link1, between, link2, ...after) => (
      <>
        {before}
        <NewTabLink href={'https://basicattentiontoken.org/user-terms-of-service'}>
          {link1}
        </NewTabLink>
        {between}
        <NewTabLink href={'https://brave.com/privacy/#rewards'}>
          {link2}
        </NewTabLink>
        {after.join()}
      </>
    ))
}
