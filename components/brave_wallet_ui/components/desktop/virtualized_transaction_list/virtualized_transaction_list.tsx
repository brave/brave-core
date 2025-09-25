// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Types
import { SearchableTransaction } from '../../../utils/search-utils'
import { SerializableTransactionInfo } from '../../../constants/types'

// Components
import {
  PortfolioTransactionItem, //
} from '../portfolio_transaction_item/portfolio_transaction_item'

// Styled Components
import { Column } from '../../shared/style'

// Memoized transaction item component
const MemoizedTransactionItem = React.memo(PortfolioTransactionItem)

type Transaction = SearchableTransaction | SerializableTransactionInfo

interface Props {
  transactionList: Transaction[]
  onSelectTransaction?: (transaction: Transaction) => void
}

export const VirtualizedTransactionList = React.memo((props: Props) => {
  const { transactionList, onSelectTransaction } = props

  // State
  // Start with 5 items
  const [visibleCount, setVisibleCount] = React.useState(5)
  const containerRef = React.useRef<HTMLDivElement>(null)

  const itemsPerLoad = 5 // Load 5 more items each time
  const loadThreshold = 200 // Load more when within 200px of bottom

  // Get the items to render (memoized to prevent unnecessary recalculations)
  const visibleItems = React.useMemo(
    () => transactionList.slice(0, visibleCount),
    [transactionList, visibleCount],
  )

  // Memoize the click handler to prevent unnecessary re-renders
  const handleTransactionClick = React.useCallback(
    (transaction: Transaction) => {
      onSelectTransaction?.(transaction)
    },
    [onSelectTransaction],
  )

  // Load more items when we're near the bottom
  const loadMoreItems = React.useCallback(() => {
    if (visibleCount < transactionList.length) {
      setVisibleCount((prev) =>
        Math.min(prev + itemsPerLoad, transactionList.length),
      )
    }
  }, [visibleCount, transactionList.length, itemsPerLoad])

  // Use Intersection Observer to detect when we're near the bottom
  React.useEffect(() => {
    if (!containerRef.current) return

    // Create a sentinel element to detect when we're near the bottom
    const sentinel = document.createElement('div')
    sentinel.style.height = '1px'
    sentinel.style.width = '100%'

    // Add sentinel to the end of the container
    containerRef.current.appendChild(sentinel)

    const observer = new IntersectionObserver(
      (entries) => {
        entries.forEach((entry) => {
          if (entry.isIntersecting) {
            loadMoreItems()
          }
        })
      },
      {
        root: null, // Use viewport as root
        rootMargin: `${loadThreshold}px`, // Trigger when within 200px
      },
    )

    observer.observe(sentinel)

    return () => {
      observer.disconnect()
      if (sentinel.parentNode) {
        sentinel.parentNode.removeChild(sentinel)
      }
    }
  }, [loadMoreItems, loadThreshold])

  // Reset visible count when transaction list changes
  React.useEffect(() => {
    setVisibleCount(5) // Reset to initial 5 items
  }, [transactionList.length])

  return (
    <Column
      fullWidth={true}
      fullHeight={true}
      justifyContent='flex-start'
      gap='16px'
      ref={containerRef}
    >
      {visibleItems.map((transaction) => (
        <MemoizedTransactionItem
          key={transaction.id}
          transaction={transaction}
          onClick={handleTransactionClick}
        />
      ))}
    </Column>
  )
})
