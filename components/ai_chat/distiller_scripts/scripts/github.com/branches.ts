/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

export function distillBranches() {
  const tables = document.querySelectorAll('table')
  const output = [...tables].map(distillBranchesTable)
  return output.join('\n')
}

function distillBranchesTable(table: HTMLTableElement) {
  const pageURL = document.location.toString()
  const pageTitle = document.title
  const tableTitle = getBranchesTableTitle(table)

  // Extract headers from the first row
  const headers = [...table.rows[0].cells].map((cell) => {
    return cell.innerText.trim()
  })

  // Define custom processing for specific headers
  const cellProcessors = {
    'updated': (cell: HTMLTableCellElement) => ({
      author: cell.querySelector('a')?.getAttribute('href'),
      date: cell.querySelector('relative-time')?.getAttribute('title')
    }),
    'pull request': (cell: HTMLTableCellElement) => {
      const link = cell.querySelector('a')
      const href = link?.getAttribute('href')
      // We can infer the state from the presence and type of icon
      const state = link
        ?.querySelector<HTMLElement>('[data-testid$="-pull-request-icon"]')
        ?.dataset.testid?.split('-')[0]
      if (href && state) {
        return { href, state }
      }
      return null
    },
    'action menu': null // Skip this cell
  } as Record<string, any>

  const data = [] as Array<Record<string, string>>

  // Iterate over each row, starting from the second one
  for (let i = 1; i < table.rows.length; i++) {
    const row = table.rows[i]
    const rowData = {} as Record<string, string>

    // Iterate over each cell in the row
    for (let j = 0; j < row.cells.length; j++) {
      const cell = row.cells[j]
      const label = headers[j]
      const loweredLabel = label.toLowerCase()

      // Check if there's a custom processor for the cell
      const processor = cellProcessors[loweredLabel]
      const trimmedText = cell.innerText.trim()

      if (typeof processor === 'function') {
        const result = processor(cell)
        if (result !== null) {
          rowData[label] = result
        }
      } else if (processor === null || trimmedText === '') {
        continue
      } else {
        rowData[label] = trimmedText
      }
    }

    data.push(rowData)
  }

  return [
    `Current time: ${new Date().toLocaleString()}`,
    `Current URL: ${pageURL}`,
    `# Page Title: ${pageTitle}`,
    `## Data Title: ${tableTitle}`,
    ...rDistillObject(data)
  ].join('\n')
}

function rDistillObject(obj: any) {
  /**
   * Recursively distill an object into a list (maintaining hierarchy):
   * - Prop A: Value of Prop A
   *   - Subprop A: Value of Subprop A
   *   - Subprop B: Value of Subprop B
   * - Prop B: Value of Prop B
   * - Prop C: Value of Prop C
   */
  const output = [] as string[]

  for (const [k, v] of Object.entries(obj)) {
    if (typeof v === 'object') {
      output.push(`- ${k}:`)
      output.push(...rDistillObject(v).map((line) => `  ${line}`))
    } else if (typeof v === 'string') {
      output.push(`- ${k.replace('\n', '/')}: ${v.replace('\n', '/')}`)
    }
  }

  return output
}

function getBranchesTableTitle(table: HTMLTableElement) {
  const holderSelector = ':has(.TableTitle), :has(.TabNav-item.selected'
  const titleSelector = '.TableTitle, .TabNav-item.selected'

  const titleHolder = table.closest(holderSelector)
  const tableTitle = titleHolder?.querySelector(titleSelector)?.textContent

  if (tableTitle) {
    return tableTitle.trim()
  }

  return table.getAttribute('aria-labelledby')
}
