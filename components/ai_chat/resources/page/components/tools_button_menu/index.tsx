// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import ButtonMenu from '@brave/leo/react/buttonMenu'
import classnames from '$web-common/classnames'
import styles from './style.module.scss'
import { useMemo } from 'react'

interface Props<T> {
  query: string,
  isToolsMenuOpen: boolean,
  setIsToolsMenuOpen: (open: boolean) => void,
  categories: { category: string, entries: T[] }[],

  handleClick: (entry: T) => void
  getLabel: (entry: T) => string
  getSubheading: (entry: T) => string | undefined

  matchesQuery: (query: string, entry: T, category?: string) => boolean
}

export default function ToolsButtonMenu<T>(props: Props<T>) {
  const filtered = useMemo(() => props.categories
    .map(g => ({
      ...g,
      entries: g.entries
        .filter(entry => props.matchesQuery(props.query, entry, g.category))
    }))
    .filter(g => g.entries.length > 0), [props.query, props.categories])

  return (
    <ButtonMenu
      className={classnames({
        [styles.buttonMenu]: true,
        // If we're filtering the list, highlight the first item
        [styles.highlightFirstItem]: !!props.query
      })}
      isOpen={props.isToolsMenuOpen}
      onClose={() => props.setIsToolsMenuOpen(false)}
    >
      {filtered.map((category) => {
        return (
          <React.Fragment key={category.category}>
            <div className={styles.menuSectionTitle}>
              {category.category}
            </div>
            {category.entries.map((entry, i) => {
              const subheading = props.getSubheading(entry)
              if (subheading) {
                return <div key={i} className={styles.menuSubtitle}>{subheading}</div>
              } else {
                const label = props.getLabel(entry)
                return (
                  <leo-menu-item
                    key={i}
                    onClick={() => props.handleClick(entry)}
                  >
                    {label}
                  </leo-menu-item>
                )
              }
            })}
          </React.Fragment>
        )
      })}
    </ButtonMenu>
  )
}
