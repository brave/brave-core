// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import Icon from '@brave/nala/react/icon'
import Toggle from '@brave/nala/react/toggle'
import classnames from '$web-common/classnames'
import { getLocale } from '$web-common/locale'
import * as Mojom from '../../../common/mojom'
import { findConnectorContent } from '../../../common/conversation_history_utils'
import { AttachableConnectors } from '../../hooks/useAttachableConnectors'
import styles from './style.module.scss'

interface Props {
  connectors: AttachableConnectors
  associatedContent: Mojom.AssociatedContent[]
  onBack: () => void
  onAdd: () => void
}

// Leo's menu clicks the item that has focus when Enter or Space is pressed in
// it. A switch handles those keys itself, so they're kept from the menu, or
// they'd toggle the connector a second time.
function keepKeysFromMenu(element: HTMLElement | null) {
  element?.addEventListener('keypress', (event) => event.stopPropagation())
}

/**
 * The connectors section of the attachments menu: a toggle for each of the
 * user's connectors that attaches it to the conversation, and a way to add
 * one. Each item has a `data-focus-key`, so that focus can be given back to it
 * when the menu is opened afresh.
 */
export default function ConnectorsMenu(props: Props) {
  const { servers } = props.connectors

  return (
    <>
      <leo-menu-item
        data-testid='connectors-back'
        data-focus-key='back'
        data-is-interactive='true'
        title={getLocale(S.CHAT_UI_CONNECTORS_MENU_BACK_LABEL)}
        onClick={props.onBack}
      >
        <div className={styles.buttonContent}>
          <Icon
            className={classnames(styles.buttonIcon, styles.directionalIcon)}
            name='carat-left'
          />
          <span className={styles.menuTitle}>
            {getLocale(S.CHAT_UI_CONNECTORS_MENU_LABEL)}
          </span>
        </div>
      </leo-menu-item>
      {servers.map((server) => {
        const content = findConnectorContent(props.associatedContent, server.id)
        // One that's attached with its tools turned off isn't in use.
        const isAttached = !!content?.toolsAttached
        const setAttached = (attached: boolean) => {
          if (attached) {
            props.connectors.attach(server)
          } else if (content) {
            props.connectors.detach(content)
          }
        }
        return (
          <leo-menu-item
            key={server.id}
            data-testid='connector-toggle'
            data-focus-key={`connector:${server.id}`}
            // Stays open, so that the user sees the toggle change.
            data-is-interactive='true'
            onClick={() => setAttached(!isAttached)}
          >
            <div className={styles.buttonContent}>
              <Icon
                className={styles.buttonIcon}
                name='cloud-hosting'
              />
              <span
                className={styles.connectorName}
                title={server.name}
              >
                {server.name}
              </span>
              <Toggle
                ref={keepKeysFromMenu}
                size='small'
                checked={isAttached}
                onChange={({ checked }) => setAttached(checked)}
              />
            </div>
          </leo-menu-item>
        )
      })}
      {servers.length > 0 && <hr />}
      <leo-menu-item
        data-testid='add-connector'
        data-focus-key='add'
        onClick={props.onAdd}
      >
        <div className={styles.buttonContent}>
          <Icon
            className={styles.buttonIcon}
            name='plus-add'
          />
          {getLocale(S.CHAT_UI_CONNECTORS_MENU_ADD_LABEL)}
        </div>
      </leo-menu-item>
    </>
  )
}
