/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

const TYPOS_BY_DOMAIN: ReadonlyMap<string, readonly string[]> = new Map([
  [
    'gmail.com',
    [
      'gamail.com',
      'gamil.com',
      'gfmail.com',
      'ghmail.com',
      'gimail.com',
      'gmaail.com',
      'gmai.com',
      'gmaiil.com',
      'gmaik.com',
      'gmail.cim',
      'gmail.clm',
      'gmail.cm',
      'gmail.comm',
      'gmail.con',
      'gmail.coom',
      'gmail.om',
      'gmail.vom',
      'gmailc.om',
      'gmaill.com',
      'gmaol.com',
      'gmaul.com',
      'gmal.com',
      'gmial.com',
      'gmil.com',
      'gmmail.com',
      'gmsil.com',
      'gnail.com',
    ],
  ],
  [
    'hotmail.com',
    [
      'hitmail.com',
      'homail.com',
      'homtail.com',
      'hormail.com',
      'hotail.com',
      'hotmai.com',
      'hotmail.cm',
      'hotmail.con',
      'hotmaill.com',
      'hotmal.com',
      'hotmial.com',
      'hotmil.com',
      'hotmsil.com',
      'hotnail.com',
      'hoymail.com',
    ],
  ],
  ['icloud.com', ['icloud.con', 'icoud.com']],
  ['outlook.com', ['oulook.com', 'outlok.com', 'outlook.con', 'outlouk.com']],
  [
    'yahoo.com',
    [
      'ahoo.com',
      'tahoo.com',
      'yaboo.com',
      'yaho.com',
      'yahho.com',
      'yahoi.com',
      'yahoo.cm',
      'yahoo.cmo',
      'yahoo.con',
      'yahoo.vom',
      'yahool.com',
      'yahooo.com',
      'yajoo.com',
      'yanoo.com',
      'yaoo.com',
      'yayoo.com',
      'yhaoo.com',
      'yhoo.com',
      'yshoo.com',
    ],
  ],
])

const DOMAIN_SUGGESTIONS: ReadonlyMap<string, string> = new Map(
  [...TYPOS_BY_DOMAIN].flatMap(([correct, typos]) =>
    typos.map((typo) => [typo, correct]),
  ),
)

export function maybeSuggestEmailCorrection(email: string): string {
  const at = email.lastIndexOf('@')
  if (at <= 0 || at === email.length - 1) return ''

  const local = email.slice(0, at)
  const domain = email.slice(at + 1).toLowerCase()
  const corrected = DOMAIN_SUGGESTIONS.get(domain)
  return corrected ? `${local}@${corrected}` : ''
}
