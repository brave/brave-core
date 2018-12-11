/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat_helper_platform.h"

#if defined CHROMIUM_BUILD
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/path_service.h"
#include "base/logging.h"
#include "base/strings/utf_string_conversions.h"
#include "url/url_canon_stdstring.h"
#include "url/url_util.h"
#endif

#include "static_values.h"

namespace braveledger_bat_helper {

  void DecodeURLChars(const std::string& input, std::string& output) {
#if defined CHROMIUM_BUILD
    url::RawCanonOutputW<1024> canonOutput;
    url::DecodeURLEscapeSequences(input.c_str(), input.length(), &canonOutput);
    output = base::UTF16ToUTF8(base::StringPiece16(canonOutput.data(), canonOutput.length()));
#else
    //TODO: to implement
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

}  // namespace braveledger_bat_helper
