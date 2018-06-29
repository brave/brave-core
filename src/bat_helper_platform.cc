/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */



#if defined CHROMIUM_BUILD
#include "base/logging.h"
#include "base/sequenced_task_runner.h"
#include "base/strings/utf_string_conversions.h"
#include "base/task_scheduler/post_task.h"
#include "url/url_util.h"
#include "url/url_canon_stdstring.h"
#include "base/guid.h"
#endif

#include "bat_helper_platform.h"
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
} //namespace braveledger_bat_helper