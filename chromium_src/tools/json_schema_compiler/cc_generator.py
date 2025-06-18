# Copyright (c) 2025 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/. */

import override_utils


@override_utils.override_method(_Generator)
def _GenerateCreateValueFromType(self, original_method, type_, var, is_ptr):
    """Override to treat 64-bit values as binary values

  This override is necessary to provide a handling for storing 64-bit integers
  into base::Value as binary values, as these values cannot be represented
  directly in base::Value.
  """
    underlying_type = self._type_helper.FollowRef(type_)
    if (underlying_type.property_type == PropertyType.BINARY
            and underlying_type.override):
        if is_ptr:
            var = '*%s' % var
        return 'base::Value(base::as_byte_span(base::NumberToString(%s)))' % var

    return original_method(self, type_, var, is_ptr)


@override_utils.override_method(_Generator)
def _GeneratePopulateVariableFromValue(self,
                                       original_method,
                                       type_,
                                       src_var,
                                       dst_var,
                                       failure_value,
                                       is_ptr=False):
    """Override to populate 64-bit values from base::Value binary blobs

   This override treats binary blobs in a base::Value as 64-bit integers if the
   field in question has an override attribute for a specific 64-bit type. This
   allows us to create a C++ type with 64-bit integers that have been stored
   as binary values in a base::Value, abstracting away from the user the dancing
   between a binary representation and a 64-bit integer.
   """
    c = Code()
    underlying_type = self._type_helper.FollowRef(type_)
    if (underlying_type.property_type == PropertyType.BINARY
            and underlying_type.override is not None):
        (c.Sblock('if (%(src_var)s.is_blob()) {') \
          .Append('%s value = {};' % underlying_type.override))

        conversion_func_suffix = ('Uint64' if underlying_type.override
                                  == 'uint64_t' else 'Int64')
        (c.Sblock('if (base::StringTo' + conversion_func_suffix + \
                  '(base::as_string_view(%(src_var)s.GetBlob()), &value)) {') \
          .Append('%(dst_var)s = value;'))
        (c.Eblock('}') \
          .Sblock('else {')
          .Concat(self._AppendError16(
            'u"\'%%(key)s\': binary value is not covertible to overriden '
            'field type " + ' +
            self._util_cc_helper.GetValueTypeString('%%(src_var)s'))) \
          .Append('return %(failure_value)s;')
          .Eblock('}')
        )
        (c.Eblock('}') \
          .Sblock('else if (%(src_var)s.is_int()) {')
          .Append('%(dst_var)s = %(src_var)s.GetInt();')
        )
        (c.Eblock('}') \
          .Sblock('else {')
          .Concat(self._AppendError16(
            'u"\'%%(key)s\': Overriden field should have type binary or '
            'int " + ' +
            self._util_cc_helper.GetValueTypeString('%%(src_var)s'))) \
          .Append('return %(failure_value)s;')
          .Eblock('}')
        )

        return Code().Sblock('{').Concat(
            c.Substitute({
                'cpp_type': self._type_helper.GetCppType(type_),
                'src_var': src_var,
                'dst_var': dst_var,
                'failure_value': failure_value,
                'key': type_.name,
                'parent_key': type_.parent.name,
            })).Eblock('}')
    return original_method(self, type_, src_var, dst_var, failure_value,
                           is_ptr)
