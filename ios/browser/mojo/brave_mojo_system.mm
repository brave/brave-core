// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/mojo/brave_mojo_system.h"

#include <pthread.h>

#include <memory>
#include <type_traits>
#include <utility>
#include <vector>

#include "base/apple/foundation_util.h"
#include "base/check.h"
#include "base/containers/flat_map.h"
#include "base/containers/span.h"
#include "base/functional/bind.h"
#include "base/location.h"
#include "base/no_destructor.h"
#include "base/task/sequenced_task_runner.h"
#include "mojo/public/c/system/thunks.h"
#include "mojo/public/c/system/types.h"
#include "mojo/public/cpp/bindings/message.h"
#include "mojo/public/cpp/system/handle.h"
#include "mojo/public/cpp/system/handle_signals_state.h"
#include "mojo/public/cpp/system/message.h"
#include "mojo/public/cpp/system/message_pipe.h"
#include "mojo/public/cpp/system/simple_watcher.h"

static_assert(std::is_same_v<BraveMojoHandle, MojoHandle>);
static_assert(std::is_same_v<BraveMojoResult, MojoResult>);
static_assert(MOJO_HANDLE_INVALID == 0);

BraveMojoResult const BraveMojoResultOk = MOJO_RESULT_OK;
BraveMojoResult const BraveMojoResultCancelled = MOJO_RESULT_CANCELLED;
BraveMojoResult const BraveMojoResultUnknown = MOJO_RESULT_UNKNOWN;
BraveMojoResult const BraveMojoResultInvalidArgument =
    MOJO_RESULT_INVALID_ARGUMENT;
BraveMojoResult const BraveMojoResultDeadlineExceeded =
    MOJO_RESULT_DEADLINE_EXCEEDED;
BraveMojoResult const BraveMojoResultNotFound = MOJO_RESULT_NOT_FOUND;
BraveMojoResult const BraveMojoResultAlreadyExists = MOJO_RESULT_ALREADY_EXISTS;
BraveMojoResult const BraveMojoResultPermissionDenied =
    MOJO_RESULT_PERMISSION_DENIED;
BraveMojoResult const BraveMojoResultResourceExhausted =
    MOJO_RESULT_RESOURCE_EXHAUSTED;
BraveMojoResult const BraveMojoResultFailedPrecondition =
    MOJO_RESULT_FAILED_PRECONDITION;
BraveMojoResult const BraveMojoResultAborted = MOJO_RESULT_ABORTED;
BraveMojoResult const BraveMojoResultOutOfRange = MOJO_RESULT_OUT_OF_RANGE;
BraveMojoResult const BraveMojoResultUnimplemented = MOJO_RESULT_UNIMPLEMENTED;
BraveMojoResult const BraveMojoResultInternal = MOJO_RESULT_INTERNAL;
BraveMojoResult const BraveMojoResultUnavailable = MOJO_RESULT_UNAVAILABLE;
BraveMojoResult const BraveMojoResultDataLoss = MOJO_RESULT_DATA_LOSS;
BraveMojoResult const BraveMojoResultBusy = MOJO_RESULT_BUSY;
BraveMojoResult const BraveMojoResultShouldWait = MOJO_RESULT_SHOULD_WAIT;

namespace {

// Watchers are bound to the main thread's task runner, the UI thread.
void CheckOnMainThread() {
  CHECK(pthread_main_np());
  CHECK(base::SequencedTaskRunner::HasCurrentDefault());
}

// Owns every active BraveMojoWatch() watcher. Only used on the main thread.
class WatcherRegistry {
 public:
  static WatcherRegistry& Get() {
    static base::NoDestructor<WatcherRegistry> registry;
    return *registry;
  }

  BraveMojoWatcherId Add(std::unique_ptr<mojo::SimpleWatcher> watcher) {
    BraveMojoWatcherId id = next_id_++;
    watchers_.emplace(id, std::move(watcher));
    return id;
  }

  // Returns the removed watcher, so that a watcher cancelled from within its
  // own callback is destroyed after the map has been updated.
  std::unique_ptr<mojo::SimpleWatcher> Remove(BraveMojoWatcherId id) {
    auto it = watchers_.find(id);
    if (it == watchers_.end()) {
      return nullptr;
    }
    std::unique_ptr<mojo::SimpleWatcher> watcher = std::move(it->second);
    watchers_.erase(it);
    return watcher;
  }

 private:
  base::flat_map<BraveMojoWatcherId, std::unique_ptr<mojo::SimpleWatcher>>
      watchers_;
  BraveMojoWatcherId next_id_ = 1;
};

}  // namespace

bool BraveMojoIsInitialized(void) {
  // The thunks table is populated by mojo::core::Init(), before that every
  // mojo system call is fatal.
  return MojoEmbedderGetSystemThunks2()->size > 0;
}

