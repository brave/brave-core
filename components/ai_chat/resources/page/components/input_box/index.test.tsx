// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

/* eslint-disable jest/no-conditional-expect, import/first */

import * as React from 'react'
import { render, screen, fireEvent, waitFor, act } from '@testing-library/react'
import { clearAllDataForTesting } from '$web-common/api'
import { ContentType, UploadedFileType, TaskState } from '../../../common/mojom'
import { MockContext } from '../../state/mock_context'
import InputBox, { InputBoxProps } from '.'

// Mock the convertFileToUploadedFile function
jest.mock('../../utils/file_utils', () => ({
  convertFileToUploadedFile: jest.fn(),
}))

import { convertFileToUploadedFile } from '../../utils/file_utils'
const mockConvertFileToUploadedFile =
  convertFileToUploadedFile as jest.MockedFunction<
    typeof convertFileToUploadedFile
  >

// Mock URL.createObjectURL for tests that include image files
// This is needed because AttachmentUploadItems calls URL.createObjectURL to create blob URLs for images
Object.defineProperty(URL, 'createObjectURL', {
  writable: true,
  value: jest.fn(() => 'mock-object-url'),
})

const testContext: InputBoxProps['context'] = {
  isMobile: false,
  hasAcceptedAgreement: true,
  getPluralString: () => Promise.resolve(''),
  attachImages: jest.fn(),
  isAIChatAgentProfileFeatureEnabled: false,
  isAIChatAgentProfile: false,
  openAIChatAgentProfile: () => {},
  associatedContentInfo: [],
  inputText: [''],
  isGenerating: false,
  pendingMessageFiles: [],
  conversationHistory: [],
  unassociatedTabs: [],
  setAttachmentsDialog: () => {},
  uploadFile: jest.fn(),
  getScreenshots: jest.fn(),
  setInputText: () => {},
  submitInputTextToAPI: jest.fn(),
  selectedActionType: undefined,
  resetSelectedActionType: () => {},
  isCharLimitApproaching: false,
  isCharLimitExceeded: false,
  inputTextCharCountDisplay: '',
  isToolsMenuOpen: false,
  setIsToolsMenuOpen: () => {},
  toolUseTaskState: TaskState.kNone,
  shouldDisableUserInput: false,
  handleVoiceRecognition: () => {},
  handleStopGenerating: () => Promise.resolve(),
  removeFile: () => {},
  isUploadingFiles: false,
  disassociateContent: () => {},
  associateDefaultContent: undefined,
  pauseTask: () => {},
  resumeTask: () => {},
  stopTask: () => {},
  handleSkillClick: () => {},
  selectedSkill: undefined,
  processImageFile: jest.fn(),
  skills: [],
  openURL: () => {},
}

// Render InputBox and flush async state updates from usePromise.
async function renderInputBox(
  ...args: Parameters<typeof render>
): Promise<ReturnType<typeof render>> {
  let result: ReturnType<typeof render>
  await act(async () => {
    result = render(...args)
  })
  return result!
}

