/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { LocaleContext } from '../../shared/lib/locale_context'

import { NewTabLink } from '../../shared/components/new_tab_link'

import * as style from './terms_of_service.style'

function getMessageWithLink (input: string) {
  const parts = input.split(/\$\d/g)
  if (parts.length < 4) {
    return input
  }

  return (
    <>
      {parts[0]}
      <NewTabLink href={'https://basicattentiontoken.org/user-terms-of-service'}>{parts[1]}</NewTabLink>
      {parts[2]}
      <NewTabLink href={'https://brave.com/privacy/#rewards'}>{parts[3]}</NewTabLink>
      {parts.slice(4).join()}
    </>
  )
}

export function TermsOfService () {
  const { getString } = React.useContext(LocaleContext)
  return (
    <style.terms>
      {getMessageWithLink(getString('termsOfService'))}
    </style.terms>
  )
}
