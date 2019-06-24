#import "Logger.h"

std::function<void(UnbufferedLoggerData)> UnbufferedLogger::onWrite;
std::function<void()> UnbufferedLogger::onFlush;


LogPipe::~LogPipe()
{
    static_cast<std::ostream*>(this)->rdbuf(io_buffer);
}

UnbufferedLogger::~UnbufferedLogger()
{
    if (isHeapAllocated)
    {
        delete out_stream;
        out_stream = nullptr;
    }
}

void UnbufferedLogger::write(UnbufferedLoggerData data)
{
  if (onWrite) {
    onWrite(data);
  }
}

void UnbufferedLogger::flush()
{
  if (onFlush) {
    onFlush();
  }
}

void UnbufferedLogger::setLoggerCallbacks(std::function<void(UnbufferedLoggerData)> onWrite, std::function<void()> onFlush)
{
    UnbufferedLogger::onWrite = onWrite;
    UnbufferedLogger::onFlush = onFlush;
}
