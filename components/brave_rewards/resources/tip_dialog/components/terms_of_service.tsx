/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { HostContext } from '../lib/host_context'

import { NewTabLink } from './new_tab_link'

import * as style from './terms_of_service.style'

function getMessageWithLink (input: string, url: string) {
  const parts = input.split(/\$\d/g)
  if (parts.length < 2) {
    return input
  }

  return (
    <>
      {parts[0]}
      <NewTabLink href={url}>{parts[1]}</NewTabLink>
      {parts.slice(2).join()}
    </>
  )
}

export function TermsOfService () {
  const { getString } = React.useContext(HostContext)
  const url = 'https://basicattentiontoken.org/user-terms-of-service'
  return (
    <style.terms>
      {getMessageWithLink(getString('termsOfService'), url)}
    </style.terms>
  )
}
