// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { RegexDebugEntry, Regex } from './regex'

export interface SourceInfo {
  title: string
  homepage: string
  network_filter_count: number
  cosmetic_filter_count: number
  parse_error_count: number
  invalid_lines: string
}

export class EngineDebugInfo {
  compiled_regex_count: number = 0
  regex_data: RegexDebugEntry[] = []
  flatbuffer_size: number = 0
  source_info: SourceInfo[] = []
}

interface Props {
  caption: string
  info: EngineDebugInfo
}

export const EngineRegexList = (props: Props) => {
  const items = props.info.regex_data.map((d, index) => (
    <Regex
      key={index}
      regex={d}
    />
  ))

  return (
    <div>
      <table>
        <caption>
          <h2>{props.caption} Regexes</h2>
        </caption>
        <tbody>
          <tr>
            <th>ID</th>
            <th>Usage count</th>
            <th>Unused (sec)</th>
            <th>Regex</th>
            <th>Actions</th>
          </tr>
          {items}
        </tbody>
      </table>
    </div>
  )
}
