// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

/**
 * Shared parsing logic for ifdef conditional compilation.
 * Used by both ifdef-loader.ts (webpack loader) and typescript-ifdef.js
 * (custom TypeScript compiler wrapper).
 */

class IfBlock {
  constructor(lineIf, lineEndIf, elifs = [], lineElse = null, innerIfs = []) {
    this.lineIf = lineIf
    this.lineEndIf = lineEndIf
    this.elifs = elifs
    this.lineElse = lineElse
    this.innerIfs = innerIfs
  }

  getIfRange() {
    const to =
      this.elifs.length > 0
        ? this.elifs[0]
        : this.lineElse != null
          ? this.lineElse
          : this.lineEndIf
    return { from: this.lineIf, to }
  }

  getElifRange(index) {
    if (this.elifs.length > index) {
      const from = this.elifs[index]
      const to =
        this.elifs.length > index + 1
          ? this.elifs[index + 1]
          : this.lineElse != null
            ? this.lineElse
            : this.lineEndIf
      return { from, to }
    } else {
      throw new Error(
        `Invalid elif index '${index}', there are only ${this.elifs.length} elifs`
      )
    }
  }

  getElseRange() {
    if (this.lineElse != null) {
      return { from: this.lineElse, to: this.lineEndIf }
    } else {
      throw new Error('Cannot use elseRange when elseIx is null')
    }
  }
}

const IfType = { If: 1, Elif: 2 }

const ifRegex = () => /\/\/ \<(if|elif) expr="(.*)"\>$/g

function matchIf(line, type = IfType.If) {
  const re = ifRegex()
  const match = re.exec(line)
  return (
    match !== null &&
    ((type === IfType.If && match[1] === 'if') ||
      (type === IfType.Elif && match[1] === 'elif'))
  )
}

function parseIfDirective(line) {
  const re = ifRegex()
  const match = re.exec(line)
  if (match) {
    return match[2].trim()
  } else {
    throw new Error(`Could not parse <if expr="...">: '${line}'`)
  }
}

function matchEndif(line) {
  const re = /\/\/ <\/if>/g
  const match = re.exec(line)
  return Boolean(match)
}

function matchElse(line) {
  const re = /\/\/ <else>/g
  const match = re.exec(line)
  return Boolean(match)
}

function findIfBlocks(lines) {
  const blocks = []
  for (let i = 0; i < lines.length; i++) {
    if (matchIf(lines[i])) {
      const ifBlock = parseIfBlock(lines, i)
      blocks.push(ifBlock)
      i = ifBlock.lineEndIf
    }
  }
  return blocks
}

function parseIfBlock(lines, ifBlockStart) {
  let foundElifs = []
  let foundElse = null
  let foundEnd
  let innerIfs = []

  for (let i = ifBlockStart + 1; i < lines.length; i++) {
    const curLine = lines[i]

    const innerIfMatch = matchIf(curLine)
    if (innerIfMatch) {
      const innerIf = parseIfBlock(lines, i)
      innerIfs.push(innerIf)
      i = innerIf.lineEndIf
      continue
    }

    const elifMatch = matchIf(curLine, IfType.Elif)
    if (elifMatch) {
      foundElifs.push(i)
      continue
    }

    const elseMatch = matchElse(curLine)
    if (elseMatch) {
      foundElse = i
      continue
    }

    const endMatch = matchEndif(curLine)
    if (endMatch) {
      foundEnd = i
      break
    }
  }

  if (foundEnd === undefined) {
    throw new Error(`#if without #endif on line ${ifBlockStart + 1}`)
  }
  return new IfBlock(ifBlockStart, foundEnd, foundElifs, foundElse, innerIfs)
}

function evaluate(condition, defs) {
  const code = `return (${condition}) ? true : false;`
  const args = Object.keys(defs)

  let result
  try {
    const f = new Function(...args, code)
    result = f(...args.map((k) => defs[k]))
  } catch (error) {
    throw new Error(
      `error evaluation <if expr="..."> condition(${condition}): ${error}`
    )
  }

  return result
}

function blankCode(lines, start, end) {
  for (let t = start; t <= end; t++) {
    const len = lines[t].length
    const lastChar = lines[t].charAt(len - 1)
    const windowsTermination = lastChar === '\r'
    lines[t] = windowsTermination ? '\r' : ''
  }
}

function applyIf(lines, ifBlock, defs) {
  let includeRange = null

  const ifCond = parseIfDirective(lines[ifBlock.lineIf])
  const ifRes = evaluate(ifCond, defs)

  if (ifRes) {
    includeRange = ifBlock.getIfRange()
  } else {
    for (let elifIx = 0; elifIx < ifBlock.elifs.length; elifIx++) {
      const elifLine = lines[ifBlock.elifs[elifIx]]
      const elifCond = parseIfDirective(elifLine)
      const elifRes = evaluate(elifCond, defs)
      if (elifRes) {
        includeRange = ifBlock.getElifRange(elifIx)
        break
      }
    }

    if (includeRange === null) {
      if (ifBlock.lineElse != null) {
        includeRange = ifBlock.getElseRange()
      }
    }
  }

  if (includeRange !== null) {
    blankCode(lines, ifBlock.lineIf, includeRange.from)
    blankCode(lines, includeRange.to, ifBlock.lineEndIf)
  } else {
    blankCode(lines, ifBlock.lineIf, ifBlock.lineEndIf)
  }

  for (let innerIf of ifBlock.innerIfs) {
    if (
      includeRange != null &&
      innerIf.lineIf >= includeRange.from &&
      innerIf.lineIf <= includeRange.to
    ) {
      applyIf(lines, innerIf, defs)
    }
  }
}

/**
 * Parse and transform source text, applying ifdef conditionals.
 *
 * @param {string} source - The source code to process
 * @param {Object} defs - Object containing build flags/definitions
 * @returns {string} The processed source code
 */
function parse(source, defs) {
  // Early skip check: do not process file when no 'if expr="..."' are contained
  if (!source.includes('<if expr="')) return source

  const lines = source.split(/\r\n|\n|\r/)

  const ifBlocks = findIfBlocks(lines)
  for (let ifBlock of ifBlocks) {
    applyIf(lines, ifBlock, defs)
  }

  return lines.join('\n')
}

module.exports = { parse }