BraveMojoResult BraveMojoCreateMessagePipe(BraveMojoHandle* out_handle0,
                                           BraveMojoHandle* out_handle1) {
  CHECK(out_handle0);
  CHECK(out_handle1);
  mojo::ScopedMessagePipeHandle handle0;
  mojo::ScopedMessagePipeHandle handle1;
  MojoResult result = mojo::CreateMessagePipe(nullptr, &handle0, &handle1);
  if (result == MOJO_RESULT_OK) {
    *out_handle0 = handle0.release().value();
    *out_handle1 = handle1.release().value();
  }
  return result;
}

void BraveMojoClose(BraveMojoHandle handle) {
  // Closes |handle| on destruction, invalid handles are ignored.
  mojo::ScopedHandle scoped_handle{mojo::Handle(handle)};
}

BraveMojoResult BraveMojoWriteMessage(BraveMojoHandle pipe,
                                      NSData* bytes,
                                      NSArray<NSNumber*>* handles) {
  std::vector<mojo::ScopedHandle> scoped_handles;
  scoped_handles.reserve(handles.count);
  bool has_invalid_handle = false;
  for (NSNumber* handle in handles) {
    scoped_handles.emplace_back(mojo::Handle(handle.unsignedLongValue));
    has_invalid_handle |= !scoped_handles.back().is_valid();
  }
  // The remaining handles are closed when |scoped_handles| goes out of scope.
  if (has_invalid_handle) {
    return MOJO_RESULT_INVALID_ARGUMENT;
  }

  mojo::Message message(base::apple::NSDataToSpan(bytes), scoped_handles);
  // The message, including its handles, is destroyed on failure.
  return mojo::WriteMessageNew(mojo::MessagePipeHandle(pipe),
                               message.TakeMojoMessage(),
                               MOJO_WRITE_MESSAGE_FLAG_NONE);
}

BraveMojoResult BraveMojoReadMessage(BraveMojoHandle pipe,
                                     NS_NOESCAPE BraveMojoMessageConsumer
                                         consumer) {
  CHECK(consumer);

  mojo::ScopedMessageHandle handle;
  MojoResult result = mojo::ReadMessageNew(
      mojo::MessagePipeHandle(pipe), &handle, MOJO_READ_MESSAGE_FLAG_NONE);
  if (result != MOJO_RESULT_OK) {
    return result;
  }

  // Messages sent by in-process C++ bindings may not have been serialized yet.
  // FAILED_PRECONDITION means the message already is.
  result = MojoSerializeMessage(handle->value(), nullptr);
  if (result != MOJO_RESULT_OK && result != MOJO_RESULT_FAILED_PRECONDITION) {
    return MOJO_RESULT_ABORTED;
  }

  mojo::Message message = mojo::Message::CreateFromMessageHandle(&handle);
  if (message.IsNull()) {
    return MOJO_RESULT_ABORTED;
  }

  base::span<const uint8_t> data = message.data_as_span();
  NSData* bytes = [NSData dataWithBytes:data.data() length:data.size()];
  NSMutableArray<NSNumber*>* handles =
      [NSMutableArray arrayWithCapacity:message.handles()->size()];
  for (mojo::ScopedHandle& message_handle : *message.mutable_handles()) {
    [handles addObject:@(message_handle.release().value())];
  }
  consumer(bytes, handles);
  return MOJO_RESULT_OK;
}

BraveMojoWatcherId BraveMojoWatch(BraveMojoHandle pipe,
                                  BraveMojoWatchCallback callback) {
  CheckOnMainThread();
  CHECK(callback);

  auto watcher = std::make_unique<mojo::SimpleWatcher>(
      FROM_HERE, mojo::SimpleWatcher::ArmingPolicy::AUTOMATIC,
      base::SequencedTaskRunner::GetCurrentDefault());
  MojoResult result = watcher->Watch(
      mojo::Handle(pipe),
      MOJO_HANDLE_SIGNAL_READABLE | MOJO_HANDLE_SIGNAL_PEER_CLOSED,
      MOJO_TRIGGER_CONDITION_SIGNALS_SATISFIED,
      base::BindRepeating(
          [](BraveMojoWatchCallback callback, MojoResult result,
             const mojo::HandleSignalsState& state) { callback(result); },
          callback));
  if (result != MOJO_RESULT_OK) {
    return 0;
  }
  return WatcherRegistry::Get().Add(std::move(watcher));
}

void BraveMojoCancelWatch(BraveMojoWatcherId watcher) {
  CheckOnMainThread();
  // Destroying the SimpleWatcher cancels it, and it supports being destroyed
  // from within its own callback.
  WatcherRegistry::Get().Remove(watcher);
}
