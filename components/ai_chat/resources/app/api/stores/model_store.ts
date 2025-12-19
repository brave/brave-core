// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as Mojom from '../../../common/mojom'
import { getLocale } from '$web-common/locale'

// Model name constants (matching C++ constants.h)
const kClaudeHaikuModelName = 'claude-3-haiku'
const kClaudeSonnetModelName = 'claude-3-sonnet'

// Model key constants
const kClaudeHaikuModelKey = 'chat-claude-haiku'
const kClaudeSonnetModelKey = 'chat-claude-sonnet'

// Default model key
const kDefaultModelKey = 'chat-automatic'

// Create Leo models matching model_service.cc GetLeoModels()
function createLeoModels(): Mojom.Model[] {
  // For freemium, most models are BASIC_AND_PREMIUM
  const kFreemiumAccess = Mojom.ModelAccess.BASIC_AND_PREMIUM

  const models: Mojom.Model[] = []

  // Automatic model
  models.push({
    key: 'chat-automatic',
    displayName: 'Automatic',
    options: {
      leoModelOptions: {
        name: 'automatic',
        displayMaker: 'Brave',
        category: Mojom.ModelCategory.CHAT,
        access: kFreemiumAccess,
        maxAssociatedContentLength: 180000,
        longConversationWarningCharacterLimit: 320000,
      },
      customModelOptions: undefined,
    },
    visionSupport: true,
    supportsTools: true,
    isSuggestedModel: true,
    isNearModel: false,
  })

  // DeepSeek R1
  models.push({
    key: 'chat-deepseek-r1',
    displayName: 'DeepSeek R1',
    options: {
      leoModelOptions: {
        name: 'bedrock-us.deepseek-r1-v1:0',
        displayMaker: 'DeepSeek',
        category: Mojom.ModelCategory.CHAT,
        access: Mojom.ModelAccess.PREMIUM,
        maxAssociatedContentLength: 64000,
        longConversationWarningCharacterLimit: 64000,
      },
      customModelOptions: undefined,
    },
    visionSupport: false,
    supportsTools: false,
    isSuggestedModel: false,
    isNearModel: false,
  })

  // Claude Haiku
  models.push({
    key: kClaudeHaikuModelKey,
    displayName: 'Claude Haiku',
    options: {
      leoModelOptions: {
        name: kClaudeHaikuModelName,
        displayMaker: 'Anthropic',
        category: Mojom.ModelCategory.CHAT,
        access: kFreemiumAccess,
        maxAssociatedContentLength: 180000,
        longConversationWarningCharacterLimit: 320000,
      },
      customModelOptions: undefined,
    },
    visionSupport: true,
    supportsTools: true,
    isSuggestedModel: false,
    isNearModel: false,
  })

  // Claude Sonnet
  models.push({
    key: kClaudeSonnetModelKey,
    displayName: 'Claude Sonnet',
    options: {
      leoModelOptions: {
        name: kClaudeSonnetModelName,
        displayMaker: 'Anthropic',
        category: Mojom.ModelCategory.CHAT,
        access: Mojom.ModelAccess.PREMIUM,
        maxAssociatedContentLength: 180000,
        longConversationWarningCharacterLimit: 320000,
      },
      customModelOptions: undefined,
    },
    visionSupport: true,
    supportsTools: true,
    isSuggestedModel: true,
    isNearModel: false,
  })

  // Llama 3.1 8B
  models.push({
    key: 'chat-basic',
    displayName: 'Llama 3.1 8B',
    options: {
      leoModelOptions: {
        name: 'llama-3-8b-instruct',
        displayMaker: 'Meta',
        category: Mojom.ModelCategory.CHAT,
        access: kFreemiumAccess,
        maxAssociatedContentLength: 64000,
        longConversationWarningCharacterLimit: 9700,
      },
      customModelOptions: undefined,
    },
    visionSupport: false,
    supportsTools: false,
    isSuggestedModel: false,
    isNearModel: false,
  })

  // Qwen 14B
  models.push({
    key: 'chat-qwen',
    displayName: 'Qwen 14B',
    options: {
      leoModelOptions: {
        name: 'qwen-14b-instruct',
        displayMaker: 'Alibaba Cloud',
        category: Mojom.ModelCategory.CHAT,
        access: kFreemiumAccess,
        maxAssociatedContentLength: 64000,
        longConversationWarningCharacterLimit: 9700,
      },
      customModelOptions: undefined,
    },
    visionSupport: false,
    supportsTools: false,
    isSuggestedModel: true,
    isNearModel: false,
  })

  // Gemma 12B
  models.push({
    key: 'chat-gemma',
    displayName: 'Gemma 12B',
    options: {
      leoModelOptions: {
        name: 'gemma-3-12b-it',
        displayMaker: 'Google DeepMind',
        category: Mojom.ModelCategory.CHAT,
        access: kFreemiumAccess,
        maxAssociatedContentLength: 64000,
        longConversationWarningCharacterLimit: 9700,
      },
      customModelOptions: undefined,
    },
    visionSupport: true,
    supportsTools: false,
    isSuggestedModel: false,
    isNearModel: false,
  })

  // Llama 4 Scout
  models.push({
    key: 'chat-llama-4-scout',
    displayName: 'Llama 4 Scout',
    options: {
      leoModelOptions: {
        name: 'llama-4-scout',
        displayMaker: 'Meta',
        category: Mojom.ModelCategory.CHAT,
        access: kFreemiumAccess,
        maxAssociatedContentLength: 64000,
        longConversationWarningCharacterLimit: 9700,
      },
      customModelOptions: undefined,
    },
    visionSupport: true,
    supportsTools: false,
    isSuggestedModel: false,
    isNearModel: false,
  })

  // Llama 4 Maverick
  models.push({
    key: 'chat-llama-4-maverick',
    displayName: 'Llama 4 Maverick',
    options: {
      leoModelOptions: {
        name: 'llama-4-maverick',
        displayMaker: 'Meta',
        category: Mojom.ModelCategory.CHAT,
        access: kFreemiumAccess,
        maxAssociatedContentLength: 64000,
        longConversationWarningCharacterLimit: 9700,
      },
      customModelOptions: undefined,
    },
    visionSupport: true,
    supportsTools: false,
    isSuggestedModel: false,
    isNearModel: false,
  })

  // GPT OSS 20B
  models.push({
    key: 'chat-gpt-oss-20b',
    displayName: 'GPT OSS 20B',
    options: {
      leoModelOptions: {
        name: 'gpt-oss-20b',
        displayMaker: 'OpenAI',
        category: Mojom.ModelCategory.CHAT,
        access: kFreemiumAccess,
        maxAssociatedContentLength: 64000,
        longConversationWarningCharacterLimit: 9700,
      },
      customModelOptions: undefined,
    },
    visionSupport: false,
    supportsTools: false,
    isSuggestedModel: false,
    isNearModel: false,
  })

  // GPT OSS 120B
  models.push({
    key: 'chat-gpt-oss-120b',
    displayName: 'GPT OSS 120B',
    options: {
      leoModelOptions: {
        name: 'gpt-oss-120b',
        displayMaker: 'OpenAI',
        category: Mojom.ModelCategory.CHAT,
        access: Mojom.ModelAccess.PREMIUM,
        maxAssociatedContentLength: 64000,
        longConversationWarningCharacterLimit: 9700,
      },
      customModelOptions: undefined,
    },
    visionSupport: false,
    supportsTools: false,
    isSuggestedModel: false,
    isNearModel: false,
  })

  // Mistral Large
  models.push({
    key: 'chat-mistral-large',
    displayName: 'Mistral Large',
    options: {
      leoModelOptions: {
        name: 'mistral-large',
        displayMaker: 'Mistral',
        category: Mojom.ModelCategory.CHAT,
        access: Mojom.ModelAccess.PREMIUM,
        maxAssociatedContentLength: 64000,
        longConversationWarningCharacterLimit: 9700,
      },
      customModelOptions: undefined,
    },
    visionSupport: true,
    supportsTools: false,
    isSuggestedModel: false,
    isNearModel: false,
  })

  // Pixtral Large
  models.push({
    key: 'chat-pixtral-large',
    displayName: 'Pixtral Large',
    options: {
      leoModelOptions: {
        name: 'pixtral-large',
        displayMaker: 'Mistral',
        category: Mojom.ModelCategory.CHAT,
        access: Mojom.ModelAccess.PREMIUM,
        maxAssociatedContentLength: 64000,
        longConversationWarningCharacterLimit: 9700,
      },
      customModelOptions: undefined,
    },
    visionSupport: true,
    supportsTools: false,
    isSuggestedModel: false,
    isNearModel: false,
  })

  // Qwen 3 235B
  models.push({
    key: 'chat-qwen-3-235b',
    displayName: 'Qwen 3 235B',
    options: {
      leoModelOptions: {
        name: 'qwen-3-235b',
        displayMaker: 'Alibaba Cloud',
        category: Mojom.ModelCategory.CHAT,
        access: Mojom.ModelAccess.PREMIUM,
        maxAssociatedContentLength: 64000,
        longConversationWarningCharacterLimit: 9700,
      },
      customModelOptions: undefined,
    },
    visionSupport: false,
    supportsTools: false,
    isSuggestedModel: false,
    isNearModel: false,
  })

  // Deepseek V3.1
  models.push({
    key: 'chat-deepseek-v3-1',
    displayName: 'Deepseek V3.1',
    options: {
      leoModelOptions: {
        name: 'deepseek-v3-1',
        displayMaker: 'Deepseek',
        category: Mojom.ModelCategory.CHAT,
        access: Mojom.ModelAccess.PREMIUM,
        maxAssociatedContentLength: 64000,
        longConversationWarningCharacterLimit: 9700,
      },
      customModelOptions: undefined,
    },
    visionSupport: false,
    supportsTools: false,
    isSuggestedModel: false,
    isNearModel: false,
  })

  // Qwen 3 Coder 480B
  models.push({
    key: 'chat-qwen-3-coder-480b',
    displayName: 'Qwen 3 Coder 480B',
    options: {
      leoModelOptions: {
        name: 'qwen-3-coder-480b',
        displayMaker: 'Alibaba Cloud',
        category: Mojom.ModelCategory.CHAT,
        access: Mojom.ModelAccess.PREMIUM,
        maxAssociatedContentLength: 64000,
        longConversationWarningCharacterLimit: 9700,
      },
      customModelOptions: undefined,
    },
    visionSupport: false,
    supportsTools: false,
    isSuggestedModel: false,
    isNearModel: false,
  })

  // Claude Opus
  models.push({
    key: 'chat-claude-opus',
    displayName: 'Claude Opus',
    options: {
      leoModelOptions: {
        name: 'claude-opus',
        displayMaker: 'Anthropic',
        category: Mojom.ModelCategory.CHAT,
        access: Mojom.ModelAccess.PREMIUM,
        maxAssociatedContentLength: 180000,
        longConversationWarningCharacterLimit: 320000,
      },
      customModelOptions: undefined,
    },
    visionSupport: true,
    supportsTools: false,
    isSuggestedModel: false,
    isNearModel: false,
  })

  return models
}

