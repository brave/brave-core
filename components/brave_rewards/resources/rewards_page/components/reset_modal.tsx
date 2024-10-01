/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Icon from '@brave/leo/react/icon'
import Checkbox from '@brave/leo/react/checkbox'

import { useLocaleContext } from '../lib/locale_strings'
import { formatMessage } from '../../shared/lib/locale_context'
import { NewTabLink } from '../../shared/components/new_tab_link'
import { Modal } from './modal'
import * as urls from '../../shared/lib/rewards_urls'

import { style } from './reset_modal.style'

interface Props {
  onReset: () => void
  onClose: () => void
}

export function ResetModal(props: Props) {
  const { getString } = useLocaleContext()
  const [consented, setConsented] = React.useState(false)

  return (
    <Modal className='reset-modal' onEscape={props.onClose}>
      <Modal.Header
        title={getString('resetRewardsTitle')}
        onClose={props.onClose}
      />
      <div {...style}>
        <div className='message-icon'>
          <Icon name='warning-triangle-filled' />
        </div>
        <div className='text'>
          {
            formatMessage(getString('resetRewardsText'), {
              tags: {
                $1: (content) => (
                  <NewTabLink key='link' href={urls.resetSupportURL}>
                    {content}
                  </NewTabLink>
                )
              }
            })
          }
        </div>
        <div>
          <Checkbox
            checked={consented}
            onChange={({checked}) => setConsented(checked)}
          >
            <div>
              {
                formatMessage(getString('resetConsentText'), {
                  tags: {
                    $1: (content) => (
                      <NewTabLink key='link' href={urls.resetSupportURL}>
                        {content}
                      </NewTabLink>
                    )
                  }
                })
              }
            </div>
          </Checkbox>
        </div>
      </div>
      <Modal.Actions
        actions={[
          {
            text: getString('cancelButtonLabel'),
            onClick: props.onClose
          },
          {
            className: 'reset-button',
            text: getString('resetButtonLabel'),
            onClick: props.onReset,
            isDisabled: !consented,
            isPrimary: true
          }
        ]}
      />
    </Modal>
  )
}
