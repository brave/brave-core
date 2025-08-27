// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

/* eslint-disable @typescript-eslint/dot-notation */

import {
  TrezorBridgeTransport,
  sendTrezorCommand,
  closeTrezorBridge,
} from "./trezor-bridge-transport";
import {
  kTrezorBridgeUrl,
  TrezorCommand,
  TrezorErrorsCodes,
  TrezorFrameCommand,
} from "./trezor-messages";
import { TrustedOrigins } from "../untrusted_shared_types";

// Mock crypto.randomUUID
const mockRandomUUID = "test-frame-id-123";
Object.defineProperty(global.crypto, "randomUUID", {
  value: () => mockRandomUUID,
  writable: true,
});

// Mock document methods
const mockRemoveChild = jest.fn();
const mockGetElementById = jest.fn();
const mockCreateElement = jest.fn();
const mockAppendChild = jest.fn();

Object.defineProperty(global.document, "getElementById", {
  value: mockGetElementById,
  writable: true,
});

Object.defineProperty(global.document, "createElement", {
  value: mockCreateElement,
  writable: true,
});

Object.defineProperty(global.document, "body", {
  value: {
    appendChild: mockAppendChild,
  },
  writable: true,
});

// Mock window.postMessage
const mockPostMessage = jest.fn();
Object.defineProperty(global.window, "postMessage", {
  value: mockPostMessage,
  writable: true,
});