// Map of model keys to subtitle string IDs
const modelSubtitleKeys: Record<string, keyof typeof S> = {
  'chat-basic': S.CHAT_UI_CHAT_BASIC_SUBTITLE,
  'chat-claude-haiku': S.CHAT_UI_CHAT_CLAUDE_HAIKU_SUBTITLE,
  'chat-claude-sonnet': S.CHAT_UI_CHAT_CLAUDE_SONNET_SUBTITLE,
  'chat-qwen': S.CHAT_UI_CHAT_QWEN_SUBTITLE,
  'chat-deepseek-r1': S.CHAT_UI_CHAT_DEEPSEEK_R1_SUBTITLE,
  'chat-automatic': S.CHAT_UI_CHAT_AUTOMATIC_SUBTITLE,
  'chat-gemma': S.CHAT_UI_CHAT_GEMMA_SUBTITLE,
  'chat-llama-4-scout': S.CHAT_UI_CHAT_LLAMA_4_SCOUT_SUBTITLE,
  'chat-llama-4-maverick': S.CHAT_UI_CHAT_LLAMA_4_MAVERICK_SUBTITLE,
  'chat-gpt-oss-20b': S.CHAT_UI_CHAT_GPT_OSS_20B_SUBTITLE,
  'chat-gpt-oss-120b': S.CHAT_UI_CHAT_GPT_OSS_120B_SUBTITLE,
  'chat-mistral-large': S.CHAT_UI_CHAT_MISTRAL_LARGE_SUBTITLE,
  'chat-pixtral-large': S.CHAT_UI_CHAT_PIXTRAL_LARGE_SUBTITLE,
  'chat-qwen-3-235b': S.CHAT_UI_CHAT_QWEN_3_235B_SUBTITLE,
  'chat-deepseek-v3-1': S.CHAT_UI_CHAT_DEEPSEEK_V3_1_SUBTITLE,
  'chat-qwen-3-coder-480b': S.CHAT_UI_CHAT_QWEN_3_CODER_480B_SUBTITLE,
  'chat-claude-opus': S.CHAT_UI_CHAT_CLAUDE_OPUS_SUBTITLE,
}

