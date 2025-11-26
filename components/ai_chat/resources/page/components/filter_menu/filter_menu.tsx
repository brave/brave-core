// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import ButtonMenu from '@brave/leo/react/buttonMenu'
import classnames from '$web-common/classnames'
import styles from './style.module.scss'
import { useMemo } from 'react'

export interface Props<T> {
  query: string | null
  isOpen: boolean
  setIsOpen: (open: boolean) => void
  categories: { category: string; entries: T[] }[]

  header?: React.ReactNode
  footer?: React.ReactNode
  noMatchesMessage?: React.ReactNode

  // Note: -1 means no match.
  matchesQuery: (query: string, entry: T, category?: string) => number

  children: (entry: T, category?: string) => React.ReactNode
}

export default function FilterMenu<T>(props: Props<T>) {
  const filtered = useMemo(
    () =>
      !props.query
        ? props.categories
        : props.categories
            .map((g) => ({
              ...g,
              entries: g.entries
                .map(
                  (entry) =>
                    [
                      props.matchesQuery(props.query!, entry, g.category),
                      entry,
                    ] as const,
                )
                // Note: -1 means no match.
                .filter(([rank]) => rank !== -1)
                .sort((a, b) => a[0] - b[0])
                .map(([_, entry]) => entry),
            }))
            .filter((g) => g.entries.length > 0),
    [props.query, props.categories],
  )

  const noMatches = useMemo(
    () => !filtered.some((g) => g.entries.length !== 0),
    [filtered],
  )
  const ref = React.useRef<HTMLElement>(null)

  React.useEffect(() => {
    props.setIsOpen(props.query !== null)
  }, [props.query])

  React.useEffect(() => {
    if (!props.isOpen) return

    const handler = (e: KeyboardEvent) => {
      const setHandled = () => {
        e.preventDefault()
        e.stopPropagation()
      }

      const focused = ref.current?.querySelector<HTMLElement>(':focus')
      // If there isn't a focused element, click the first menu item, if any.
      const menuItem =
        focused ?? ref.current?.querySelector<HTMLElement>('leo-menu-item')

      const accepts =
        e.key === 'Enter'
        || e.key === 'Tab'
        || (e.key === ' ' && !menuItem?.textContent?.includes(' '))
      if (accepts) {
        setHandled()

        if (!menuItem) return

        menuItem.click()
      }

      // As we select the first item by default we want to skip it when the user
      // presses one of the arrow keys.
      let direction = 0
      if (e.key === 'ArrowDown') direction = 1
      if (e.key === 'ArrowUp') direction = -1

      if (direction !== 0 && !focused) {
        setHandled()
        const items: HTMLElement[] = Array.from(
          ref.current?.querySelectorAll('leo-menu-item') ?? [],
        )
        items.at(direction)?.focus()
      }
    }
    document.addEventListener('keydown', handler, { capture: true })

    return () => {
      document.removeEventListener('keydown', handler, { capture: true })
    }
  }, [props.isOpen, props.query])

  return (
    <ButtonMenu
      // This is weird but there seems to be a bug in Nala where the menu is not
      // being opened/closed after the Attachments sidebar is shown.
      // https://github.com/brave/brave-browser/issues/48262#issuecomment-3166296480
      // This should be removed once the underlying issue in Nala is fixed.
      // https://github.com/brave/leo/issues/1170
      key={props.isOpen.toString()}
      ref={ref}
      className={classnames({
        [styles.buttonMenu]: true,
        // If we're filtering the list, highlight the first item
        [styles.highlightFirstItem]: props.query !== null,
      })}
      isOpen={props.isOpen}
      onClose={() => {
        setTimeout(() => props.setIsOpen(false))
        return false
      }}
      placement='top-start'
    >
      {props.header}
      {filtered.map((category) => {
        return (
          <React.Fragment key={category.category}>
            {category.category && (
              <div className={styles.menuSectionTitle}>{category.category}</div>
            )}
            {category.entries.map((entry, i) => (
              <React.Fragment key={i}>
                {props.children(entry, category.category)}
              </React.Fragment>
            ))}
          </React.Fragment>
        )
      })}
      {noMatches && props.noMatchesMessage}
      {props.footer}
    </ButtonMenu>
  )
}
