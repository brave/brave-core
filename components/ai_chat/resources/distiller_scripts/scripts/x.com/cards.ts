/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { store } from './data'
import { getUserSignature } from './user'

export function distillCard(post: any) {
  const card = store.access('cards', post.card)
  if (card) {
    const title = card.binding_values.title
    const cardURL = card.binding_values.card_url
    const unifiedCard = card.binding_values.unified_card
    const host = card.binding_values.domain
    const desc = card.binding_values.description

    if (card.name.endsWith('audiospace')) {
      /* Not Implemented */
      return null
    } else if (card.name === 'unified_card' && unifiedCard) {
      return distillUnifiedCard(unifiedCard)
    }

    return [
      'Resource:',
      title ? ` - Title: "${title.string_value}"` : null,
      host && cardURL
        ? ` - Address: [${host.string_value}](${cardURL.string_value})`
        : null,
      desc ? ` - Description: "${desc.string_value}"` : null
    ]
      .filter(Boolean)
      .join('\n')
  }
  return null
}

function distillUnifiedCard(cardString: any) {
  const card = JSON.parse(cardString.string_value)
  return distillUnifiedCardComponents(card)
}

function distillUnifiedCardComponents(card: any) {
  return card.components
    .map((key: string) => {
      const component = card.component_objects[key]

      switch (component.type) {
        case 'media':
          return distillUnifiedCardMedia(component, card)
        case 'grok_share':
          return distillUnifiedCardGrokShare(component, card)
        case 'community_details':
          return distillUnifiedCardCommunityDetails(component, card)
        default:
          return null
      }
    })
    .filter(Boolean)
    .join('\n')
}

function distillUnifiedCardMedia(component: any, card: any) {
  const {
    data: { id }
  } = component
  const {
    type,
    original_info: { width, height }
  } = card.media_entities[id]

  if (type !== 'photo') {
    /**
     * TODO (Sampson): Handle other media types
     */
    console.warn(`Unhandled media type: ${type}`, card.media_entities[id])
  }
  return ['Media:', ` - Type: ${type}`, ` - Dimensions: ${width}x${height}`]
    .filter(Boolean)
    .join('\n')
}

type ConvoEntry = {
  sender: 'USER' | 'AGENT'
  grokMode: 'NORMAL' | 'FUN'
  message: string
}

function distillUnifiedCardGrokShare(component: any, card: any) {
  const conversationPreview = component.data.conversation_preview
  const destination = component.data.destination
  const grokUser = component.data.grok_user
  const profileUser = component.data.profile_user
  const urlData = card.destination_objects[destination].data.url_data
  const grokSig = getUserSignature(grokUser)
  const userSig = getUserSignature(profileUser)

  const conversation = conversationPreview.map((entry: ConvoEntry) => {
    const message = entry.message
    const signature = entry.sender === 'USER' ? userSig : grokSig
    return `${signature}: ${message}\n`
  })

  conversation.push(`URL: ${urlData.url}`)

  return conversation.filter(Boolean).join('\n')
}

function distillUnifiedCardCommunityDetails(component: any, card: any) {
  const {
    data: {
      destination,
      member_count: memberCount,
      members_facepile: membersFacepile,
      name
    }
  } = component

  const {
    data: { url_data: urlData }
  } = card.destination_objects[destination]

  const facepileSignatures = membersFacepile
    .map((id: string) => {
      return getUserSignature(card('users')[id]) || null
    })
    .filter(Boolean)

  const count = memberCount - facepileSignatures.length

  return [
    'Community:',
    ` - Name: ${name}`,
    ` - Members: ${facepileSignatures.join(', ')}, and ${count} others`,
    ` - URL: ${urlData.url}`
  ]
    .filter(Boolean)
    .join('\n')
}
