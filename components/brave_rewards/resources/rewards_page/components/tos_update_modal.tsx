/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { Modal } from './modal'

import { formatMessage } from '../../shared/lib/locale_context'
import { useLocaleContext } from '../lib/locale_strings'
import { NewTabLink } from '../../shared/components/new_tab_link'
import * as urls from '../../shared/lib/rewards_urls'

import { style } from './tos_update_modal.style'

interface Props {
  onAccept: () => void
  onReset: () => void
}

export function TosUpdateModal(props: Props) {
  const { getString } = useLocaleContext()

  const onMount = (elem: HTMLElement | null) => {
    if (elem) {
      const link = elem.querySelector<HTMLLinkElement>('a')
      if (link) {
        link.focus()
      }
    }
  }

  return (
    <Modal>
      <Modal.Header title={getString('tosUpdateRequiredTitle')} />
      <div {...style}>
        <div>
          {
            formatMessage(getString('tosUpdateRequiredText'), {
              tags: {
                $1: (content) => (
                  <button key='reset' onClick={props.onReset}>
                    {content}
                  </button>
                )
              }
            })
          }
        </div>
        <div ref={onMount}>
          {
            formatMessage(getString('tosUpdateLink'), {
              tags: {
                $1: (content) => (
                  <NewTabLink key='link' href={urls.termsOfServiceURL}>
                    {content}
                  </NewTabLink>
                )
              }
            })
          }
        </div>
      </div>
      <Modal.Actions
        actions={[
          {
            text: getString('tosUpdateAcceptButtonLabel'),
            onClick: props.onAccept,
            isPrimary: true
          }
        ]}
      />
    </Modal>
  )
}
