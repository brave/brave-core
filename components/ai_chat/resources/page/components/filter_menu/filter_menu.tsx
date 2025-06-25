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
  query: string | null,
  isOpen: boolean,
  setIsOpen: (open: boolean) => void,
  categories: { category: string, entries: T[] }[],

  matchesQuery: (query: string, entry: T, category?: string) => boolean | undefined

  children: (entry: T, category?: string) => React.ReactNode
}

export default function FilterMenu<T>(props: Props<T>) {
  const filtered = useMemo(() => !props.query
    ? props.categories
    : props.categories
      .map(g => ({
        ...g,
        entries: g.entries
          .filter(entry => props.matchesQuery(props.query!, entry, g.category))
      }))
      .filter(g => g.entries.length > 0), [props.query, props.categories])

  const ref = React.useRef<HTMLElement>(null)

  React.useEffect(() => {
    props.setIsOpen(props.query !== null)
  }, [props.query])

  React.useEffect(() => {
    if (!props.isOpen) return

    const handler = (e: KeyboardEvent) => {
      if (e.key !== 'Enter') return

      e.preventDefault()
      e.stopPropagation();

      const focused = ref.current?.querySelector<HTMLElement>(':focus')
      // If there isn't a focused element, click the first menu item, if any.
      const menuItem = focused ?? ref.current?.querySelector<HTMLElement>('leo-menu-item')
      if (!menuItem) return

      menuItem.click()
    }
    document.addEventListener('keydown', handler, { capture: true })

    return () => {
      document.removeEventListener('keydown', handler, { capture: true })
    }
  }, [props.isOpen, props.query])

  return (
    <ButtonMenu
      ref={ref}
      className={classnames({
        [styles.buttonMenu]: true,
        // If we're filtering the list, highlight the first item
        [styles.highlightFirstItem]: props.query !== null
      })}
      isOpen={props.isOpen}
      onClose={() => props.setIsOpen(false)}
      placement="top"
    >
      {filtered.map((category) => {
        return (
          <React.Fragment key={category.category}>
            <div className={styles.menuSectionTitle}>
              {category.category}
            </div>
            {category.entries.map((entry, i) => <React.Fragment key={i}>
              {props.children(entry, category.category)}
            </React.Fragment>)}
          </React.Fragment>
        )
      })}
    </ButtonMenu>
  )
}
