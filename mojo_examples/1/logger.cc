#include "logger.h"

namespace logger {

LoggerImpl::LoggerImpl(mojo::PendingReceiver<mojom::Logger> pending_receiver)
    : receiver_(this, std::move(pending_receiver)) {
  receiver_.set_disconnect_handler(
      base::BindOnce(&LoggerImpl::OnError, base::Unretained(this)));
}

LoggerImpl::~LoggerImpl() = default;

void LoggerImpl::Log(const std::string& message) {
  LOG(ERROR) << "[LoggerImpl]: " << message;
  lines_.push_back(message);
}

void LoggerImpl::GetTail(GetTailCallback callback) {
  std::move(callback).Run(!lines_.empty() ? lines_.back() : "");
}

void LoggerImpl::OnError() {
  LOG(ERROR) << "[LoggerImpl]: "
             << "Client disconnected! Purging log lines...";
  lines_.clear();
}

}  // namespace logger
