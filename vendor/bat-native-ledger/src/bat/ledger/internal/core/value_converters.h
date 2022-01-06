/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_CORE_VALUE_CONVERTERS_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_CORE_VALUE_CONVERTERS_H_

#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "base/strings/string_number_conversions.h"
#include "base/time/time.h"
#include "base/time/time_to_iso8601.h"
#include "base/values.h"
#include "bat/ledger/internal/core/enum_string.h"

namespace ledger {

class ValueReader {
 public:
  explicit ValueReader(const base::Value& value);
  ~ValueReader();

  template <typename T>
  void Read(const std::string& name, T* dest) {
    DCHECK(dest);
    if (value_.is_dict()) {
      if (auto* value = value_.FindKey(name)) {
        absl::optional<T> parsed;
        ReadValue(*value, &parsed);
        if (parsed) {
          *dest = std::move(*parsed);
          return;
        }
      }
    }
    success_ = false;
  }

  template <typename T>
  void Read(const std::string& name, absl::optional<T>* dest) {
    DCHECK(dest);
    if (value_.is_dict()) {
      if (auto* value = value_.FindKey(name)) {
        ReadValue(*value, dest);
      }
    }
  }

  template <typename T>
  void Read(const std::string& name, std::vector<T>* dest) {
    DCHECK(dest);
    if (value_.is_dict()) {
      if (auto* list = value_.FindListKey(name)) {
        for (auto& item : list->GetList()) {
          absl::optional<T> parsed;
          ReadValue(item, &parsed);
          if (parsed) {
            dest->push_back(std::move(*parsed));
          }
        }
      }
    }
  }

  bool Succeeded() { return success_; }

 private:
  template <typename T, typename std::enable_if_t<std::is_class_v<T>, int> = 0>
  void ReadValue(const base::Value& value, absl::optional<T>* out) {
    DCHECK(out);
    *out = T::FromValue(value);
  }

  template <typename T, typename std::enable_if_t<std::is_enum_v<T>, int> = 0>
  void ReadValue(const base::Value& value, absl::optional<T>* out) {
    DCHECK(out);
    if (auto* s = value.GetIfString()) {
      *out = EnumString<T>::Parse(*s);
    }
  }

  void ReadValue(const base::Value& value, absl::optional<bool>* out) {
    DCHECK(out);
    if (auto parsed = value.GetIfBool()) {
      *out = *parsed;
    }
  }

  void ReadValue(const base::Value& value, absl::optional<int>* out) {
    DCHECK(out);
    if (auto parsed = value.GetIfInt()) {
      *out = *parsed;
    }
  }

  void ReadValue(const base::Value& value, absl::optional<double>* out) {
    DCHECK(out);
    if (auto parsed = value.GetIfDouble()) {
      *out = *parsed;
    } else if (auto* s = value.GetIfString()) {
      double v;
      if (base::StringToDouble(*s, &v)) {
        *out = v;
      }
    }
  }

  void ReadValue(const base::Value& value, absl::optional<std::string>* out) {
    DCHECK(out);
    if (auto* s = value.GetIfString()) {
      *out = std::string(*s);
    }
  }

  void ReadValue(const base::Value& value, absl::optional<int64_t>* out) {
    DCHECK(out);
    if (auto* s = value.GetIfString()) {
      int64_t n;
      if (base::StringToInt64(*s, &n)) {
        *out = n;
      }
    }
  }

  void ReadValue(const base::Value& value, absl::optional<base::Time>* out) {
    DCHECK(out);
    if (auto* s = value.GetIfString()) {
      base::Time time;
      if (base::Time::FromString(s->c_str(), &time)) {
        *out = time;
      }
    }
  }

  void ReadValue(const base::Value& value,
                 absl::optional<base::TimeDelta>* out) {
    DCHECK(out);
    absl::optional<double> d;
    ReadValue(value, &d);
    if (d) {
      *out = base::Seconds(*d);
    }
  }

  const base::Value& value_;
  bool success_ = true;
};

template <typename Data>
class StructValueReader {
 public:
  explicit StructValueReader(const base::Value& value) : reader_(value) {}

  ~StructValueReader() = default;

  template <typename T>
  void Read(const std::string& name, T Data::*dest) {
    reader_.Read(name, &(data_.*dest));
  }

  absl::optional<Data> Finish() {
    if (reader_.Succeeded()) {
      return std::move(data_);
    }
    return {};
  }

 private:
  ValueReader reader_;
  Data data_;
};

class ValueWriter {
 public:
  ValueWriter();
  ~ValueWriter();

  template <typename T>
  void Write(const std::string& name, const T& data) {
    value_.SetKey(name, ToValue(data));
  }

  template <typename T>
  void Write(const std::string& name, const absl::optional<T>& data) {
    if (data) {
      value_.SetKey(name, ToValue(*data));
    }
  }

  template <typename T>
  void Write(const std::string& name, const std::vector<T>& data) {
    base::Value list(base::Value::Type::LIST);
    for (auto& item : data) {
      list.Append(ToValue(item));
    }
    value_.SetKey(name, std::move(list));
  }

  base::Value Finish() {
    base::Value val = std::move(value_);
    value_ = base::Value(base::Value::Type::DICTIONARY);
    return val;
  }

 private:
  template <typename T, typename std::enable_if_t<std::is_class_v<T>, int> = 0>
  base::Value ToValue(const T& data) {
    return data.ToValue();
  }

  template <typename T, typename std::enable_if_t<std::is_enum_v<T>, int> = 0>
  base::Value ToValue(T value) {
    return base::Value(StringifyEnum(value));
  }

  base::Value ToValue(bool value) { return base::Value(value); }

  base::Value ToValue(int value) { return base::Value(value); }

  base::Value ToValue(double value) { return base::Value(value); }

  base::Value ToValue(const std::string& value) { return base::Value(value); }

  base::Value ToValue(const base::StringPiece value) {
    return base::Value(value);
  }

  base::Value ToValue(int64_t value) {
    return base::Value(base::NumberToString(value));
  }

  base::Value ToValue(base::Time time) {
    return ToValue(base::TimeToISO8601(time));
  }

  base::Value ToValue(base::TimeDelta value) {
    return ToValue(value.InSecondsF());
  }

  base::Value value_{base::Value::Type::DICTIONARY};
};

}  // namespace ledger

#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_CORE_VALUE_CONVERTERS_H_
