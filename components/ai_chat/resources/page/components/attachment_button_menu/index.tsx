/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import ButtonMenu from '@brave/nala/react/buttonMenu'
import Button from '@brave/nala/react/button'
import Icon from '@brave/nala/react/icon'
import classnames from '$web-common/classnames'
import { ConversationContext } from '../../state/conversation_context'
import { AIChatContext, useAIChat } from '../../state/ai_chat_context'
import {
  isMCPServerContent,
  shouldDisableAttachmentsButton,
} from '../../../common/conversation_history_utils'
import useAttachableConnectors, {
  AttachableConnectors,
} from '../../hooks/useAttachableConnectors'
import ConnectorsMenu from './connectors_menu'

// Utils
import { getLocale } from '$web-common/locale'
import { pickFiles } from '$web-common/uploadFile'

// Styles
import styles from './style.module.scss'

type Props = Pick<
  ConversationContext,
  | 'attachFiles'
  | 'getScreenshots'
  | 'conversationHistory'
  | 'associatedContentInfo'
  | 'setAttachmentsDialog'
  | 'associateDefaultContent'
  | 'unassociatedTabs'
  | 'focusInput'
>
  & Pick<AIChatContext, 'isMobile'> & {
    conversationStarted: boolean
    // When provided, shows an entry that opens a local "workspace" folder for
    // Leo's experimental workspace tools. Undefined hides the entry.
    onSelectWorkspaceFolder?: () => void
    // Whether to show a sub-menu that attaches the user's connectors to the
    // conversation.
    canAttachConnectors?: boolean
  }

// On Android we don't run the SandboxedTextExtractor or PdfExtractor, so we need some mime protections.
const accept =
  // <if expr="is_android">
  'image/*,application/pdf'
// <else>
undefined
// </if>

export default function AttachmentButtonMenu(props: Props) {
  return props.canAttachConnectors ? (
    <MenuWithConnectors {...props} />
  ) : (
    <Menu {...props} />
  )
}

function MenuWithConnectors(props: Props) {
  const aiChat = useAIChat()
  const connectors = useAttachableConnectors()
  return (
    <Menu
      {...props}
      connectors={connectors}
      onAddConnector={() => aiChat.setConnectorsDialog({ view: 'editor' })}
    />
  )
}

type View = 'main' | 'connectors'

const CONNECTORS_ENTRY = '[data-testid="connectors"]'

