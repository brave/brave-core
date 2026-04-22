// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// TypeScript service implementations used in place of Mojo/C++ bindings when
// ai_chat_app = true. The stubs below do nothing by default; replace or extend
// them with real backend calls for production use.

import * as Mojom from '../../../common/mojom'
import { makeCloseable } from '$web-common/api'
import createAIChatApi from '../ai_chat_api'
import * as Registry from './conversation_registry'
import * as IDB from './idb_store'

const defaultServiceState: Mojom.ServiceState = {
  hasAcceptedAgreement: true,
  isStoragePrefEnabled: true,
  isStorageNoticeDismissed: true,
  canShowPremiumPrompt: false,
}

export default function bindAppServices() {
  const service = makeCloseable<Mojom.ServiceInterface>({
    getConversations: async () => ({
      conversations: await Registry.getAllConversations(),
    }),
    getActionMenuList: async () => ({ actionList: [] }),
    getSkills: async () => ({ skills: [] }),
    getPremiumStatus: async () => ({
      status: Mojom.PremiumStatus.Inactive,
      info: null,
    }),
    markAgreementAccepted: () => {},
    enableStoragePref: () => {},
    dismissStorageNotice: () => {},
    dismissPremiumPrompt: () => {},
    bindConversation: () => {},
    deleteConversation: (uuid: string) => {
      Registry.notifyConversationDeleted(uuid)
      void IDB.del(Registry.CONV_KEY_PREFIX + uuid)
    },
    renameConversation: () => {},
    conversationExists: async () => ({ exists: true }),
    createSkill: () => {},
    updateSkill: () => {},
    deleteSkill: () => {},
    bindObserver: async () => ({ state: defaultServiceState }),
    bindMetrics: () => {},
  })

  const uiHandler = makeCloseable<Mojom.AIChatUIHandlerInterface>({
    uploadFile: async () => ({ uploadedFiles: [] }),
    processImageFile: async () => ({
      processedFile: {
        filename: '',
        filesize: 0,
        data: [],
        type: Mojom.UploadedFileType.kImage,
        extractedText: undefined,
      },
    }),
    processPdfFile: async () => ({
      processedFile: {
        filename: '',
        filesize: 0,
        data: [],
        type: Mojom.UploadedFileType.kPdf,
        extractedText: undefined,
      },
    }),
    processTextFile: async () => ({
      processedFile: {
        filename: '',
        filesize: 0,
        data: [],
        type: Mojom.UploadedFileType.kText,
        extractedText: undefined,
      },
    }),
    getPluralString: async () => ({ pluralString: '' }),
    setChatUI: async () => ({ isStandalone: true }),
    newConversation: () => {},
    bindRelatedConversation: () => {},
    openURL: () => {},
    closeUI: () => {},
    openModelSupportUrl: () => {},
    openStorageSupportUrl: () => {},
    goPremium: () => {},
    managePremium: () => {},
    refreshPremiumSession: () => {},
    handleVoiceRecognition: () => {},
    openAIChatSettings: () => {},
    openMemorySettings: () => {},
    openConversationFullPage: () => {},
    associateTab: () => {},
    associateUrlContent: () => {},
    disassociateContent: () => {},
    openAIChatAgentProfile: () => {},
    showSoftKeyboard: () => {},
  })

  const bookmarksService = makeCloseable<Mojom.BookmarksPageHandlerInterface>({
    getBookmarks: async () => ({ bookmarks: [] }),
  })

  const historyService = makeCloseable<Mojom.HistoryUIHandlerInterface>({
    getHistory: async () => ({ history: [] }),
  })

  const metricsService = makeCloseable<Mojom.MetricsInterface>({
    onSendingPromptWithNTP: () => {},
    onQuickActionStatusChange: () => {},
    onSendingPromptWithFullPage: () => {},
    recordSkillClick: () => {},
  })

  const aiChat = createAIChatApi(
    service,
    uiHandler,
    bookmarksService,
    historyService,
    metricsService,
  )

  aiChat.api.state.update(defaultServiceState)
  aiChat.api.isStandalone.update(true)

  Registry.subscribeToConversationList((convs) => {
    aiChat.api.getConversations.update(convs)
  })

  return aiChat
}
