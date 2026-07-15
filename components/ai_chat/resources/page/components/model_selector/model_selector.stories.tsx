// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { Meta } from '@storybook/react'
import * as Mojom from '../../../common/mojom'
import { MockContext } from '../../state/mock_context'
import { ModelSelector } from '.'

const MODELS: Mojom.Model[] = [
  {
    key: 'chat-automatic',
    displayName: 'Automatic',
    visionSupport: true,
    audioSupport: false,
    videoSupport: false,
    supportsTools: true,
    supportedCapabilities: [Mojom.ConversationCapability.CHAT],
    capabilities: [],
    isSuggestedModel: true,
    isNearModel: false,
    supportsPrivateInference: false,
    options: {
      leoModelOptions: {
        name: 'automatic',
        displayMaker: '',
        description: '',
        category: Mojom.ModelCategory.CHAT,
        access: Mojom.ModelAccess.BASIC_AND_PREMIUM,
        maxAssociatedContentLength: 10000,
        longConversationWarningCharacterLimit: 9700,
      },
      customModelOptions: undefined,
    },
  },
  {
    key: 'chat-claude-sonnet',
    displayName: 'Claude Sonnet',
    visionSupport: true,
    audioSupport: false,
    videoSupport: false,
    supportsTools: true,
    supportedCapabilities: [Mojom.ConversationCapability.CHAT],
    capabilities: [
      Mojom.ModelCapability.SEARCH,
      Mojom.ModelCapability.VISION,
      Mojom.ModelCapability.TOOLS,
    ],
    isSuggestedModel: true,
    isNearModel: false,
    supportsPrivateInference: false,
    options: {
      leoModelOptions: {
        name: 'claude-sonnet',
        displayMaker: 'Anthropic',
        description: '',
        category: Mojom.ModelCategory.CHAT,
        access: Mojom.ModelAccess.PREMIUM,
        maxAssociatedContentLength: 10000,
        longConversationWarningCharacterLimit: 9700,
      },
      customModelOptions: undefined,
    },
  },
  {
    key: 'chat-gpt-mini',
    displayName: 'GPT Mini',
    visionSupport: true,
    audioSupport: false,
    videoSupport: false,
    supportsTools: false,
    supportedCapabilities: [Mojom.ConversationCapability.CHAT],
    capabilities: [
      Mojom.ModelCapability.FAST,
      Mojom.ModelCapability.VISION,
    ],
    isSuggestedModel: false,
    isNearModel: false,
    supportsPrivateInference: false,
    options: {
      leoModelOptions: {
        name: 'gpt-mini',
        displayMaker: 'OpenAI',
        description: '',
        category: Mojom.ModelCategory.CHAT,
        access: Mojom.ModelAccess.BASIC,
        maxAssociatedContentLength: 10000,
        longConversationWarningCharacterLimit: 9700,
      },
      customModelOptions: undefined,
    },
  },
  {
    key: 'chat-llama',
    displayName: 'Llama',
    visionSupport: false,
    audioSupport: false,
    videoSupport: false,
    supportsTools: false,
    supportedCapabilities: [Mojom.ConversationCapability.CHAT],
    capabilities: [Mojom.ModelCapability.FAST],
    isSuggestedModel: false,
    isNearModel: false,
    supportsPrivateInference: false,
    options: {
      leoModelOptions: {
        name: 'llama',
        displayMaker: 'Meta',
        description: '',
        category: Mojom.ModelCategory.CHAT,
        access: Mojom.ModelAccess.BASIC,
        maxAssociatedContentLength: 10000,
        longConversationWarningCharacterLimit: 9700,
      },
      customModelOptions: undefined,
    },
  },
  {
    key: 'custom-local',
    displayName: 'Local Phi',
    visionSupport: false,
    audioSupport: false,
    videoSupport: false,
    supportsTools: false,
    supportedCapabilities: [Mojom.ConversationCapability.CHAT],
    capabilities: [],
    isSuggestedModel: false,
    isNearModel: false,
    supportsPrivateInference: false,
    options: {
      leoModelOptions: undefined,
      customModelOptions: {
        modelRequestName: 'phi3',
        contextSize: 4096,
        apiKey: '',
        modelSystemPrompt: '',
        longConversationWarningCharacterLimit: 3000,
        maxAssociatedContentLength: 4000,
        endpoint: { url: 'http://localhost:11434' },
      },
    },
  },
]

export const _ModelSelector = {
  render: () => {
    return (
      <div style={{ padding: 24, maxWidth: 400 }}>
        <MockContext
          conversationOverrides={{
            allModels: MODELS,
            currentModel: MODELS[1],
            userDefaultModel: MODELS[1],
          }}
          initialState={{
            serviceState: {
              pinnedModelKeys: ['chat-claude-sonnet', 'chat-gpt-mini'],
            },
          }}
        >
          <ModelSelector />
        </MockContext>
      </div>
    )
  },
}

export default {
  title: 'AI Chat/Model Selector',
  component: ModelSelector,
} as Meta<typeof ModelSelector>
