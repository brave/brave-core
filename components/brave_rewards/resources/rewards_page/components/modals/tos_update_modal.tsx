/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { Modal } from '../common/modal'

import { formatString } from '$web-common/formatString'
import { useLocaleContext } from '../../lib/locale_strings'
import { NewTabLink } from '../../../shared/components/new_tab_link'
import * as urls from '../../../shared/lib/rewards_urls'

import { style } from './tos_update_modal.style'

interface Props {
  onAccept: () => void
  onReset: () => void
}

export function TosUpdateModal(props: Props) {
  const { getString } = useLocaleContext()
  return (
    <Modal>
      <Modal.Header title={getString('tosUpdateRequiredTitle')} />
      <div data-css-scope={style.scope}>
        <div>
          {formatString(getString('tosUpdateRequiredText'), {
            $1: (content) => <button onClick={props.onReset}>{content}</button>,
          })}
        </div>
        <div>
          {formatString(getString('tosUpdateLink'), {
            $1: (content) => (
              <NewTabLink href={urls.termsOfServiceURL}>{content}</NewTabLink>
            ),
          })}
        </div>
      </div>
      <Modal.Actions
        actions={[
          {
            text: getString('tosUpdateAcceptButtonLabel'),
            onClick: props.onAccept,
            isPrimary: true,
            autoFocus: true,
          },
        ]}
      />
    </Modal>
  )
}
