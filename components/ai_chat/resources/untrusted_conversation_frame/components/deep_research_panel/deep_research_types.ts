/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as mojom from '../../../common/mojom'

// Deep research event types derived from mojom events
export interface DeepResearchQueriesEvent {
  type: 'queries'
  queries: string[]
}

export interface UrlInfo {
  url: string
  favicon: string
}

export interface DeepResearchThinkingEvent {
  type: 'thinking'
  query: string
  urls_analyzed: number
  urls_selected: string[]
  urls_info: UrlInfo[]
}

export interface DeepResearchAnswerEvent {
  type: 'answer'
  answer: string
  final?: boolean
  citations?: string[]
}

export interface DeepResearchBlindspotsEvent {
  type: 'blindspots'
  blindspots: string[]
}

export interface DeepResearchProgressEvent {
  type: 'progress'
  iteration: number
  elapsed_seconds: number
  urls_analyzed: number
  queries_issued: number
}

export interface DeepResearchAnalyzingEvent {
  type: 'analyzing'
}

export interface ImageResult {
  title: string
  url: string
  thumbnail_url: string
  width?: number
  height?: number
}

export interface DeepResearchImagesEvent {
  type: 'images'
  images: ImageResult[]
}

export interface NewsResult {
  title: string
  url: string
  thumbnail_url: string
  favicon: string
  age?: string
  source?: string
  is_breaking?: boolean
}

export interface DeepResearchNewsEvent {
  type: 'news'
  news: NewsResult[]
}

export interface DiscussionResult {
  title: string
  url: string
  description?: string
  favicon: string
  age?: string
  forum_name?: string
  num_answers?: number
}

export interface DeepResearchDiscussionsEvent {
  type: 'discussions'
  discussions: DiscussionResult[]
}

export type DeepResearchEvent =
  | DeepResearchQueriesEvent
  | DeepResearchThinkingEvent
  | DeepResearchAnswerEvent
  | DeepResearchBlindspotsEvent
  | DeepResearchProgressEvent
  | DeepResearchAnalyzingEvent
  | DeepResearchImagesEvent
  | DeepResearchNewsEvent
  | DeepResearchDiscussionsEvent

export interface DeepResearchMessage {
  events: DeepResearchEvent[]
  finished: boolean
}

// Helper to convert mojom events to DeepResearchEvents
export function convertMojomEvent(
  event: mojom.ConversationEntryEvent
): DeepResearchEvent | null {
  if (event.searchQueriesEvent) {
    return {
      type: 'queries',
      queries: event.searchQueriesEvent.searchQueries
    }
  }

  if (event.thinkingEvent) {
    return {
      type: 'thinking',
      query: event.thinkingEvent.query,
      urls_analyzed: event.thinkingEvent.urlsAnalyzed,
      urls_selected: event.thinkingEvent.urlsSelected,
      urls_info: event.thinkingEvent.urlsInfo.map(info => ({
        url: info.url,
        favicon: info.favicon
      }))
    }
  }

  if (event.completionEvent) {
    return {
      type: 'answer',
      answer: event.completionEvent.completion,
      final: false
    }
  }

  if (event.blindspotsEvent) {
    return {
      type: 'blindspots',
      blindspots: event.blindspotsEvent.blindspots
    }
  }

  if (event.progressEvent) {
    return {
      type: 'progress',
      iteration: event.progressEvent.iteration,
      elapsed_seconds: event.progressEvent.elapsedSeconds,
      urls_analyzed: event.progressEvent.urlsAnalyzed,
      queries_issued: event.progressEvent.queriesIssued
    }
  }

  if (event.searchStatusEvent) {
    return {
      type: 'analyzing'
    }
  }

  if (event.imageResultsEvent) {
    return {
      type: 'images',
      images: event.imageResultsEvent.images.map(img => ({
        title: img.title,
        url: img.imageUrl.url,
        thumbnail_url: img.thumbnailUrl.url,
        width: img.width ?? undefined,
        height: img.height ?? undefined
      }))
    }
  }

  if (event.newsResultsEvent) {
    return {
      type: 'news',
      news: event.newsResultsEvent.news.map(news => ({
        title: news.title,
        url: news.url.url,
        thumbnail_url: news.thumbnailUrl.url,
        favicon: news.faviconUrl.url,
        age: news.age ?? undefined,
        source: news.source ?? undefined,
        is_breaking: news.isBreaking ?? undefined
      }))
    }
  }

  if (event.discussionResultsEvent) {
    return {
      type: 'discussions',
      discussions: event.discussionResultsEvent.discussions.map(disc => ({
        title: disc.title,
        url: disc.url.url,
        description: disc.description ?? undefined,
        favicon: disc.faviconUrl.url,
        age: disc.age ?? undefined,
        forum_name: disc.forumName ?? undefined,
        num_answers: disc.numAnswers ?? undefined
      }))
    }
  }

  return null
}