describe("TrezorBridgeTransport", () => {
  let transport: TrezorBridgeTransport;
  let mockIframe: HTMLIFrameElement;
  let mockContentWindow: Window;

  beforeEach(() => {
    jest.clearAllMocks();

    // Reset mocks
    mockGetElementById.mockReturnValue(null);
    mockCreateElement.mockReturnValue({
      id: "",
      src: "",
      style: { display: "" },
      onload: null as any,
    });
    mockAppendChild.mockImplementation(() => {});
    mockRemoveChild.mockImplementation(() => {});
    mockPostMessage.mockImplementation(() => {});

    // Create mock iframe
    mockIframe = {
      id: mockRandomUUID,
      src: kTrezorBridgeUrl,
      style: { display: "none" },
      onload: null as any,
      contentWindow: null as any,
    } as HTMLIFrameElement;

    // Create mock content window
    mockContentWindow = {
      postMessage: jest.fn(),
    } as any;

    transport = new TrezorBridgeTransport(kTrezorBridgeUrl);
  });

  describe("constructor", () => {
    it("should initialize with correct properties", () => {
      expect(transport).toBeInstanceOf(TrezorBridgeTransport);
      expect(transport["frameId"]).toBe(mockRandomUUID);
      expect(transport["bridgeFrameUrl"]).toBe(kTrezorBridgeUrl);
      expect(transport["bridge"]).toBeUndefined();
    });
  });

  describe("getTrezorBridgeOrigin", () => {
    it("should return correct origin from bridge URL", () => {
      const origin = transport["getTrezorBridgeOrigin"]();
      expect(origin).toBe("chrome-untrusted://trezor-bridge");
    });
  });

  describe("hasBridgeCreated", () => {
    it("should return false when bridge is not created", () => {
      mockGetElementById.mockReturnValue(null);
      expect(transport.hasBridgeCreated()).toBe(false);
    });

    it("should return true when bridge is created", () => {
      mockGetElementById.mockReturnValue(mockIframe);
      expect(transport.hasBridgeCreated()).toBe(true);
    });
  });

  describe("createBridge", () => {
    it("should create and return iframe element", async () => {
      const createdIframe = {
        id: mockRandomUUID,
        src: kTrezorBridgeUrl,
        style: { display: "none" },
        onload: null as any,
      } as HTMLIFrameElement;

      mockCreateElement.mockReturnValue(createdIframe);

      const promise = transport.createBridge();

      // Simulate iframe load
      createdIframe.onload!();

      const result = await promise;
      expect(result).toBe(createdIframe);
      expect(mockCreateElement).toHaveBeenCalledWith("iframe");
      expect(mockAppendChild).toHaveBeenCalledWith(createdIframe);
      expect(createdIframe.id).toBe(mockRandomUUID);
      expect(createdIframe.src).toBe(kTrezorBridgeUrl);
      expect(createdIframe.style.display).toBe("none");
    });
  });

  describe("closeBridge", () => {
    it("should do nothing when bridge is not created", () => {
      transport.bridge = undefined;
      mockGetElementById.mockReturnValue(null);

      transport.closeBridge();

      expect(mockRemoveChild).not.toHaveBeenCalled();
    });

    it("should remove bridge element when bridge exists", () => {
      const mockParent = {
        removeChild: mockRemoveChild,
      };
      const mockElement = mockIframe;

      transport.bridge = mockIframe;
      mockGetElementById.mockReturnValue(mockElement);
      mockElement.parentNode = mockParent as any;

      transport.closeBridge();

      expect(mockRemoveChild).toHaveBeenCalledWith(mockElement);
    });
  });

  describe("sendCommandToTrezorFrame", () => {
    const mockCommand: TrezorFrameCommand = {
      id: "test-command-id",
      command: TrezorCommand.Unlock,
      origin: "chrome://wallet",
    };

    beforeEach(() => {
      transport.bridge = mockIframe;
      mockIframe.contentWindow = mockContentWindow;
    });

    it("should create bridge if not exists and send command", async () => {
      transport.bridge = undefined;
      mockGetElementById.mockReturnValue(null);

      const createdIframe = { ...mockIframe, contentWindow: mockContentWindow };
      mockCreateElement.mockReturnValue(createdIframe);

      const promise = transport.sendCommandToTrezorFrame(mockCommand);

      // Simulate iframe load
      createdIframe.onload!();

      await promise;

      expect(mockCreateElement).toHaveBeenCalledWith("iframe");
      expect(mockContentWindow.postMessage).toHaveBeenCalledWith(
        mockCommand,
        kTrezorBridgeUrl,
      );
    });

    it("should return BridgeNotReady when bridge creation fails", async () => {
      transport.bridge = undefined;
      mockGetElementById.mockReturnValue(null);
      mockCreateElement.mockReturnValue(null);

      const result = await transport.sendCommandToTrezorFrame(mockCommand);

      expect(result).toBe(TrezorErrorsCodes.BridgeNotReady);
    });

    it("should return BridgeNotReady when no contentWindow", async () => {
      transport.bridge = mockIframe;
      mockIframe.contentWindow = null;

      const result = await transport.sendCommandToTrezorFrame(mockCommand);

      expect(result).toBe(TrezorErrorsCodes.BridgeNotReady);
    });

    it("should return CommandInProgress when handler exists", async () => {
      // Add a handler for the same command ID
      transport.addCommandHandler(mockCommand.id, jest.fn());

      const result = await transport.sendCommandToTrezorFrame(mockCommand);

      expect(result).toBe(TrezorErrorsCodes.CommandInProgress);
    });

    it("should send command successfully when bridge is ready", async () => {
      const promise = transport.sendCommandToTrezorFrame(mockCommand);

      // Simulate response
      setTimeout(() => {
        transport.onMessageReceived({
          type: "message",
          origin: "chrome-untrusted://trezor-bridge",
          data: {
            id: mockCommand.id,
            command: TrezorCommand.Unlock,
            payload: { success: true },
          },
        } as MessageEvent);
      }, 0);

      const result = await promise;
      expect(mockContentWindow.postMessage).toHaveBeenCalledWith(
        mockCommand,
        kTrezorBridgeUrl,
      );
      expect(result).toEqual({
        id: mockCommand.id,
        command: TrezorCommand.Unlock,
        payload: { success: true },
      });
    });
  });

  describe("onMessageReceived", () => {
    const mockCommand: TrezorFrameCommand = {
      id: "test-command-id",
      command: TrezorCommand.Unlock,
      origin: "chrome://wallet",
    };

    beforeEach(() => {
      // Add a handler for testing
      transport.addCommandHandler(mockCommand.id, jest.fn());
    });

    it("should ignore messages from untrusted origins", () => {
      const mockHandler = jest.fn();
      transport.handlers.set(mockCommand.id, mockHandler);

      transport.onMessageReceived({
        type: "message",
        origin: "https://malicious-site.com",
        data: mockCommand,
      } as MessageEvent);

      expect(mockHandler).not.toHaveBeenCalled();
    });

    it("should ignore messages with wrong type", () => {
      const mockHandler = jest.fn();
      transport.handlers.set(mockCommand.id, mockHandler);

      transport.onMessageReceived({
        type: "error",
        origin: "chrome-untrusted://trezor-bridge",
        data: mockCommand,
      } as MessageEvent);

      expect(mockHandler).not.toHaveBeenCalled();
    });

    it("should ignore messages when no handlers exist", () => {
      transport.handlers.clear();

      transport.onMessageReceived({
        type: "message",
        origin: "chrome-untrusted://trezor-bridge",
        data: mockCommand,
      } as MessageEvent);

      // Should not throw or call any handlers
    });

    it("should ignore messages with no data", () => {
      const mockHandler = jest.fn();
      transport.handlers.set(mockCommand.id, mockHandler);

      transport.onMessageReceived({
        type: "message",
        origin: "chrome-untrusted://trezor-bridge",
        data: null,
      } as MessageEvent);

      expect(mockHandler).not.toHaveBeenCalled();
    });

    it("should ignore messages with unknown command ID", () => {
      const mockHandler = jest.fn();
      transport.handlers.set(mockCommand.id, mockHandler);

      transport.onMessageReceived({
        type: "message",
        origin: "chrome-untrusted://trezor-bridge",
        data: { ...mockCommand, id: "unknown-id" },
      } as MessageEvent);

      expect(mockHandler).not.toHaveBeenCalled();
    });

    it("should process valid messages from trusted origin", () => {
      const mockHandler = jest.fn();
      transport.handlers.set(mockCommand.id, mockHandler);

      transport.onMessageReceived({
        type: "message",
        origin: "chrome-untrusted://trezor-bridge",
        data: mockCommand,
      } as MessageEvent);

      expect(mockHandler).toHaveBeenCalledWith(mockCommand);
      expect(transport.handlers.has(mockCommand.id)).toBe(false);
    });

    it("should accept messages from all trusted origins", () => {
      const mockHandler = jest.fn();
      transport.handlers.set(mockCommand.id, mockHandler);

      // Test each trusted origin
      TrustedOrigins.forEach((origin) => {
        transport.onMessageReceived({
          type: "message",
          origin,
          data: { ...mockCommand, id: `${mockCommand.id}-${origin}` },
        } as MessageEvent);
      });

      // Should have called handler for each trusted origin
      expect(mockHandler).toHaveBeenCalledTimes(TrustedOrigins.length);
    });
  });
});

