// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

let gIdToRegexMap = new Map<string, string>()

export function discardRegex (id: string) {
  chrome.send('brave_adblock_internals.discardRegex', [id])
}

export function saveRegexTexts (list: RegexDebugEntry[]) {
  for (const entry of list) {
    if (entry.regex !== '') {
      gIdToRegexMap.set(entry.id, entry.regex)
    }
  }
}

export function discardRegexs (list: RegexDebugEntry[]) {
  for (const entry of list) {
    if (entry.regex !== '') {
      discardRegex(entry.id)
    }
  }
}

export class RegexDebugEntry {
  id: string = ''
  regex: string = ''
  unused_sec: number = 0
  usage_count: number = 0
}

interface Props {
  regex: RegexDebugEntry
}

export class Regex extends React.Component<Props, {}> {
  render () {
    const regex = this.props.regex
    let regexText = regex.regex
    const discarded = regexText === ''
    if (discarded) {
      regexText = '[UNKNOWN]'
      const savedText = gIdToRegexMap.get(regex.id)
      if (savedText) {
        regexText = savedText
      }
    }

    const className = discarded ? 'inactive-entry' : 'active-entry'
    let unused = discarded ? '[DISCARDED]' : regex.unused_sec.toString()

    return (
      <tr>
        <td>{regex.id}</td>
        <td>{regex.usage_count}</td>
        <td>{unused} </td>
        <td><div className={className}>{regexText}</div></td>
        <td><input type="button" value="Discard"
          onClick={() => { discardRegex(regex.id) }} /></td>
      </tr>)
  }
}
