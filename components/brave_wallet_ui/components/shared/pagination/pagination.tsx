// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import Icon from '@brave/leo/react/icon'

// styles
import { PaginationButton, PaginationRow } from './pagination.styles'

function createNavPageNumbers(currentPage: number, totalPages: number) {
  const pageOffsets = []
  const canGoBackMultiple = currentPage - 2 >= 1
  const canGoBack = currentPage - 1 >= 1

  const canGoForward = currentPage + 1 <= totalPages
  const canGoForwardMultiple = currentPage + 2 <= totalPages

  // back
  if (!canGoForwardMultiple && currentPage - 4 >= 1) {
    pageOffsets.push(currentPage - 4)
  }
  if (!canGoForward && currentPage - 3 >= 1) {
    pageOffsets.push(currentPage - 3)
  }
  if (canGoBackMultiple) {
    pageOffsets.push(currentPage - 2)
  }
  if (canGoBack) {
    pageOffsets.push(currentPage - 1)
  }

  // current
  pageOffsets.push(currentPage)

  // forward
  if (canGoForward) {
    pageOffsets.push(currentPage + 1)
  }
  if (canGoForwardMultiple) {
    pageOffsets.push(currentPage + 2)
  }
  if (!canGoBackMultiple && currentPage + 3 <= totalPages) {
    pageOffsets.push(currentPage + 3)
  }
  if (!canGoBack && currentPage + 4 <= totalPages) {
    pageOffsets.push(currentPage + 4)
  }

  return pageOffsets
}

export function Pagination({
  onSelectPageNumber,
  currentPageNumber,
  lastPageNumber
}: {
  onSelectPageNumber: (pageNumber: number) => void
  currentPageNumber: number
  lastPageNumber: number
}) {
  // computed
  const canNavigateBack = currentPageNumber > 1
  const canNavigateForward = currentPageNumber < lastPageNumber

  // render
  return (
    <PaginationRow>
      {/* First Page */}
      <PaginationButton
        size='small'
        isDisabled={!canNavigateBack}
        kind='plain-faint'
        onClick={() => onSelectPageNumber(1)}
      >
        <Icon name='carat-first' />
      </PaginationButton>

      {/* Back */}
      <PaginationButton
        size='small'
        isDisabled={!canNavigateBack}
        kind='plain-faint'
        onClick={() => onSelectPageNumber(currentPageNumber - 1)}
      >
        <Icon name='carat-left' />
      </PaginationButton>

      {/* Numbers for navigating pages (up to 5 numbered buttons) */}
      {createNavPageNumbers(currentPageNumber, lastPageNumber).map(
        (newPageNumber) => {
          const isCurrentPage = newPageNumber === currentPageNumber
          return (
            <PaginationButton
              size='small'
              key={newPageNumber}
              kind={isCurrentPage ? 'outline' : 'plain-faint'}
              onClick={() => onSelectPageNumber(newPageNumber)}
            >
              {newPageNumber}
            </PaginationButton>
          )
        }
      )}

      {/* Forward */}
      <PaginationButton
        size='small'
        isDisabled={!canNavigateForward}
        kind='plain-faint'
        onClick={() => onSelectPageNumber(currentPageNumber + 1)}
      >
        <Icon name='carat-right' />
      </PaginationButton>

      {/* Last */}
      <PaginationButton
        size='small'
        isDisabled={!canNavigateForward}
        kind='plain-faint'
        onClick={() => onSelectPageNumber(lastPageNumber)}
      >
        <Icon name='carat-last' />
      </PaginationButton>
    </PaginationRow>
  )
}
