/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_COMMON_REMOTE_WORKER_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_COMMON_REMOTE_WORKER_H_

#include <memory>
#include <type_traits>
#include <utility>

#include "base/memory/scoped_refptr.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "mojo/public/cpp/bindings/self_owned_receiver.h"

namespace base {
class SequencedTaskRunner;
}

namespace brave_rewards::internal {

// Schedules the creation of an interface implementation on a worker sequence
// and binds a pending remote to it. The implementation's lifetime is tied to
// the lifetime of the message pipe. When the pipe is reset, the implementation
// will be destroyed on the worker sequence.
template <typename Impl, typename Interface, typename... Args>
  requires(std::is_base_of_v<Interface, Impl>)
void CreateRemoteWorker(scoped_refptr<base::SequencedTaskRunner> task_runner,
                        mojo::PendingReceiver<Interface> pending_receiver,
                        Args&&... args) {
  auto create_on_worker = [](mojo::PendingReceiver<Interface> pending_receiver,
                             std::decay_t<Args>... args) {
    if (pending_receiver) {
      mojo::MakeSelfOwnedReceiver(
          std::make_unique<Impl>(std::forward<Args>(args)...),
          std::move(pending_receiver));
    }
  };

  task_runner->PostTask(
      FROM_HERE, base::BindOnce(create_on_worker, std::move(pending_receiver),
                                std::forward<Args>(args)...));
}

// A convenience wrapper around mojo::Remote that allows running a Mojo
// interface implementation on a worker sequence.
template <typename T>
class RemoteWorker {
 public:
  explicit RemoteWorker(scoped_refptr<base::SequencedTaskRunner> task_runner)
      : task_runner_(task_runner) {}

  ~RemoteWorker() = default;

  auto operator->() const { return remote_.get(); }

  bool is_bound() const { return remote_.is_bound(); }

  void reset() { remote_.reset(); }

  // Schedules the creation of an interface implementation on the worker
  // sequence and binds the remote. The implementation's lifetime is tied to
  // the lifetime of the message pipe. When the pipe is reset, or this object is
  // destroyed, the implementation will be destroyed on the worker sequence.
  template <typename Impl, typename... Args>
    requires(std::is_base_of_v<T, Impl>)
  void BindRemote(Args&&... args) {
    remote_.reset();
    CreateRemoteWorker<Impl>(task_runner_, remote_.BindNewPipeAndPassReceiver(),
                             std::forward<Args>(args)...);
  }

 private:
  mojo::Remote<T> remote_;
  scoped_refptr<base::SequencedTaskRunner> task_runner_;
};

}  // namespace brave_rewards::internal

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_COMMON_REMOTE_WORKER_H_
