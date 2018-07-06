/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat_helper_platform.h"

#if defined CHROMIUM_BUILD
#include "base/guid.h"
#include "base/logging.h"
#include "base/sequenced_task_runner.h"
#include "base/strings/utf_string_conversions.h"
#include "base/task_scheduler/post_task.h"
#include "url/url_canon_stdstring.h"
#include "url/url_util.h"
#endif

#include "static_values.h"

namespace braveledger_bat_helper {

#if defined CHROMIUM_BUILD
  std::unique_ptr<BatClientWebRequestChromium> batClientWebRequest(new BatClientWebRequestChromium);
#endif
  scoped_refptr<base::SequencedTaskRunner> task_runner_;

  void DecodeURLChars(const std::string& input, std::string& output) {
#if defined CHROMIUM_BUILD
    url::RawCanonOutputW<1024> canonOutput;
    url::DecodeURLEscapeSequences(input.c_str(), input.length(), &canonOutput);
    output = base::UTF16ToUTF8(base::StringPiece16(canonOutput.data(), canonOutput.length()));
#else
    //TODO: to implement
#endif
  }

  std::string GenerateGUID()
  {
#if defined CHROMIUM_BUILD
    return base::GenerateGUID();
#else
    //TODO: to implement
    return "please implement";
#endif
  }

  void encodeURIComponent(const std::string & instr, std::string & outstr)
  {
#if defined CHROMIUM_BUILD
    url::StdStringCanonOutput surveyorIdCanon(&outstr);
    url::EncodeURIComponent(instr.c_str(), instr.length(), &surveyorIdCanon);
    surveyorIdCanon.Complete();
#else
    //TODO: to implement
    assert(false);
#endif
  }

  void getHomeDir(std::string& home)
  {
#if defined CHROMIUM_BUILD
    base::FilePath dirToSave;
    base::PathService::Get(base::DIR_HOME, &dirToSave);
    home = dirToSave.value();
#else
    //TODO: to implement
    assert(false);
#endif
  }

  void appendPath(const std::string& root, const std::string& leaf, std::string & path)
  {
#if defined CHROMIUM_BUILD
    base::FilePath root_path (root.c_str());
    root_path = root_path.Append(leaf);
    path = root_path.value();
#else
    //TODO: to implement
    assert(false);
#endif
  }

  bool writeFile(const std::string & path, const std::string& data)
  {
#if defined CHROMIUM_BUILD
    base::FilePath dirToSave (path.c_str());

    int succeded = base::WriteFile(dirToSave, data.c_str(), data.length());
    LOG(ERROR) << "writeToFile to: " << dirToSave << " : " << data.length() << " : " << succeded;
    assert(succeded != -1);
    return (succeded != -1) ? true : false;
#else
    //TODO: to implement
    assert(false);
    return false;
#endif
  }


  bool readFile(const std::string & path, std::ostringstream & ss)
  {
#if defined CHROMIUM_BUILD
    bool succeded = false;
    base::FilePath path_to_file (path.c_str());
    base::PathExists(path_to_file);
    if (base::PathExists(path_to_file)) {
      std::string str;
      succeded = base::ReadFileToString(path_to_file, &str);
      ss << str;
    }
    return succeded;
#else
    assert(false);
    return false;
#endif
  }

}  // namespace braveledger_bat_helper
