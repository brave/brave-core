/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import Button from '@brave/leo/react/button'
import Checkbox from '@brave/leo/react/checkbox'

import { LocaleContext, formatMessage } from '../../shared/lib/locale_context'
import { NewTabLink } from '../../shared/components/new_tab_link'
import { PageModal } from './page_modal'

import * as style from './reset_modal.style'

const supportURL = 'https://support.brave.com/hc/en-us/articles/10007969237901'

interface Props {
  onReset: () => void
  onClose: () => void
}

export function ResetModal (props: Props) {
  const { getString } = React.useContext(LocaleContext)
  const [consented, setConsented] = React.useState(false)

  return (
    <PageModal onClose={props.onClose}>
      <style.root data-test-id='rewards-reset-modal'>
        <style.title>
          {getString('resetWallet')}
        </style.title>
        <style.text>
          {
            formatMessage(getString('rewardsResetText'), {
              tags: {
                $1: (content) => (
                  <NewTabLink key='link' href={supportURL}>
                    {content}
                  </NewTabLink>
                )
              }
            })
          }
        </style.text>
        <style.consent>
          <Checkbox checked={consented} onChange={({checked}) => setConsented(checked)}>
            <style.consentLabel>
              {
                formatMessage(getString('rewardsResetConsent'), {
                  tags: {
                    $1: (content) => (
                      <NewTabLink key='link' href={supportURL}>
                        {content}
                      </NewTabLink>
                    )
                  }
                })
              }
            </style.consentLabel>
          </Checkbox>
        </style.consent>
        <style.action>
          <Button onClick={props.onReset} isDisabled={!consented}>
            {getString('reset')}
          </Button>
        </style.action>
      </style.root>
    </PageModal>
  )
}
