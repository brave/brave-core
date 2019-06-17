// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#import "RewardsLogStream.h"

RewardsLogStream::RewardsLogStream(const char* file,
                                   const int line,
                                   const ledger::LogLevel log_level) {
    
  this->log_stream = std::make_unique<UnbufferedLogger>([this, file, line, log_level](std::string data){
    this->log_stream->write({this->log_stream.get(), log_level, file, line, data});
  }, [this]{
    this->log_stream->flush();
  });
    

//  std::map<ledger::LogLevel, std::string> map {
//    {ledger::LOG_ERROR, "ERROR"},
//    {ledger::LOG_WARNING, "WARNING"},
//    {ledger::LOG_INFO, "INFO"},
//    {ledger::LOG_DEBUG, "DEBUG"},
//    {ledger::LOG_RESPONSE, "RESPONSE"}
//  };
//
//  log_message_ = map[log_level] + ": ";
}

RewardsLogStream::RewardsLogStream(const char* file,
                                   const int line,
                                   const ads::LogLevel log_level) {
    
  this->log_stream = std::make_unique<UnbufferedLogger>([this, file, line, log_level](std::string data){
    this->log_stream->write({this->log_stream.get(), log_level, file, line, data});
  }, [this]{
    this->log_stream->flush();
  });
    
//  std::map<ads::LogLevel, std::string> map {
//    {ads::LOG_ERROR, "ERROR"},
//    {ads::LOG_WARNING, "WARNING"},
//    {ads::LOG_INFO, "INFO"}
//  };
//    
//  log_message_ = map[log_level] + ": ";
}

RewardsLogStream::~RewardsLogStream() {
  //Auto flush the stream no matter what when this class is destroyed.
  //This will guarantee that iOS receives all the logs that were buffered in the stream.
  //This is done because we don't know when the logger causes a flush.
  //However, we know the logger creates "temporary instances" always and discards them after logging.
  //Therefore, it is safe to assume that logging is finished when the destructor is called.
  this->log_stream->stream().flush();
}

std::ostream& RewardsLogStream::stream() {
  //std::cerr << std::endl << log_message_;
  return log_stream->stream();
}