function Menu(
  props: Props & {
    connectors?: AttachableConnectors
    onAddConnector?: () => void
  },
) {
  const [isOpen, setIsOpen] = React.useState(false)
  const [view, setView] = React.useState<View>('main')
  const menuRef = React.useRef<HTMLElement>(null)
  // What to focus once the menu of the view being switched to has opened.
  const focusOnOpen = React.useRef<string>(undefined)
  // The `data-focus-key` of the connectors' item that last had focus.
  const focusedConnectorsItem = React.useRef<string>(undefined)
  const viewSwitch = React.useRef<number>(undefined)

  const isMenuDisabled = shouldDisableAttachmentsButton(
    props.conversationHistory,
  )

  const hasAssociatedContent =
    // Screenshots are not supported on iOS.
    // <if expr="!is_ios">
    props.associatedContentInfo.some((content) => !isMCPServerContent(content))
  // <else>
  false
  // </if>

  const showView = (next: View, focusSelector: string) => {
    focusOnOpen.current = focusSelector
    focusedConnectorsItem.current = undefined
    window.clearTimeout(viewSwitch.current)
    // A menu that opens during a click takes the click for one outside it, and
    // closes, so the menu of the next view waits until the click is over.
    viewSwitch.current = window.setTimeout(() => setView(next))
  }

  React.useEffect(() => () => window.clearTimeout(viewSwitch.current), [])

  const connectorsKey = props.connectors?.servers
    .map((server) => server.id)
    .join()

  React.useEffect(() => {
    const menu = menuRef.current
    if (view !== 'connectors' || !menu) {
      return
    }
    const onFocusIn = (event: FocusEvent) => {
      const item = (event.target as Element).closest('[data-focus-key]')
      if (item) {
        focusedConnectorsItem.current = item.getAttribute('data-focus-key')!
      }
    }
    menu.addEventListener('focusin', onFocusIn)
    return () => menu.removeEventListener('focusin', onFocusIn)
  }, [view, connectorsKey])

  React.useEffect(() => {
    let selector = focusOnOpen.current
    focusOnOpen.current = undefined
    // A new list of connectors opens a new menu, which takes focus away from
    // the item that had it.
    if (!selector && view === 'connectors' && focusedConnectorsItem.current) {
      selector = `[data-focus-key="${focusedConnectorsItem.current}"]`
    }
    if (!selector) {
      return
    }
    // Leo's menu shows its items after this effect runs, so they're focused
    // once it has.
    const timer = window.setTimeout(() => {
      const menu = menuRef.current
      // Focus that has moved out of the menu, such as to the composer, is left
      // where it is.
      const active = document.activeElement
      if (active && active !== document.body && !menu?.contains(active)) {
        return
      }
      const item =
        menu?.querySelector<HTMLElement>(selector)
        ?? menu?.querySelector<HTMLElement>('leo-menu-item')
      item?.focus()
    })
    return () => window.clearTimeout(timer)
  }, [view, connectorsKey])

  const menuProps: React.ComponentProps<typeof ButtonMenu> = {
    ref: menuRef,
    positionStrategy: 'fixed',
    // Leo's web component reflects a `false` prop to an attribute, and reads
    // the attribute back as the string "false", which opens the menu. So the
    // prop is left unset while the menu is closed.
    isOpen: isOpen || undefined,
    onChange: ({ isOpen }) => {
      setIsOpen(isOpen)
      // The menu always opens on the rest of its items, whatever view a
      // pending switch was for.
      window.clearTimeout(viewSwitch.current)
      focusOnOpen.current = undefined
      focusedConnectorsItem.current = undefined
      setView('main')
    },
  }

  const anchor = (
    <div
      slot='anchor-content'
      className={styles.anchor}
    >
      <Button
        fab
        kind='plain-faint'
        title={getLocale(S.AI_CHAT_LEO_ATTACHMENT_MENU_BUTTON_LABEL)}
        isDisabled={isMenuDisabled}
        data-testid='attachment-button'
      >
        <Icon name='attachment' />
      </Button>
    </div>
  )

  // Leo's menu collects its items once, as it opens, and can't move to or
  // select an item added after that. So the connectors are a menu of their
  // own, opened afresh whenever the list of them changes.
  if (view === 'connectors' && props.connectors) {
    return (
      <ButtonMenu
        {...menuProps}
        key={`connectors:${connectorsKey}`}
        onClose={({ reason }) => {
          // Escape goes back to the rest of the menu.
          if (reason === 'cancel') {
            showView('main', CONNECTORS_ENTRY)
            return false
          }
          return undefined
        }}
      >
        {anchor}
        <ConnectorsMenu
          connectors={props.connectors}
          associatedContent={props.associatedContentInfo}
          onBack={() => showView('main', CONNECTORS_ENTRY)}
          onAdd={() => props.onAddConnector?.()}
        />
      </ButtonMenu>
    )
  }

  return (
    <>
      <ButtonMenu {...menuProps}>
        {anchor}
        <leo-menu-item
          data-testid='attach-file'
          onClick={async () => {
            const files = await pickFiles({
              multiple: true,
              accept,
            })
            if (files.length > 0) {
              props.attachFiles(files)
            }
            props.focusInput()
          }}
        >
          <div className={styles.buttonContent}>
            <Icon
              className={styles.buttonIcon}
              name='upload'
            />
            {getLocale(S.AI_CHAT_UPLOAD_FILE_BUTTON_LABEL)}
          </div>
        </leo-menu-item>
        {hasAssociatedContent && (
          <leo-menu-item
            data-testid='screenshot'
            onClick={() => {
              props.getScreenshots()
              props.focusInput()
            }}
          >
            <div className={styles.buttonContent}>
              <Icon
                className={styles.buttonIcon}
                name='screenshot'
              />
              {getLocale(S.AI_CHAT_SCREENSHOT_BUTTON_LABEL)}
            </div>
          </leo-menu-item>
        )}
        {props.isMobile && (
          <leo-menu-item
            onClick={async () => {
              const files = await pickFiles({
                capture: 'environment',
                accept,
              })
              if (files.length > 0) {
                props.attachFiles(files)
              }
              props.focusInput()
            }}
          >
            <div className={styles.buttonContent}>
              <Icon
                className={styles.buttonIcon}
                name='camera'
              />
              {getLocale(S.AI_CHAT_TAKE_A_PICTURE_BUTTON_LABEL)}
            </div>
          </leo-menu-item>
        )}
        {props.associateDefaultContent && (
          <leo-menu-item
            onClick={() => {
              props.associateDefaultContent?.associate()
              props.focusInput()
            }}
          >
            <div className={styles.buttonContent}>
              <img
                className={styles.favicon}
                src={`//favicon2?size=64&pageUrl=${encodeURIComponent(props.associateDefaultContent.url)}&allowGoogleServerFallback=0`}
                alt=''
              />
              {getLocale(S.AI_CHAT_CURRENT_TAB_CONTENTS_BUTTON_LABEL)}
            </div>
          </leo-menu-item>
        )}
        {props.unassociatedTabs.length > 0 && (
          <leo-menu-item
            data-testid='open-tabs'
            onClick={() => props.setAttachmentsDialog('tabs')}
          >
            <div className={styles.buttonContent}>
              <Icon
                className={styles.buttonIcon}
                name='window-tabs'
              />
              {getLocale(S.AI_CHAT_ATTACH_OPEN_TABS_BUTTON_LABEL)}
            </div>
          </leo-menu-item>
        )}
        <leo-menu-item onClick={() => props.setAttachmentsDialog('bookmarks')}>
          <div className={styles.buttonContent}>
            <Icon
              className={styles.buttonIcon}
              name='window-bookmark'
            />
            {getLocale(S.AI_CHAT_ATTACH_OPEN_BOOKMARKS_BUTTON_LABEL)}
          </div>
        </leo-menu-item>
        <leo-menu-item onClick={() => props.setAttachmentsDialog('history')}>
          <div className={styles.buttonContent}>
            <Icon
              className={styles.buttonIcon}
              name='history'
            />
            {getLocale(S.AI_CHAT_ATTACH_OPEN_HISTORY_BUTTON_LABEL)}
          </div>
        </leo-menu-item>
        {props.onSelectWorkspaceFolder && (
          <leo-menu-item
            data-testid='workspace-folder'
            onClick={() => {
              props.onSelectWorkspaceFolder?.()
              props.focusInput()
            }}
          >
            <div className={styles.buttonContent}>
              <Icon
                className={styles.buttonIcon}
                name='folder-code'
              />
              {getLocale(S.AI_CHAT_WORKSPACE_FOLDER_BUTTON_LABEL)}
            </div>
          </leo-menu-item>
        )}
        {props.connectors && (
          <leo-menu-item
            data-testid='connectors'
            // Shows the connectors in place of the rest of the menu.
            data-is-interactive='true'
            aria-haspopup='menu'
            onClick={() => showView('connectors', 'leo-menu-item')}
          >
            <div className={styles.buttonContent}>
              <Icon
                className={styles.buttonIcon}
                name='cloud-hosting'
              />
              <span className={styles.label}>
                {getLocale(S.CHAT_UI_CONNECTORS_MENU_LABEL)}
              </span>
              <Icon
                className={classnames(
                  styles.buttonIcon,
                  styles.directionalIcon,
                )}
                name='carat-right'
              />
            </div>
          </leo-menu-item>
        )}
      </ButtonMenu>
    </>
  )
}
