#include "base/logging.h"
#include "base/run_loop.h"
#include "base/task/single_thread_task_executor.h"
#include "base/task/single_thread_task_runner.h"
#include "base/task/single_thread_task_runner_thread_mode.h"
#include "base/task/task_traits.h"
#include "base/task/thread_pool.h"
#include "base/task/thread_pool/thread_pool_instance.h"
#include "brave/mojo_vs_task_runners/mojom/logger.mojom.h"
#include "mojo/core/embedder/embedder.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "mojo/public/cpp/bindings/remote.h"

class LoggerImpl : public mojom::Logger {
 public:
  static LoggerImpl& Logger();

  ~LoggerImpl() override;

  void Log(const std::string& message) override;

 private:
  LoggerImpl();
};

LoggerImpl& LoggerImpl::Logger() {
  thread_local LoggerImpl logger;
  return logger;
}

inline void LoggerImpl::Log(const std::string& message) {
  LOG(ERROR) << message;
}

LoggerImpl::LoggerImpl() {
  Log("LoggerImpl()");
}

LoggerImpl::~LoggerImpl() {
  Log("~LoggerImpl()");
}

class SelfOwnedReceiver {
 public:
  static void Create(mojo::PendingReceiver<mojom::Logger> receiver,
                     scoped_refptr<base::SingleThreadTaskRunner> task_runner,
                     base::OnceClosure disconnect_handler) {
    new SelfOwnedReceiver(std::move(receiver), std::move(task_runner),
                          std::move(disconnect_handler));
  }

  SelfOwnedReceiver(const SelfOwnedReceiver&) = delete;
  SelfOwnedReceiver& operator=(const SelfOwnedReceiver&) = delete;

 private:
  void Delete() {
    LOG(ERROR) << "Before receiver_.reset(): "
               << task_runner_->GetCountForTesting() - 1;
    receiver_.reset();
    LOG(ERROR) << "After receiver_.reset(): "
               << task_runner_->GetCountForTesting() - 1;
    delete this;
  }

  SelfOwnedReceiver(mojo::PendingReceiver<mojom::Logger> receiver,
                    scoped_refptr<base::SingleThreadTaskRunner> task_runner,
                    base::OnceClosure disconnect_handler)
      : receiver_(&LoggerImpl::Logger(), std::move(receiver), task_runner),
        task_runner_(task_runner) {
    receiver_.set_disconnect_handler(
        base::BindOnce(&SelfOwnedReceiver::Delete, base::Unretained(this))
            .Then(std::move(disconnect_handler)));
  }

  ~SelfOwnedReceiver() = default;

  mojo::Receiver<mojom::Logger> receiver_;
  scoped_refptr<base::SingleThreadTaskRunner> task_runner_;
};

void CreateSelfOwnedReceiver(
    mojo::PendingReceiver<mojom::Logger> receiver,
    scoped_refptr<base::SingleThreadTaskRunner> task_runner,
    base::OnceClosure disconnect_handler) {
  SelfOwnedReceiver::Create(std::move(receiver), std::move(task_runner),
                            std::move(disconnect_handler));
}

// npm run apply_patches
// npm run build -- --target szilard
// ..\out\Component\mojo_vs_task_runners.exe
int main(int, char*[]) {
  mojo::core::Init();

  base::ThreadPoolInstance::CreateAndStartWithDefaultParams("thread_pool");

  base::SingleThreadTaskExecutor task_executor;
  base::RunLoop loop;

  mojo::Remote<mojom::Logger> remote;
  {
    auto task_runner = base::ThreadPool::CreateSingleThreadTaskRunner(
        {base::MayBlock(), base::WithBaseSyncPrimitives(),
         base::TaskPriority::USER_BLOCKING,
         base::TaskShutdownBehavior::BLOCK_SHUTDOWN},
        base::SingleThreadTaskRunnerThreadMode::DEDICATED);
    task_runner->PostTask(FROM_HERE,
                          base::BindOnce(&CreateSelfOwnedReceiver,
                                         remote.BindNewPipeAndPassReceiver(),
                                         task_runner, loop.QuitClosure()));
  }
  remote->Log("...");
  remote.reset();

  loop.Run();

  base::ThreadPoolInstance::Get()->Shutdown();

  return 0;
}
