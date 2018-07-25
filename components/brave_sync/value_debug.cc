/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */
#include "brave/components/brave_sync/value_debug.h"

#include <string>
#include "base/values.h"
#include "base/debug/stack_trace.h"
#include "base/strings/string_number_conversions.h"

namespace {

const int kIdentStep = 3;

std::string Spaces(const int &ident) {
  return std::string(ident, ' ');
}

std::string ToStringDict(const base::Value &dict, int ident) {
  CHECK(dict.is_dict());
  std::stringstream result;

  result << Spaces(ident) << "TYPE=DICTIONARY" << std::endl;
  result << Spaces(ident) << "[" << std::endl;
  for (const auto iter : dict.DictItems()) {
    result << Spaces(ident + kIdentStep) << "name=" << iter.first << std::endl;
    const auto & iter_val = iter.second;
    result << brave::debug::ToPrintableString(iter_val, ident + kIdentStep);
  }
  result << Spaces(ident) << "]";

  return result.str();
}

std::string ToStringList(const base::Value &list, int ident) {
  CHECK(list.is_list());
  std::stringstream result;
  result << Spaces(ident) << "TYPE=LIST" << std::endl;
  result << Spaces(ident) << "[" << std::endl;
  for (const auto &val : list.GetList() ) {
    result << brave::debug::ToPrintableString(val, ident + kIdentStep) << std::endl;
  }
  result << Spaces(ident) << "]";

  return result.str();
}

} // namespace

namespace brave {

namespace debug {

std::string ToPrintableString(const base::Value &val, const int &ident/* = 0*/) {
  std::stringstream result;
  switch (val.type()) {
    case base::Value::Type::NONE:
      result << Spaces(ident) << "TYPE=NONE VALUE=<empty>";
    break;
    case base::Value::Type::BOOLEAN:
      result << Spaces(ident) << "TYPE=BOOLEAN VALUE=" << (val.GetBool() ? "true" : "false") << std::endl;
    break;
    case base::Value::Type::INTEGER:
      result << Spaces(ident) << "TYPE=INTEGER VALUE=" << val.GetInt() << std::endl;
    break;
    case base::Value::Type::DOUBLE:
      result << Spaces(ident) << "TYPE=DOUBLE VALUE=" << val.GetDouble() << std::endl;
    break;
    case base::Value::Type::STRING:
      result << Spaces(ident) << "TYPE=STRING VALUE=<" << val.GetString() << ">" << std::endl;
    break;
    case base::Value::Type::BINARY:
      result << Spaces(ident) << "TYPE=BINARY VALUE=<";
      for (const auto &b : val.GetBlob()) {
        result << base::NumberToString(static_cast<unsigned char>(b)) << " ";
      }
      result << ">" << std::endl;
    break;
    case base::Value::Type::DICTIONARY:
      result << ToStringDict(val, ident) << std::endl;
    break;
    case base::Value::Type::LIST:
      result << ToStringList(val, ident) << std::endl;
    break;

  default:
    break;

  }
  return result.str();
}


} //namespace debug

} //namespace brave
