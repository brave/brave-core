// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'
import styled from 'styled-components'
import { getLocale } from '../../../../../common/locale'
import { OnSetPublisherPref } from '../'

type Props = {
  publisher: BraveToday.Publisher
  onSetPublisherPref: OnSetPublisherPref
  title?: boolean
}

// TODO(petemill): Make this shared and have WidgetMenu use it,
// so we're not duplicating styles, functionality and accessibility.
const isOpenClassName = 'is-open'

const PublisherMeta = styled('div')`
  width: fit-content;
`

const Trigger = styled('button')`
  appearance: none;
  display: block;
  cursor: pointer;
  position: relative;
  margin: 0;
  border: none;
  border-radius: 100px;
  padding: 0;
  background: none;
  display: flex;
  flex-direction: row;
  justify-content: flex-start;
  align-items: center;
  outline: none;
  color: inherit;
  font-weight: inherit;

  // Negative margin is ok here because we're doing a
  // "ghost" outline, but we do need to take care not to overlap any sibling
  // elements.
  --ghost-padding-v: max(4.7%, 5px);
  --ghost-padding-h: max(9%, 12px);
  padding: var(--ghost-padding-v) var(--ghost-padding-h);
  margin: calc(var(--ghost-padding-v) * -1 - 1px) calc(var(--ghost-padding-h) * -1 - 1px);
  border: solid 1px transparent;
  overflow: visible;

  &.${isOpenClassName},
  &:focus-visible,
  &:hover,
  &:active {
    border-color: inherit;
  }

  &:active {
    background-color: rgba(255, 255, 255, .2);
  }
`

const Text = styled('span')`
  max-width: 100%;
  overflow: hidden;
  text-overflow: ellipsis;
  font-family: ${p => p.theme.fontFamily.heading};
`

const Menu = styled('ul')`
  list-style: none;
  list-style-type: none;
  margin: 0;
  position: absolute;
  width: max-content;
  min-width: 166px;
  bottom: 114%;
  left: 0;
  border-radius: 4px;
  box-shadow: 0px 0px 6px 0px rgba(0, 0, 0, 0.3);
  padding: 8px 0;
  background-color: ${p => p.theme.color.contextMenuBackground};
  color:  ${p => p.theme.color.contextMenuForeground};
`

const MenuItem = styled('li')`
  list-style-type: none;
  padding: 10px 18px;
  outline: none;
  font-size: 12px;

  &:hover,
  &:focus {
    background-color: ${p => p.theme.color.contextMenuHoverBackground};
    color: ${p => p.theme.color.contextMenuHoverForeground};
  }

  &:active {
    // TODO(petemill): Theme doesn't have a context menu interactive color,
    // make one and don't make entire element opaque.
    opacity: .8;
  }

  &:focus-visible {
    outline: solid 1px ${p => p.theme.color.brandBrave};
  }
`

export default function PublisherMetaComponent (props: Props) {

  const [isMenuOpen, setIsMenuOpen] = React.useState(false)

  const triggerElementRef = React.useRef<HTMLButtonElement>(null)

  const onClickCloseMenu = React.useCallback((e: MouseEvent) => {
    const triggerElement = triggerElementRef.current
    if (!triggerElement || triggerElement.contains(e.target as Node)) {
      return
    }
    setIsMenuOpen(false)
  }, [setIsMenuOpen])

  const onKeyDown = React.useCallback((e: KeyboardEvent) => {
    if (e.defaultPrevented) {
      return
    }
    if (e.key === 'Escape') {
      setIsMenuOpen(false)
    }
  }, [setIsMenuOpen])

  const toggleMenu = React.useCallback((e: React.MouseEvent) => {
    e.stopPropagation()
    e.preventDefault()
    setIsMenuOpen((value) => !value)
  }, [setIsMenuOpen])

  // Setup or remote event listeners when opens or closes
  // or this element is removed.
  React.useEffect(() => {
    const removeEventListeners = () => {
      window.removeEventListener('click', onClickCloseMenu)
      window.removeEventListener('keydown', onKeyDown)
    }
    if (!isMenuOpen) {
      removeEventListeners()
    } else {
      // TODO(petemill): set element focus when using keyboard arrows.
      window.addEventListener('click', onClickCloseMenu)
      window.addEventListener('keydown', onKeyDown)
    }
    return removeEventListeners
  }, [isMenuOpen])

  const onClickDisablePublisher = React.useCallback(() => {
    props.onSetPublisherPref(
      props.publisher.publisher_id,
      false
    )
  }, [props.publisher, props.onSetPublisherPref])

  const commandText = React.useMemo<string>(() => {
    const raw = getLocale('braveTodayDisableSourceCommand')
    const publisherIndex = raw.indexOf('$1')
    if (publisherIndex === -1) {
      console.warn('Locale string for braveTodayDisableSourceCommand did not have a $1 replacement area for publisher name.', raw)
      return `${raw} ${props.publisher.publisher_name}`
    }
    return raw.substr(0, publisherIndex) +
      props.publisher.publisher_name +
      raw.substr(publisherIndex + 2)
  }, [props.publisher.publisher_name])

  return (
    <PublisherMeta>
      <Trigger
        className={isMenuOpen ? isOpenClassName : undefined}
        onClick={toggleMenu}
        ref={triggerElementRef}
        aria-haspopup='true'
        aria-expanded={isMenuOpen ? 'true' : 'false'}
      >
        <Text>
          {props.publisher.publisher_name}
        </Text>
        {isMenuOpen &&
          <Menu
            role='menu'
          >
            <MenuItem
              role='menuitem'
              tabIndex={0}
              onClick={onClickDisablePublisher}
            >
              {commandText}
            </MenuItem>
          </Menu>
        }
      </Trigger>
    </PublisherMeta>
  )
}
