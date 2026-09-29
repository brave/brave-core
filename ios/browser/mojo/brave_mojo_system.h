// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_BROWSER_MOJO_BRAVE_MOJO_SYSTEM_H_
#define BRAVE_IOS_BROWSER_MOJO_BRAVE_MOJO_SYSTEM_H_

// A minimal interface to the mojo system API exported from BraveCore.framework.
// It exists so that code outside of BraveCore (such as the SwiftMojo
// framework) can talk to C++ mojo services over message pipes without
// statically linking a second copy of mojo.
//
// Every function other than BraveMojoIsInitialized() may only be called once it
// returns true. Watching is bound to the main (UI) thread, everything else may
// be called from any thread.

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

// An owned mojo handle. 0 is never a valid handle.
typedef uintptr_t BraveMojoHandle;

// A MojoResult.
typedef uint32_t BraveMojoResult NS_TYPED_ENUM;

OBJC_EXPORT BraveMojoResult const BraveMojoResultOk;
OBJC_EXPORT BraveMojoResult const BraveMojoResultCancelled;
OBJC_EXPORT BraveMojoResult const BraveMojoResultUnknown;
OBJC_EXPORT BraveMojoResult const BraveMojoResultInvalidArgument;
OBJC_EXPORT BraveMojoResult const BraveMojoResultDeadlineExceeded;
OBJC_EXPORT BraveMojoResult const BraveMojoResultNotFound;
OBJC_EXPORT BraveMojoResult const BraveMojoResultAlreadyExists;
OBJC_EXPORT BraveMojoResult const BraveMojoResultPermissionDenied;
OBJC_EXPORT BraveMojoResult const BraveMojoResultResourceExhausted;
OBJC_EXPORT BraveMojoResult const BraveMojoResultFailedPrecondition;
OBJC_EXPORT BraveMojoResult const BraveMojoResultAborted;
OBJC_EXPORT BraveMojoResult const BraveMojoResultOutOfRange;
OBJC_EXPORT BraveMojoResult const BraveMojoResultUnimplemented;
OBJC_EXPORT BraveMojoResult const BraveMojoResultInternal;
OBJC_EXPORT BraveMojoResult const BraveMojoResultUnavailable;
OBJC_EXPORT BraveMojoResult const BraveMojoResultDataLoss;
OBJC_EXPORT BraveMojoResult const BraveMojoResultBusy;
OBJC_EXPORT BraveMojoResult const BraveMojoResultShouldWait;

// Identifies an active watch started with BraveMojoWatch(). 0 is never a valid
// watcher.
typedef uint64_t BraveMojoWatcherId;

// Receives a message read with BraveMojoReadMessage(). Ownership of every
// handle in |handles| is transferred to the block.
typedef void (^BraveMojoMessageConsumer)(NSData* bytes,
                                         NSArray<NSNumber*>* handles);

// Called when a watched pipe changes state. |result| is:
//   - BraveMojoResultOk: the pipe is readable or its peer has been closed.
//     Reading returns BraveMojoResultFailedPrecondition once the peer has been
//     closed and every message has been read.
//   - BraveMojoResultFailedPrecondition: the pipe can never become readable
//     again, its peer has been closed and every message has been read.
//   - BraveMojoResultCancelled: the pipe has been closed. The block is not
//     called again.
typedef void (^BraveMojoWatchCallback)(BraveMojoResult result);

// Returns true once mojo has been initialized in this process, after which
// the rest of this API may be used.
OBJC_EXPORT bool BraveMojoIsInitialized(void);

// Creates a message pipe, storing its two ends in |out_handle0| and
// |out_handle1| on success.
OBJC_EXPORT BraveMojoResult
BraveMojoCreateMessagePipe(BraveMojoHandle* out_handle0,
                           BraveMojoHandle* out_handle1);

// Closes |handle|. Closing 0 is a no-op.
OBJC_EXPORT void BraveMojoClose(BraveMojoHandle handle);

// Writes a message with a copy of |bytes| and the handles in |handles| to
// |pipe|.
//
// Ownership of every handle in |handles| is always taken, if the message could
// not be written they are closed.
OBJC_EXPORT BraveMojoResult BraveMojoWriteMessage(BraveMojoHandle pipe,
                                                  NSData* bytes,
                                                  NSArray<NSNumber*>* handles);

// Reads the next message from |pipe| and hands its contents to |consumer|
// before returning. Messages written by in-process C++ bindings are serialized
// first, so |consumer| always receives the full wire format.
//
// Returns BraveMojoResultShouldWait if there are no messages to read, or
// BraveMojoResultFailedPrecondition if there are none and the peer has been
// closed. |consumer| is only called when BraveMojoResultOk is returned.
OBJC_EXPORT BraveMojoResult
BraveMojoReadMessage(BraveMojoHandle pipe,
                     NS_NOESCAPE BraveMojoMessageConsumer consumer);

// Watches |pipe| for becoming readable or its peer being closed, invoking
// |callback| asynchronously on the main thread whenever it does. The watch is
// rearmed after every notification, so |callback| should read all available
// messages. Must be called on the main thread.
//
// Returns 0 if |pipe| cannot be watched. Every watcher returned must
// eventually be passed to BraveMojoCancelWatch(), including after |callback|
// received BraveMojoResultCancelled.
OBJC_EXPORT BraveMojoWatcherId BraveMojoWatch(BraveMojoHandle pipe,
                                              BraveMojoWatchCallback callback);

// Stops |watcher| and releases its callback. |callback| is never invoked after
// this returns. Cancelling 0 or an already cancelled watcher is a no-op. May be
// called from within the watcher's own callback. Must be called on the main
// thread.
OBJC_EXPORT void BraveMojoCancelWatch(BraveMojoWatcherId watcher);

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_BROWSER_MOJO_BRAVE_MOJO_SYSTEM_H_
