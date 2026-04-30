/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

const DOMAIN_SUGGESTIONS: ReadonlyMap<string, string> = new Map([
  ['gmail.con', 'gmail.com'],
  ['gmai.com', 'gmail.com'],
  ['gamil.com', 'gmail.com'],
  ['gnail.com', 'gmail.com'],
  ['gmal.com', 'gmail.com'],
  ['gmil.com', 'gmail.com'],
  ['gmial.com', 'gmail.com'],
  ['gmaill.com', 'gmail.com'],
  ['gmail.cm', 'gmail.com'],
  ['gmaul.com', 'gmail.com'],
  ['gmaiil.com', 'gmail.com'],
  ['gmaol.com', 'gmail.com'],
  ['gamail.com', 'gmail.com'],
  ['gmail.om', 'gmail.com'],
  ['gmsil.com', 'gmail.com'],
  ['gmailc.om', 'gmail.com'],
  ['gmail.cim', 'gmail.com'],
  ['gmail.vom', 'gmail.com'],
  ['gmail.comm', 'gmail.com'],
  ['gmail.clm', 'gmail.com'],
  ['gmaail.com', 'gmail.com'],
  ['gmaik.com', 'gmail.com'],
  ['gmmail.com', 'gmail.com'],
  ['gmail.coom', 'gmail.com'],
  ['ghmail.com', 'gmail.com'],
  ['gfmail.com', 'gmail.com'],
  ['gimail.com', 'gmail.com'],

  ['yahoo.con', 'yahoo.com'],
  ['yaho.com', 'yahoo.com'],
  ['tahoo.com', 'yahoo.com'],
  ['yshoo.com', 'yahoo.com'],
  ['yaoo.com', 'yahoo.com'],
  ['yhaoo.com', 'yahoo.com'],
  ['yhoo.com', 'yahoo.com'],
  ['yahoo.cm', 'yahoo.com'],
  ['yahooo.com', 'yahoo.com'],
  ['yahho.com', 'yahoo.com'],
  ['yaboo.com', 'yahoo.com'],
  ['yanoo.com', 'yahoo.com'],
  ['yahool.com', 'yahoo.com'],
  ['ahoo.com', 'yahoo.com'],
  ['yahoi.com', 'yahoo.com'],
  ['yajoo.com', 'yahoo.com'],
  ['yahoo.vom', 'yahoo.com'],
  ['yahoo.cmo', 'yahoo.com'],
  ['yayoo.com', 'yahoo.com'],

  ['hotmail.con', 'hotmail.com'],
  ['hitmail.com', 'hotmail.com'],
  ['hotmai.com', 'hotmail.com'],
  ['hormail.com', 'hotmail.com'],
  ['hotnail.com', 'hotmail.com'],
  ['hotmsil.com', 'hotmail.com'],
  ['hotmal.com', 'hotmail.com'],
  ['hotmial.com', 'hotmail.com'],
  ['homail.com', 'hotmail.com'],
  ['hotmil.com', 'hotmail.com'],
  ['hotmaill.com', 'hotmail.com'],
  ['homtail.com', 'hotmail.com'],
  ['hotmail.cm', 'hotmail.com'],
  ['hoymail.com', 'hotmail.com'],
  ['hotail.com', 'hotmail.com'],

  ['icloud.con', 'icloud.com'],
  ['icoud.com', 'icloud.com'],

  ['oulook.com', 'outlook.com'],
  ['outlok.com', 'outlook.com'],
  ['outlook.con', 'outlook.com'],
  ['outlouk.com', 'outlook.com'],
])

export function maybeSuggestEmailCorrection(email: string): string | null {
  const at = email.lastIndexOf('@')
  if (at <= 0 || at === email.length - 1) return null

  const local = email.slice(0, at)
  const domain = email.slice(at + 1).toLowerCase()
  const corrected = DOMAIN_SUGGESTIONS.get(domain)
  return corrected ? `${local}@${corrected}` : null
}
