/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_PAYMENTS_PUBLISHER_INFO_BACKEND_
#define BRAVE_BROWSER_PAYMENTS_PUBLISHER_INFO_BACKEND_

#include "base/files/file_path.h"
#include "base/macros.h"
#include "base/sequence_checker.h"

namespace leveldb {
class DB;
}  // namespace leveldb

namespace brave_rewards {

class PublisherInfoBackend {
 public:
  PublisherInfoBackend(const base::FilePath& path);
  ~PublisherInfoBackend();

  bool Put(const std::string& key, const std::string& value);
  bool Get(const std::string& lookup, std::string* value);
  bool Search(const std::vector<std::string>& prefixes,
              uint32_t start, uint32_t limit,
              std::vector<std::string>& results);
  bool Load(uint32_t start, uint32_t limit,
            std::vector<std::string>& results);

 private:
  bool EnsureInitialized();
  const base::FilePath path_;
  std::unique_ptr<leveldb::DB> db_;

  SEQUENCE_CHECKER(sequence_checker_);
  DISALLOW_COPY_AND_ASSIGN(PublisherInfoBackend);
};

}  // namespace brave_rewards

#endif  // BRAVE_BROWSER_PAYMENTS_PUBLISHER_INFO_BACKEND_
