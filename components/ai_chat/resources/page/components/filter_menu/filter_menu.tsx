// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import ButtonMenu from '@brave/leo/react/buttonMenu'
import Icon from '@brave/leo/react/icon'
import classnames from '$web-common/classnames'
import styles from './style.module.scss'
import { useMemo } from 'react'
import { FuzzyFinder, Match } from './fuzzy_finder'

const MAX_RESULTS = 100

export interface Props<T> {
  query: string | null
  isOpen: boolean
  setIsOpen: (open: boolean) => void
  categories: { category: string; entries: T[] }[]

  header?: React.ReactNode
  footer?: React.ReactNode
  noMatchesMessage?: React.ReactNode
  onAutoSelect?: () => void

  onResultsChanged?: (results: { category: string; entries: T[] }[]) => void
  isFullWidth?: boolean

  // Note: undefined means no match.
  matchesQuery: (
    query: FuzzyFinder,
    entry: T,
    category?: string,
  ) => Match | undefined

  children: (entry: T, category?: string, match?: Match) => React.ReactNode
}

function FilterMenuCategoryGroup<T>(props: {
  entries: T[]
  lookup: Map<T, Match | undefined>
  renderEntry: Props<T>['children']
  highlightFirstItem: boolean
  categoryLabel?: string
}) {
  const [isExpanded, setIsExpanded] = React.useState(true)

  return (
    <>
      {props.categoryLabel && (
        <button
          className={styles.menuSectionTitle}
          onClick={(e) => {
            e.stopPropagation()
            e.preventDefault()
            setIsExpanded((open) => !open)
          }}
        >
          {props.categoryLabel}
          <Icon name={isExpanded ? 'carat-up' : 'carat-down'} />
        </button>
      )}
      <leo-menu-section
        class={classnames(styles.categoryEntriesWrap, {
          [styles.categoryEntriesWrapExpanded]:
            !props.categoryLabel || isExpanded,
          [styles.highlightFirstItem]: props.highlightFirstItem,
        })}
      >
        {props.entries.map((entry, i) => (
          <React.Fragment key={i}>
            {props.renderEntry(
              entry,
              props.categoryLabel,
              props.lookup.get(entry),
            )}
          </React.Fragment>
        ))}
      </leo-menu-section>
    </>
  )
}

export function MatchedText(props: { text: string; match?: Match }) {
  if (!props.match) {
    return props.text
  }

  let lastIndex = 0
  const parts: React.ReactNode[] = []
  for (const range of props.match.ranges) {
    parts.push(props.text.slice(lastIndex, range.start))
    parts.push(
      <span className={styles.matchedText}>
        {props.text.slice(range.start, range.end)}
      </span>,
    )
    lastIndex = range.end
  }
  parts.push(props.text.slice(lastIndex))

  return <span>{...parts}</span>
}

export default function FilterMenu<T>(props: Props<T>) {
  const [filtered, lookup] = useMemo(() => {
    // If the menu isn't open don't do any filtering.
    if (!props.isOpen)
      return [props.categories, new Map<T, Match | undefined>()]

    const lookup = new Map<T, Match | undefined>()
    return [
      !props.query
        ? props.categories
        : props.categories
            .map((g) => ({
              ...g,
              entries: g.entries
                .map((entry) => {
                  const match = props.matchesQuery(
                    new FuzzyFinder(props.query ?? ''),
                    entry,
                    g.category,
                  )
                  lookup.set(entry, match)
                  return [match, entry] as const
                })
                .filter(([match]) => match)
                .sort((a, b) => b[0]!.score - a[0]!.score)
                .map(([_, entry]) => entry),
            }))
            .filter((g) => g.entries.length > 0)
            .slice(0, MAX_RESULTS),
      lookup,
    ]
  }, [props.query, props.categories, props.isOpen])

  React.useEffect(() => {
    props.onResultsChanged?.(filtered)
  }, [filtered])

  const noMatches = useMemo(
    () => !filtered.some((g) => g.entries.length !== 0),
    [filtered],
  )
  const firstNamedCategoryIndex = filtered.findIndex((g) => g.category)
  const firstFlatCategoryIndex = filtered.findIndex((g) => !g.category)
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

        props.onAutoSelect?.()
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
        [styles.fullWidth]: props.isFullWidth,
      })}
      isOpen={props.isOpen}
      onClose={() => {
        setTimeout(() => props.setIsOpen(false))
        return false
      }}
      widthIsMaxWidth
    >
      {props.header}
      {filtered.map((category, categoryIndex) => {
        // Idle row tint (see .highlightFirstItem): first named section when any
        // exist; otherwise the first flat (empty title) block only.
        const highlightFirstItem =
          props.query !== null
          && (category.category
            ? categoryIndex === firstNamedCategoryIndex
            : firstNamedCategoryIndex < 0
              && categoryIndex === firstFlatCategoryIndex)

        return (
          <FilterMenuCategoryGroup
            key={category.category || `filter-section-${categoryIndex}`}
            categoryLabel={category.category}
            entries={category.entries}
            lookup={lookup}
            renderEntry={props.children}
            highlightFirstItem={highlightFirstItem}
          />
        )
      })}
      {noMatches && props.noMatchesMessage}
      {props.footer}
    </ButtonMenu>
  )
}