describe('input box', () => {
  beforeEach(() => {
    clearAllDataForTesting()
  })

  it('associated content is rendered in input box when not associated with a turn', async () => {
    const { container } = await renderInputBox(
      <MockContext>
        <InputBox
          context={{
            ...testContext,
            associatedContentInfo: [
              {
                contentId: 1,
                contentType: ContentType.PageContent,
                contentUsedPercentage: 0.5,
                title: 'Associated Content',
                url: { url: 'https://example.com' },
                uuid: '1234',
                conversationTurnUuid: undefined,
              },
            ],
          }}
          conversationStarted={false}
        />
      </MockContext>,
    )

    expect(
      screen.getByText('Associated Content', { selector: '.title' }),
    ).toBeInTheDocument()
    expect(
      container.querySelector('img[src*="//favicon2"]'),
    ).toBeInTheDocument()
  })

  it('associated content is not rendered in input box when there is no associated content', async () => {
    const { container } = await renderInputBox(
      <MockContext>
        <InputBox
          context={{
            ...testContext,
            associatedContentInfo: [],
          }}
          conversationStarted={false}
        />
      </MockContext>,
    )

    expect(screen.queryByText('Associated Content')).not.toBeInTheDocument()
    expect(
      container.querySelector('img[src*="//favicon2"]'),
    ).not.toBeInTheDocument()
  })

  it('associated content is not rendered in input box after being associated with a turn', async () => {
    const { container } = await renderInputBox(
      <MockContext>
        <InputBox
          context={{
            ...testContext,
            associatedContentInfo: [
              {
                contentId: 1,
                contentType: ContentType.PageContent,
                contentUsedPercentage: 0.5,
                title: 'Associated Content',
                url: { url: 'https://example.com' },
                uuid: '1234',
                conversationTurnUuid: 'turn-1',
              },
            ],
          }}
          conversationStarted
        />
      </MockContext>,
    )

    expect(screen.queryByText('Associated Content')).not.toBeInTheDocument()
    expect(
      container.querySelector('img[src*="//favicon2"]'),
    ).not.toBeInTheDocument()
  })

  it('send button is disabled when the input text is empty', async () => {
    const { container } = await renderInputBox(
      <MockContext>
        <InputBox
          context={{
            ...testContext,
            inputText: [''],
          }}
          conversationStarted={false}
        />
      </MockContext>,
    )

    const sendButton = container.querySelector('.sendButtonDisabled')
    expect(sendButton).toBeInTheDocument()
    expect(sendButton).toHaveClass('sendButtonDisabled')
  })

  it('send button is enabled when the input text is not empty', async () => {
    const { container } = await renderInputBox(
      <MockContext>
        <InputBox
          context={{
            ...testContext,
            inputText: ['test'],
          }}
          conversationStarted={false}
        />
      </MockContext>,
    )

    const sendButton = container.querySelector('.button')
    expect(sendButton).toBeInTheDocument()
    expect(sendButton).not.toHaveClass('sendButtonDisabled')
  })

  it('streaming button is shown while generating', async () => {
    const { container } = await renderInputBox(
      <MockContext>
        <InputBox
          context={{ ...testContext, isGenerating: true }}
          conversationStarted={false}
        />
      </MockContext>,
    )

    const streamingButton = container.querySelector('.streamingButton')
    expect(streamingButton).toBeInTheDocument()
    expect(streamingButton).toHaveClass('streamingButton')
  })

  type ContentAgentParams = {
    isAgentProfileFeatureEnabled: boolean
    isAgentProfile: boolean
    isConversationStarted: boolean
  }

  const contentAgentParams: ContentAgentParams[] = [
    {
      isAgentProfileFeatureEnabled: false,
      isAgentProfile: false,
      isConversationStarted: false,
    },
    {
      isAgentProfileFeatureEnabled: false,
      isAgentProfile: false,
      isConversationStarted: true,
    },
    {
      isAgentProfileFeatureEnabled: false,
      isAgentProfile: true,
      isConversationStarted: false,
    },
    {
      isAgentProfileFeatureEnabled: false,
      isAgentProfile: true,
      isConversationStarted: true,
    },
    {
      isAgentProfileFeatureEnabled: true,
      isAgentProfile: false,
      isConversationStarted: false,
    },
    {
      isAgentProfileFeatureEnabled: true,
      isAgentProfile: false,
      isConversationStarted: true,
    },
    {
      isAgentProfileFeatureEnabled: true,
      isAgentProfile: true,
      isConversationStarted: false,
    },
    {
      isAgentProfileFeatureEnabled: true,
      isAgentProfile: true,
      isConversationStarted: true,
    },
  ]

  it.each(contentAgentParams)(
    'Content Agent button is shown only if the feature is enabled',
    async (params: ContentAgentParams) => {
      await renderInputBox(
        <MockContext>
          <InputBox
            context={{
              ...testContext,
              isAIChatAgentProfileFeatureEnabled:
                params.isAgentProfileFeatureEnabled,
              isAIChatAgentProfile: params.isAgentProfile,
            }}
            conversationStarted={params.isConversationStarted}
          />
        </MockContext>,
      )

      const contentAgentLaunchButton = screen.queryByTitle(
        S.CHAT_UI_AI_BROWSING_TOGGLE_BUTTON_LABEL,
      )
      const contentAgentTooltip = screen.queryByTestId('agent-profile-tooltip')

      if (params.isAgentProfileFeatureEnabled && params.isAgentProfile) {
        expect(contentAgentLaunchButton).not.toBeInTheDocument()
        expect(contentAgentTooltip).toBeInTheDocument()
      } else if (
        params.isAgentProfileFeatureEnabled
        && !params.isAgentProfile
      ) {
        expect(contentAgentLaunchButton).toBeInTheDocument()
        expect(contentAgentTooltip).not.toBeInTheDocument()
      } else {
        expect(contentAgentLaunchButton).not.toBeInTheDocument()
        expect(contentAgentTooltip).not.toBeInTheDocument()
      }
    },
  )

  it('documents show up in attachment wrapper', async () => {
    const { container } = await renderInputBox(
      <MockContext>
        <InputBox
          context={{
            ...testContext,
            pendingMessageFiles: [
              {
                filename: 'test.pdf',
                data: [],
                type: UploadedFileType.kPdf,
                filesize: 1024,
              },
            ],
          }}
          conversationStarted={false}
        />
      </MockContext>,
    )

    expect(screen.getByText('test.pdf')).toBeInTheDocument()
    const attachmentWrapper = container.querySelector('.attachmentWrapper')
    expect(attachmentWrapper).toBeInTheDocument()
  })

  it('attachments are shown if only documents are available', async () => {
    const { container } = await renderInputBox(
      <MockContext>
        <InputBox
          context={{
            ...testContext,
            pendingMessageFiles: [
              {
                filename: 'document1.pdf',
                data: [],
                type: UploadedFileType.kPdf,
                filesize: 2048,
              },
              {
                filename: 'document2.pdf',
                data: [],
                type: UploadedFileType.kPdf,
                filesize: 1536,
              },
            ],
          }}
          conversationStarted={false}
        />
      </MockContext>,
    )

    const attachmentWrapper = container.querySelector('.attachmentWrapper')
    expect(attachmentWrapper).toBeInTheDocument()
    expect(screen.getByText('document1.pdf')).toBeInTheDocument()
    expect(screen.getByText('document2.pdf')).toBeInTheDocument()
    // Check that both documents are rendered as attachment items
    const attachmentItems = container.querySelectorAll('.itemWrapper')
    expect(attachmentItems.length).toBeGreaterThanOrEqual(2)
  })

  it('combinations of associated content, images and documents show up', async () => {
    const { container } = await renderInputBox(
      <MockContext>
        <InputBox
          context={{
            ...testContext,
            associatedContentInfo: [
              {
                conversationTurnUuid: undefined,
                contentId: 1,
                contentType: ContentType.PageContent,
                contentUsedPercentage: 0.5,
                title: 'Page Content',
                url: { url: 'https://example.com' },
                uuid: '1234',
              },
            ],
            pendingMessageFiles: [
              {
                filename: 'image.jpg',
                data: [],
                type: UploadedFileType.kImage,
                filesize: 1024,
              },
              {
                filename: 'document.pdf',
                data: [],
                type: UploadedFileType.kPdf,
                filesize: 2048,
              },
            ],
          }}
          conversationStarted={false}
        />
      </MockContext>,
    )

    const attachmentWrapper = container.querySelector('.attachmentWrapper')
    expect(attachmentWrapper).toBeInTheDocument()

    // Associated content
    expect(screen.getByText('Page Content')).toBeInTheDocument()
    expect(
      container.querySelector('img[src*="//favicon2"]'),
    ).toBeInTheDocument()

    // Image file
    expect(screen.getByText('image.jpg')).toBeInTheDocument()
    expect(container.querySelector('img.image')).toBeInTheDocument()

    // Document file
    expect(screen.getByText('document.pdf')).toBeInTheDocument()

    // Check that we have multiple attachment items (page content + files)
    const attachmentItems = container.querySelectorAll('.itemWrapper')
    expect(attachmentItems.length).toBeGreaterThanOrEqual(3)
  })

  describe('paste event handling', () => {
    const createMockFile = (name: string, type: string, size = 1024) => {
      const file = new File(['mock content'], name, { type })
      Object.defineProperty(file, 'size', { value: size })
      return file
    }

    beforeEach(() => {
      jest.clearAllMocks()

      mockConvertFileToUploadedFile.mockImplementation((file: File) => {
        return Promise.resolve({
          filename: file.name,
          filesize: file.size,
          data: Array.from(new Uint8Array(8)), // Mock data array
          type: UploadedFileType.kImage,
        })
      })
    })

    it('filters image files and calls attachImages on paste', async () => {
      const mockAttachImages = jest.fn()
      const { container } = await renderInputBox(
        <MockContext>
          <InputBox
            context={{
              ...testContext,
              attachImages: mockAttachImages,
            }}
            conversationStarted={false}
          />
        </MockContext>,
      )

      const textarea = container.querySelector('[data-editor]')!
      const imageFile = createMockFile('test.png', 'image/png')
      const textFile = createMockFile('test.txt', 'text/plain')

      await act(async () => {
        fireEvent.paste(textarea, {
          clipboardData: {
            files: [imageFile, textFile],
            items: [imageFile, textFile].map((file) => ({
              kind: 'file' as const,
              type: file.type,
              getAsFile: () => file,
            })),
            types: ['Files'],
            getData: jest.fn(),
            setData: jest.fn(),
          },
          preventDefault: jest.fn(),
        })
      })

      await waitFor(() => {
        expect(mockAttachImages).toHaveBeenCalledWith([
          expect.objectContaining({
            filename: 'test.png',
            filesize: 1024,
            data: expect.any(Array),
            type: UploadedFileType.kImage,
          }),
        ])
      })
    })
  })

  it(
    'Content Agent warning is shown if the conversation has not started'
      + ' and isAIChatAgentProfile is true',
    async () => {
      const { container } = await renderInputBox(
        <MockContext>
          <InputBox
            context={{
              ...testContext,
              isAIChatAgentProfileFeatureEnabled: true,
              isAIChatAgentProfile: true,
            }}
            conversationStarted={false}
          />
        </MockContext>,
      )
      expect(
        container.querySelector('.contentAgentWarning'),
      ).toBeInTheDocument()
    },
  )

  it(
    'Content Agent warning is not shown after the conversation has started'
      + ' and isAIChatAgentProfile is true',
    async () => {
      const { container } = await renderInputBox(
        <MockContext>
          <InputBox
            context={{
              ...testContext,
              isAIChatAgentProfileFeatureEnabled: true,
              isAIChatAgentProfile: true,
            }}
            conversationStarted={true}
          />
        </MockContext>,
      )
      expect(
        container.querySelector('.contentAgentWarning'),
      ).not.toBeInTheDocument()
    },
  )

  it(
    'Content Agent warning is not shown if isAIChatAgentProfile'
      + 'is not true',
    async () => {
      const { container } = await renderInputBox(
        <MockContext>
          <InputBox
            context={{
              ...testContext,
            }}
            conversationStarted={true}
          />
        </MockContext>,
      )
      expect(
        container.querySelector('.contentAgentWarning'),
      ).not.toBeInTheDocument()
    },
  )
})
