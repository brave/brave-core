/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { getDateString, isString, shortNumberString } from './utils'
import { store } from './data'
import { LEO_DISTILLATION_LEVEL } from '../distillation'
import { expandEntities } from './entities'
import { getDistillationLevel } from './distiller'

const SeenUsersSet = new Set<any>()
const UserSignatures = new Map<string, string>()

/**
 * Generates a formatted signature string for a user,
 * considering different possible representations (name,
 * screen name, or ID).
 */
export function getUserSignature(user: any) {
  if (isString(user) || Number.isInteger(user)) {
    user = store.access('users', user)
  }

  SeenUsersSet.add(user)

  const { name, screen_name: screenName, id_str: idStr } = user ?? {}

  /**
   * If we have already created a signature for this user, we will
   * instead return just their screen name. The verbose signature is
   * already present in the document, so only the shorter version
   * should suffice (and save us some chars).
   */
  if (
    UserSignatures.has(idStr) ||
    getDistillationLevel() === LEO_DISTILLATION_LEVEL.LOW
  ) {
    return isString(screenName) ? `@${screenName}` : 'Anonyous'
  }

  if (isString(screenName)) {
    const signature = isString(name)
      ? `${name} <@${screenName}>`
      : `@${screenName}`

    UserSignatures.set(idStr, signature)

    return signature
  }

  return idStr ? `User #${idStr}` : 'Unknown User'
}

export function distillSeenUsers(level = LEO_DISTILLATION_LEVEL.MEDIUM) {
  switch (level) {
    case LEO_DISTILLATION_LEVEL.LOW:
      return distillLowSeenUsers()
    case LEO_DISTILLATION_LEVEL.MEDIUM:
    case LEO_DISTILLATION_LEVEL.HIGH:
    case LEO_DISTILLATION_LEVEL.FULL:
      return distillMediumHighSeenUsers()
    default:
      return null
  }
}

function distillLowSeenUsers() {
  const output = [] as string[]

  for (const user of SeenUsersSet) {
    if (typeof user.created_at === 'undefined') {
      continue
    }

    const followers = shortNumberString(user.followers_count)
    const memberSince = getDateString(user.created_at)

    const entry = [
      `@${user.screen_name}${isUserVerified(user) ? ' [Verified]' : ''}`,
      ` - Joined: ${memberSince}`,
      ` - Followers: ${followers}`
    ].join('\n')

    output.push(entry)
  }

  return output.join('\n\n')
}

function isUserVerified(user: any): boolean {
  return (
    user.verified ||
    user.is_blue_verified ||
    user.professional ||
    user.verified_type
  )
}

function distillMediumHighSeenUsers() {
  const output = [] as string[]

  for (const user of SeenUsersSet) {
    if (typeof user.created_at === 'undefined') {
      continue
    }

    const memberSince = getDateString(user.created_at)
    const description =
      user.description &&
      expandEntities(user.description, user.entities.description)

    const followersStr = user.followers_count.toLocaleString()
    const followingStr = user.friends_count.toLocaleString()

    const entry = [
      `**${user.name}**`,
      ` - Username: @${user.screen_name}`,
      ` - Verified: ${isUserVerified(user) ? 'Yes' : 'No'}`,
      ` - Followers: ${followersStr} | Following: ${followingStr}`,
      ` - Member Since: ${memberSince}`,
      description && [` - Description: ${description}`]
    ]
      .filter(Boolean)
      .join('\n')

    output.push(entry)
  }

  return output.join('\n\n')
}
