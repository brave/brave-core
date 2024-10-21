/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { getXProps, store } from './data'
import { getUserSignature } from './user'
import {
  indentLines,
  wrapLines,
  isString,
  decodeHTMLSpecialChars,
  getDateString
} from './utils'
import { distillCard } from './cards'
import { distillPostMediaEntities } from './entities'
import { getDistillationLevel } from './distiller'
import { LEO_DISTILLATION_LEVEL } from '../distillation'

/**
 * Generates a list of formatted metrics (like Likes,
 * Quotes, Replies) for a given tweet, considering the
 * plurality of values.
 * TODO (Sampson): Come back and define some types, such
 * as the `post` parameter.
 */
function getPostMetrics(post: any) {
  const metrics = [
    { key: 'favorite', singular: 'Like', plural: 'Likes' },
    { key: 'quote', singular: 'Quote', plural: 'Quotes' },
    { key: 'reply', singular: 'Reply', plural: 'Replies' },
    { key: 'retweet', singular: 'Repost', plural: 'Reposts' },
    { key: 'bookmark', singular: 'Bookmark', plural: 'Bookmarks' }
  ]

  /**
   * X is now combining these two metrics into a single value.
   * To keep the output consistent, we'll do the same here.
   */
  let processedQuotesAndReposts = false

  return metrics
    .map(({ key, singular, plural }) => {
      const propKey = `${key}_count`
      const propValue = post[propKey]

      if (propValue === undefined) return null

      /**
       * We won't include zero values for low distillation
       */
      if (getDistillationLevel() === LEO_DISTILLATION_LEVEL.LOW) {
        if (parseInt(propValue) === 0) return null
      }

      if (['quote', 'retweet'].includes(key) && !processedQuotesAndReposts) {
        processedQuotesAndReposts = true

        const combinedValue =
          (post.quote_content ?? 0) + (post.retweet_count ?? 0)
        const label = combinedValue === 1 ? 'Quote/Repost' : 'Quotes/Reposts'

        return `${combinedValue.toLocaleString()} ${label}`
      } else {
        const label = propValue === 1 ? singular : plural
        return `${propValue.toLocaleString()} ${label}`
      }
    })
    .filter(Boolean)
}

/**
 * Extracts post data from a DOM element and distills
 * it into a formatted output.
 */
export function distillPostElement(element: HTMLElement) {
  const postXProps = getXProps(element.parentElement)
  const postID = postXProps?.children?.props?.entry?.content?.id
  const postData = postID && store.access('tweets', postID)

  return postData ? distillPostData(postData) : element.innerText
}

/**
 * Constructs the text output for a given post, handling
 * reposts, headers, and post body content.
 */
function distillPostData(post: any, level: number = 0): string | null {
  if (post.retweeted_status) {
    return distillRepostPost(post)
  }

  const header = distillPostHeader(post)
  const body = buildPostBody(post, level)

  return [header, body && indentLines(`\n${body}`)].filter(Boolean).join('\n')
}

/**
 * Builds the main content of a post, including
 * handling quoted content and text wrapping.
 */
function buildPostBody(post: any, level: number) {
  const text = getPostText(post)
  const quoted = post.quoted_status ? buildPostQuotedBody(post, level) : null
  const attachments = post.extended_entities?.media
    ? distillPostMediaEntities(post)
    : null
  const card = post.card ? distillCard(post) : null

  return [
    text ? wrapLines(text) : null,
    card ? [`\n${indentLines(wrapLines(card), ' | ')}`] : null,
    quoted ? [`\n${indentLines(quoted, ' > ')}`] : null,
    attachments ? [`\nAttachments:\n${attachments}`] : null
  ]
    .flat()
    .filter(Boolean)
    .join('\n')
}

/**
 * Creates the header of a post, including the author
 * information, action line, and any relevant metrics.
 */
function distillPostHeader(post: any) {
  const author = store.access('users', post.user)
  const signature = getUserSignature(author)
  const metrics = getPostMetrics(post)

  return [
    `From: ${signature}`,
    buildPostActionLine(post),
    metrics.length > 0 ? `Metrics: ${metrics.join(' | ')}` : null
  ]
    .filter(Boolean)
    .join('\n')
}

/**
 * Retrieves and decodes the main text of a post, handling
 * different fields where the text might be located.
 */
function getPostText(post: any) {
  let response = isString(post.note_tweet?.text)
    ? post.note_tweet.text
    : post.full_text

  // Remove leading references
  if (getDistillationLevel() < LEO_DISTILLATION_LEVEL.HIGH) {
    response = response.replace(/^(\@[a-zA-Z0-9_]+\s+)+/, '')
  }

  return isString(response) ? decodeHTMLSpecialChars(response) : null
}

/**
 * Formats a reposted tweet, incorporating the original
 * tweet’s content and indicating who reposted it.
 */
function distillRepostPost(post: any): string | null {
  const repostingUser = store.access('users', post.user)
  const repostingPost = store.access('tweets', post.retweeted_status)
  if (repostingUser && repostingPost) {
    const distilled = distillPostData(repostingPost)
    if (distilled) {
      const [fromUser, ...rest] = distilled.split('\n')
      const repostedBy = `Reposted by ${getUserSignature(repostingUser)}`
      return [`${fromUser} (${repostedBy})`, ...rest].join('\n')
    }
  }
  return null
}

/**
 * Constructs an action line for a post, such as indicating
 * a reply or when the post was created.
 */
function buildPostActionLine(post: any) {
  const date = getDateString(post.created_at)
  if (post.in_reply_to_user_id_str && post.in_reply_to_screen_name) {
    const user = store.access('users', post.in_reply_to_user_id_str)
    const signature =
      getUserSignature(user) ?? `@${post.in_reply_to_screen_name}`
    return `Replied to ${signature}: ${date}`
  }
  return `Posted: ${date}`
}

/**
 * Handles the content for quoted posts, including truncation
 * logic and URL continuation if the maximum depth is reached.
 */
function buildPostQuotedBody(post: any, level: number = 0): string | null {
  const quote = store.access('tweets', post.quoted_status)
  const distilled =
    level < 3 && quote ? distillPostData(quote, level + 1) : null
  return distilled || `Thread continues at ${getExpandedPostURL(post)}`
}

/**
 * Retrieves the expanded URL for a post, either from the
 * provided permalink or by constructing it using user
 * information.
 */
function getExpandedPostURL(post: any) {
  const url = post.quoted_status_permalink?.expandedUrl
  if (url) return url
  const { screen_name: screenName } = store.access('users', post.user) ?? {}
  return isString(screenName)
    ? `https://x.com/${screenName}/status/${post.id_str}`
    : null
}
