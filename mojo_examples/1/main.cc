/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/run_loop.h"
#include "base/task/single_thread_task_executor.h"
#include "logger.h"
#include "mojo/core/embedder/embedder.h"
#include "mojo/public/cpp/bindings/remote.h"

using namespace logger;

auto Bind1(mojo::Remote<mojom::Logger>& remote) {
  mojo::MessagePipe message_pipe;
  remote = mojo::Remote<mojom::Logger>(
      mojo::PendingRemote<mojom::Logger>(std::move(message_pipe.handle0), 0));
  mojo::PendingReceiver<mojom::Logger> pending_receiver(
      std::move(message_pipe.handle1));

  return LoggerImpl(std::move(pending_receiver));
}

auto Bind2(mojo::Remote<mojom::Logger>& remote) {
  return LoggerImpl(remote.BindNewPipeAndPassReceiver());
}

// npm run build -- --target szilard
// ..\out\Component\mojo_example_1.exe
int main(int, char*[]) {
  mojo::core::Init();

  base::SingleThreadTaskExecutor task_executor;
  DCHECK(base::SequencedTaskRunner::GetCurrentDefault());
  base::RunLoop loop;

  mojo::Remote<mojom::Logger> remote;
  // Both Bind1() and Bind2() bind |remote| to the default
  // sequence, which is initialized by base::SingleThreadTaskExecutor's
  // constructor above.
  auto logger = Bind2(remote);  // equivalent to Bind1(remote);
  remote->Log("Nice!");
  // remote->GetTail(base::BindOnce([](const std::string& message) {
  //   if (!message.empty()) {
  //     LOG(ERROR) << "Tail was: " << message;
  //   }
  // }));
  remote.reset();

  loop.RunUntilIdle();

  return 0;
}