/**
 * ModelStore holds models and provides key/name lookups.
 * This is shared across AIChatService, ConversationHandler, and V1Engine.
 */
export default class ModelStore {
  private models: Mojom.Model[]
  private defaultModelKey: string

  // Maps for fast lookups
  private modelsByKey: Map<string, Mojom.Model> = new Map()
  private keyByName: Map<string, string> = new Map()
  private nameByKey: Map<string, string> = new Map()

  constructor(models?: Mojom.Model[], defaultModelKey?: string) {
    this.models = models ?? createLeoModels()
    this.defaultModelKey = defaultModelKey ?? kDefaultModelKey
    this.rebuildMaps()
  }

  private rebuildMaps(): void {
    this.modelsByKey.clear()
    this.keyByName.clear()
    this.nameByKey.clear()

    for (const model of this.models) {
      this.modelsByKey.set(model.key, model)

      const modelName = model.options.leoModelOptions?.name
      if (modelName) {
        this.keyByName.set(modelName, model.key)
        this.nameByKey.set(model.key, modelName)
      }
    }
  }

  // Get all models
  getModels(): Mojom.Model[] {
    return [...this.models]
  }

  // Get models with subtitles (for model selection UI)
  getModelsWithSubtitles(): Mojom.ModelWithSubtitle[] {
    return this.models.map((model) => {
      const subtitleKey = modelSubtitleKeys[model.key]
      return {
        model: { ...model },
        subtitle: subtitleKey ? getLocale(subtitleKey) : '',
      }
    })
  }

  // Get default model key
  getDefaultModelKey(): string {
    return this.defaultModelKey
  }

  // Set default model key
  setDefaultModelKey(key: string): void {
    if (this.modelsByKey.has(key)) {
      this.defaultModelKey = key
    }
  }

  // Get a model by key
  getModelByKey(key: string): Mojom.Model | undefined {
    return this.modelsByKey.get(key)
  }

  // Get model key by name (e.g., 'claude-3-sonnet' -> 'chat-claude-sonnet')
  getModelKeyByName = (modelName: string): string | undefined => {
    return this.keyByName.get(modelName)
  }

  // Get model name by key (e.g., 'chat-claude-sonnet' -> 'claude-3-sonnet')
  getModelNameByKey = (modelKey: string): string | undefined => {
    return this.nameByKey.get(modelKey)
  }

  // Get max associated content length for a specific model
  getMaxAssociatedContentLength(modelKey?: string): number {
    const key = modelKey ?? this.defaultModelKey
    const model = this.modelsByKey.get(key)
    return model?.options.leoModelOptions?.maxAssociatedContentLength ?? 20000
  }

  // Set models (e.g., after fetching from server)
  setModels(models: Mojom.Model[]): void {
    this.models = models
    this.rebuildMaps()
  }
}