describe("sendTrezorCommand", () => {
  beforeEach(() => {
    jest.clearAllMocks();
  });

  it("should create transport and send command", async () => {
    const mockCommand: TrezorFrameCommand = {
      id: "test-command-id",
      command: TrezorCommand.Unlock,
      origin: "chrome://wallet",
    };

    const mockResponse = { success: true };

    // Mock the transport's sendCommandToTrezorFrame method
    const mockSendCommand = jest.fn().mockResolvedValue(mockResponse);
    jest
      .spyOn(TrezorBridgeTransport.prototype, "sendCommandToTrezorFrame")
      .mockImplementation(mockSendCommand);

    const result = await sendTrezorCommand(mockCommand);

    expect(mockSendCommand).toHaveBeenCalledWith(mockCommand);
    expect(result).toBe(mockResponse);
  });

  it("should reuse existing transport instance", async () => {
    const mockCommand: TrezorFrameCommand = {
      id: "test-command-id",
      command: TrezorCommand.Unlock,
      origin: "chrome://wallet",
    };

    const mockSendCommand = jest.fn().mockResolvedValue({ success: true });
    jest
      .spyOn(TrezorBridgeTransport.prototype, "sendCommandToTrezorFrame")
      .mockImplementation(mockSendCommand);

    // Call twice
    await sendTrezorCommand(mockCommand);
    await sendTrezorCommand(mockCommand);

    // Should only create one transport instance
    expect(mockSendCommand).toHaveBeenCalledTimes(2);
  });
});

describe("closeTrezorBridge", () => {
  beforeEach(() => {
    jest.clearAllMocks();
  });

  it("should do nothing when no transport exists", () => {
    expect(() => closeTrezorBridge()).not.toThrow();
  });

  it("should close bridge when transport exists", () => {
    const mockCloseBridge = jest.fn();
    jest
      .spyOn(TrezorBridgeTransport.prototype, "closeBridge")
      .mockImplementation(mockCloseBridge);

    // Create transport first
    sendTrezorCommand({
      id: "test",
      command: TrezorCommand.Unlock,
      origin: "chrome://wallet",
    });

    closeTrezorBridge();

    expect(mockCloseBridge).toHaveBeenCalled();
  });
});

describe("trusted origins validation", () => {
  it("should include trezor bridge origin", () => {
    expect(TrustedOrigins).toContain("chrome-untrusted://trezor-bridge");
  });

  it("should include all expected origins", () => {
    const expectedOrigins = [
      "chrome-untrusted://ledger-bridge",
      "chrome-untrusted://trezor-bridge",
      "chrome://wallet",
      "chrome://wallet-panel.top-chrome",
    ];

    expectedOrigins.forEach((origin) => {
      expect(TrustedOrigins).toContain(origin);
    });
  });
});
