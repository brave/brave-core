/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_USERMODEL_USERMODEL_STATE_H_
#define BRAVE_BROWSER_BRAVE_USERMODEL_USERMODEL_STATE_H_

#include "base/files/file_path.h"
#include "base/macros.h"
#include "base/sequence_checker.h"

namespace leveldb {
class DB;
}  // namespace leveldb

namespace brave_ads {

class UserModelState {
 public:
  UserModelState(const base::FilePath& path);
  ~UserModelState();

  bool Put(const std::string& key, const std::string& value);
  bool Get(const std::string& lookup, std::string* value);

 private:
  bool EnsureInitialized();
  const base::FilePath path_;
  std::unique_ptr<leveldb::DB> db_;

  SEQUENCE_CHECKER(sequence_checker_);
  DISALLOW_COPY_AND_ASSIGN(UserModelState);
};

}  // namespace brave_ads

#endif  // BRAVE_BROWSER_BRAVE_USERMODEL_USERMODEL_STATE_H_
