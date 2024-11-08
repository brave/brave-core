# Copyright (c) 2023 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

# pylint: disable=import-error,too-many-return-statements,no-self-use
# pylint: disable=too-many-branches

import os

import secrets
import string
import mojom.generate.module as mojom
import mojom.generate.generator as generator
from mojom.generate.generator import WriteFile
from mojom.generate.template_expander import UseJinja

class MojoTypemap():
    @staticmethod
    def IsMojoType(_):
        return False
    def __init__(self, kind, is_inside_container=False):
        self.kind = kind
        self.is_inside_container = is_inside_container
    def ObjCWrappedType(self):
        pass
    def ExpectedCppType(self):
        pass
    def DefaultObjCValue(self, default):
        pass
    def ObjCToCpp(self, accessor):
        pass
    def CppToObjC(self, accessor):
        pass

class StringMojoTypemap(MojoTypemap):
    @staticmethod
    def IsMojoType(kind):
        return mojom.IsStringKind(kind)
    def ObjCWrappedType(self):
        return "NSString*"
    def ExpectedCppType(self):
        return "std::string"
    def DefaultObjCValue(self, default):
        return "@%s" % default if default is not None else '@""'
    def ObjCToCpp(self, accessor):
        return "base::SysNSStringToUTF8(%s)" % accessor
    def CppToObjC(self, accessor):
        return "base::SysUTF8ToNSString(%s)" % accessor

class TimeMojoTypemap(MojoTypemap):
    @staticmethod
    def IsMojoType(kind):
        return (mojom.IsStructKind(kind) and
                kind.qualified_name == 'mojo_base.mojom.Time')
    def ObjCWrappedType(self):
        return "NSDate*"
    def ExpectedCppType(self):
        return "base::Time"
    def DefaultObjCValue(self, default):
        return "[[NSDate alloc] init]"
    def ObjCToCpp(self, accessor):
        return "base::Time::FromNSDate(%s)" % accessor
    def CppToObjC(self, accessor):
        return "%s.ToNSDate()" % accessor

class TimeDeltaMojoTypemap(MojoTypemap):
    @staticmethod
    def IsMojoType(kind):
        return (mojom.IsStructKind(kind) and
                kind.qualified_name == 'mojo_base.mojom.TimeDelta')
    def ObjCWrappedType(self):
        return "NSDate*"
    def ExpectedCppType(self):
        return "base::TimeDelta"
    def DefaultObjCValue(self, default):
        return "[[NSDate alloc] init]"
    def ObjCToCpp(self, accessor):
        return "base::Seconds(%s.timeIntervalSince1970)" % accessor
    def CppToObjC(self, accessor):
        return ("[NSDate dateWithTimeIntervalSince1970:%s.InSecondsF()]"
                % accessor)

class URLMojoTypemap(MojoTypemap):
    @staticmethod
    def IsMojoType(kind):
        return (mojom.IsStructKind(kind) and
                kind.qualified_name == 'url.mojom.Url')
    def ObjCWrappedType(self):
        return "NSURL*"
    def ExpectedCppType(self):
        return "GURL"
    def DefaultObjCValue(self, default):
        return "[[NSURL alloc] init]"
    def ObjCToCpp(self, accessor):
        return "net::GURLWithNSURL(%s)" % accessor
    def CppToObjC(self, accessor):
        return "net::NSURLWithGURL(%s)" % accessor

class OriginMojoTypemap(MojoTypemap):
    @staticmethod
    def IsMojoType(kind):
        return (mojom.IsStructKind(kind) and
                kind.qualified_name == 'url.mojom.Origin')
    def ObjCWrappedType(self):
        return "URLOriginIOS*"
    def ExpectedCppType(self):
        return "url::Origin"
    def DefaultObjCValue(self, default):
        return "[[URLOriginIOS alloc] init]"
    def ObjCToCpp(self, accessor):
        return "[%s underlyingOrigin]" % accessor
    def CppToObjC(self, accessor):
        return "[[URLOriginIOS alloc] initWithOrigin:%s]" % accessor

_kind_to_c_type = {
    mojom.BOOL: "bool",
    mojom.INT8: "int8_t",
    mojom.UINT8: "uint8_t",
    mojom.INT16: "int16_t",
    mojom.UINT16: "uint16_t",
    mojom.INT32: "int32_t",
    mojom.UINT32: "uint32_t",
    mojom.FLOAT: "float",
    mojom.INT64: "int64_t",
    mojom.UINT64: "uint64_t",
    mojom.DOUBLE: "double",
}

_kind_to_nsnumber_getter = {
    mojom.BOOL: "boolValue",
    mojom.INT8: "charValue",
    mojom.UINT8: "unsignedCharValue",
    mojom.INT16: "shortValue",
    mojom.UINT16: "unsignedShortValue",
    mojom.INT32: "intValue",
    mojom.UINT32: "unsignedIntValue",
    mojom.FLOAT: "floatValue",
    mojom.INT64: "longLongValue",
    mojom.UINT64: "unsignedLongLongValue",
    mojom.DOUBLE: "doubleValue",
}

class NumberMojoTypemap(MojoTypemap):
    @staticmethod
    def IsMojoType(kind):
        return kind in _kind_to_nsnumber_getter
    def ObjCWrappedType(self):
        if self.is_inside_container:
            return "NSNumber*"
        return _kind_to_c_type[self.kind]
    def ExpectedCppType(self):
        return _kind_to_c_type[self.kind]
    def ObjCToCpp(self, accessor):
        if self.is_inside_container:
            return "%s.%s" % (accessor, _kind_to_nsnumber_getter[self.kind])
        return accessor
    def DefaultObjCValue(self, default):
        # Default for primitives not needed, array/map default is empty
        return default
    def CppToObjC(self, accessor):
        if self.is_inside_container:
            return "@(%s)" % accessor
        return accessor

class ArrayMojoTypemap(MojoTypemap):
    def __init__(self, kind, is_inside_container):
        super().__init__(
            kind, is_inside_container=is_inside_container)
        self.wrappedTypemap = MojoTypemapForKind(kind.kind, True)
    @staticmethod
    def IsMojoType(kind):
        return mojom.IsArrayKind(kind)
    def ObjCWrappedType(self):
        return "NSArray<%s>*" % self.wrappedTypemap.ObjCWrappedType()
    def ExpectedCppType(self):
        return "std::vector<%s>" % self.wrappedTypemap.ExpectedCppType()
    def DefaultObjCValue(self, default):
        return "@[]"
    def ObjCToCpp(self, accessor):
        local_var_name = _RandomLocalVarName()
        args = (self.wrappedTypemap.ExpectedCppType(), local_var_name,
                self.wrappedTypemap.ObjCWrappedType(), local_var_name,
                accessor, local_var_name, self.wrappedTypemap.ObjCToCpp(
                    local_var_name), local_var_name)
        return """^{
            std::vector<%s> array_%s;
            for (%s %s in %s) {
                array_%s.push_back(%s);
            }
            return array_%s;
        }()""" % args
    def CppToObjC(self, accessor):
        args = (accessor, self.wrappedTypemap.CppToObjC("o"))
        return """[param = std::cref(%s)]{
            const auto a = [[NSMutableArray alloc] init];
            for (const auto& o : param.get()) {
                [a addObject:%s];
            }
            return a;
        }()""" % args

class StructMojoTypemap(MojoTypemap):
    @staticmethod
    def IsMojoType(kind):
        return mojom.IsStructKind(kind)
    def ObjCWrappedType(self):
        return "%s%s*" % (ObjCPrefixFromKind(self.kind), self.kind.name)
    def ExpectedCppType(self):
        return "%s::%sPtr" % (CppNamespaceFromKind(self.kind), self.kind.name)
    def DefaultObjCValue(self, default):
        args = (ObjCPrefixFromKind(self.kind), self.kind.name)
        return "[[%s%s alloc] init]" % args
    def ObjCToCpp(self, accessor):
        return "%s.cppObjPtr" % accessor
    def CppToObjC(self, accessor):
        args = (ObjCPrefixFromKind(self.kind), self.kind.name, self.kind.name,
                accessor)
        return "[[%s%s alloc] initWith%s:*%s]" % args

class EnumMojoTypemap(MojoTypemap):
    @staticmethod
    def IsMojoType(kind):
        return mojom.IsEnumKind(kind)
    def _ObjCWrappedType(self):
        return "%s%s" % (ObjCPrefixFromKind(self.kind), self.kind.name)
    def ObjCWrappedType(self):
        if self.is_inside_container:
            return "NSNumber*"
        return self._ObjCWrappedType()
    def ExpectedCppType(self):
        return "%s::%s" % (CppNamespaceFromKind(self.kind), self.kind.name)
    def DefaultObjCValue(self, default):
        if default is None:
            return None
        return self.CppToObjC("%s::%s" % (self.ExpectedCppType(), default.name))
    def ObjCToCpp(self, accessor):
        if self.is_inside_container:
            accessor = "%s.intValue" % accessor
        return "static_cast<%s>(%s)" % (self.ExpectedCppType(), accessor)
    def CppToObjC(self, accessor):
        result = "static_cast<%s>(%s)" % (self._ObjCWrappedType(), accessor)
        if self.is_inside_container:
            return "@(%s)" % result
        return result

class DictionaryMojoTypemap(MojoTypemap):
    def __init__(self, kind, is_inside_container):
        super().__init__(kind, is_inside_container=is_inside_container)
        self.keyTypemap = MojoTypemapForKind(self.kind.key_kind, True)
        self.valueTypemap = MojoTypemapForKind(self.kind.value_kind, True)
    @staticmethod
    def IsMojoType(kind):
        return mojom.IsMapKind(kind)
    def ObjCWrappedType(self):
        return "NSDictionary<%s, %s>*" % (self.keyTypemap.ObjCWrappedType(),
                                          self.valueTypemap.ObjCWrappedType())
    def ExpectedCppType(self):
        return "base::flat_map<%s, %s>" % (self.keyTypemap.ExpectedCppType(),
                                           self.valueTypemap.ExpectedCppType())
    def DefaultObjCValue(self, default):
        return "@{}"
    def ObjCToCpp(self, accessor):
        local_var_name = _RandomLocalVarName()
        args = (self.keyTypemap.ExpectedCppType(),
                self.valueTypemap.ExpectedCppType(),
                self.keyTypemap.ObjCWrappedType(), local_var_name, accessor,
                self.keyTypemap.ObjCToCpp(local_var_name),
                self.valueTypemap.ObjCToCpp("%s[%s]" % (accessor,
                                                        local_var_name)))
        return """^{
            base::flat_map<%s, %s> map;
            for (%s %s in %s) {
                map[%s] = %s;
            }
            return map;
        }()""" % args
    def CppToObjC(self, accessor):
        args = (accessor, self.keyTypemap.CppToObjC("item.first"),
                self.valueTypemap.CppToObjC("item.second"))
        return """^{
            const auto d = [NSMutableDictionary new];
            for (const auto& item : %s) {
                d[%s] = %s;
            }
            return d;
        }()""" % args

class BaseValueMojoTypemap(MojoTypemap):
    @staticmethod
    def IsMojoType(kind):
        return (mojom.IsUnionKind(kind) and
                kind.qualified_name == 'mojo_base.mojom.Value')
    def ObjCWrappedType(self):
        return "MojoBaseValue*"
    def ExpectedCppType(self):
        return "base::Value"
    def DefaultObjCValue(self, default):
        return "[[MojoBaseValue alloc] init]"
    def ObjCToCpp(self, accessor):
        return "%s.cppObjPtr" % accessor
    def CppToObjC(self, accessor):
        return "[[MojoBaseValue alloc] initWithValue:%s.Clone()]" % accessor

class BaseDictionaryValueMojoTypemap(MojoTypemap):
    @staticmethod
    def IsMojoType(kind):
        return (mojom.IsStructKind(kind) and
                kind.qualified_name == \
                    'mojo_base.mojom.DictionaryValue')
    def ObjCWrappedType(self):
        return "NSDictionary<NSString*,MojoBaseValue*>*"
    def ExpectedCppType(self):
        return "base::Value::Dict"
    def DefaultObjCValue(self, default):
        return "@{}"
    def ObjCToCpp(self, accessor):
        return "brave::BaseValueDictFromNSDictionary(%s)" % accessor
    def CppToObjC(self, accessor):
        return "brave::NSDictionaryFromBaseValueDict(%s.Clone())" % accessor

class BaseListValueMojoTypemap(MojoTypemap):
    @staticmethod
    def IsMojoType(kind):
        return (mojom.IsStructKind(kind) and
                kind.qualified_name == 'mojo_base.mojom.ListValue')

    def ObjCWrappedType(self):
        return "NSArray<MojoBaseValue*>*"

    def ExpectedCppType(self):
        return "base::Value"

    def DefaultObjCValue(self, default):
        return "@[]"

    def ObjCToCpp(self, accessor):
        return "brave::BaseValueListFromNSArray(%s)" % accessor

    def CppToObjC(self, accessor):
        return "brave::NSArrayFromBaseValue(%s.Clone())" % accessor

class PendingRemoteMojoTypemap(MojoTypemap):
    @staticmethod
    def IsMojoType(kind):
        return mojom.IsPendingRemoteKind(kind)
    def ObjCWrappedType(self):
        return "id<%s%s>" % (ObjCPrefixFromKind(self.kind.kind),
                             self.kind.kind.name)
    def ExpectedCppType(self):
        return "%s::%s" % (CppNamespaceFromKind(self.kind.kind),
                           self.kind.kind.name)
    def DefaultObjCValue(self, default):
        return None
    def ObjCToCpp(self, accessor):
        name = (ObjCPrefixFromKind(self.kind.kind) + self.kind.kind.name +
                "Bridge")
        args = (name, accessor, UnderToLowerCamel(name))
        return """^{
            auto bridge = std::make_unique<%s>(%s);
            auto bridgePtr = bridge.get();
            self->_%sReceivers.push_back(base::SequenceBound<decltype(bridge)>(
                web::GetUIThreadTaskRunner({}), std::move(bridge)));
            return bridgePtr->GetRemote();
        }()""" % args
    def CppToObjC(self, accessor):
        return None

class UnionMojoTypemap(MojoTypemap):
    @staticmethod
    def IsMojoType(kind):
        return mojom.IsUnionKind(kind)
    def ObjCWrappedType(self):
        return "%s%s*" % (ObjCPrefixFromKind(self.kind), self.kind.name)
    def ExpectedCppType(self):
        return "%s::%sPtr" % (CppNamespaceFromKind(self.kind), self.kind.name)
    def DefaultObjCValue(self, default):
        args = (ObjCPrefixFromKind(self.kind), self.kind.name)
        return "[[%s%s alloc] init]" % args
    def ObjCToCpp(self, accessor):
        return "%s.cppObjPtr" % accessor
    def CppToObjC(self, accessor):
        args = (ObjCPrefixFromKind(self.kind), self.kind.name, self.kind.name,
                accessor)
        return "[[%s%s alloc] initWith%s:*%s]" % args

_mojo_typemaps = [
    StringMojoTypemap,
    TimeMojoTypemap,
    TimeDeltaMojoTypemap,
    URLMojoTypemap,
    OriginMojoTypemap,
    NumberMojoTypemap,
    EnumMojoTypemap,
    BaseValueMojoTypemap,
    ArrayMojoTypemap,
    BaseListValueMojoTypemap,
    DictionaryMojoTypemap,
    BaseDictionaryValueMojoTypemap,
    StructMojoTypemap,
    PendingRemoteMojoTypemap,
    UnionMojoTypemap,
]

def _RandomLocalVarName():
    return ''.join(secrets.choice(string.ascii_letters) for i in range(16))

def MojoTypemapForKind(kind, is_inside_container=False):
    typemap = next((x for x in _mojo_typemaps if x.IsMojoType(kind)), None)
    if typemap is None:
        raise Exception("No typemap available for the given kind: %s" % kind)
    return typemap(kind, is_inside_container)

def CppNamespaceFromKind(kind):
    return str(kind.module.namespace).replace(".", "::")

def ObjCPrefixFromKind(kind):
    return ObjCPrefixFromModule(kind.module)

def ObjCPrefixFromModule(module):
    return UnderToCamel(str(module.namespace).replace(".mojom", ""))

def UnderToCamel(value, lower_initial=False, digits_split=False):
    # There are some mojom files that don't use snake_cased names, so we try to
    # fix that to get more consistent output.
    return generator.ToCamel(generator.ToLowerSnakeCase(value),
                             lower_initial=lower_initial,
                             digits_split=digits_split)

def UnderToLowerCamel(value):
    return UnderToCamel(value, lower_initial=True)

class Generator(generator.Generator):
    @staticmethod
    def GetTemplatePrefix():
        return "objc_templates"

    def GetFilters(self):
        objc_filters = {
            "objc_property_modifiers": self._GetObjCPropertyModifiers,
            "objc_property_default": self._GetObjCPropertyDefaultValue,
            "objc_union_null_return_value": self._GetObjCUnionNullReturnValue,
            "objc_property_needs_default_assignment": \
                self._ObjcPropertyNeedsDefaultValueAssignment,
            "objc_wrapper_type": self._GetObjCWrapperType,
            "objc_property_formatter": self._ObjCPropertyFormatter,
            "objc_method_name_formatter": self._ObjCMethodNameFormatter,
            "objc_enum_formatter": self._ObjCEnumFormatter,
            "objc_argument_modifiers": self._GetObjCArgumentModifiers,
            "cpp_to_objc_assign": self._CppToObjCAssign,
            "const_objc_assign": self._ConstObjCAssign,
            "objc_to_cpp_assign": self._ObjCToCppAssign,
            "expected_cpp_param_type": self._GetExpectedCppParamType,
            "cpp_namespace_from_kind": CppNamespaceFromKind,
            "under_to_camel": UnderToCamel,
            "under_to_lower_camel": UnderToLowerCamel,
            "interface_remote_sets": self._GetInterfaceRemoteSets,
            "objc_import_module_name": self._GetObjCImportModuleName,
        }
        return objc_filters

    def _GetObjCImportModuleName(self, module):
        return os.path.basename(module.path)

    def _GetInterfaceRemoteSets(self, interface):
        remotes = []
        for method in interface.methods:
            for param in method.parameters:
                if mojom.IsPendingRemoteKind(param.kind):
                    name = "%s%sBridge" % (ObjCPrefixFromKind(param.kind.kind),
                                           param.kind.kind.name)
                    remotes.append(name)
        return set(remotes)

    def _GetExpectedCppParamType(self, kind):
        should_pass_param_by_value = self._ShouldPassParamByValue(kind)
        typemap = MojoTypemapForKind(kind, False)
        typestring = typemap.ExpectedCppType()
        if (mojom.IsNullableKind(kind)
                and not (isinstance(
                    typemap, (StructMojoTypemap, UnionMojoTypemap)))):
            typestring = "std::optional<%s>" % typestring
        if should_pass_param_by_value:
            return typestring
        return "const %s&" % typestring

    def _GetObjCPropertyModifiers(self, kind, inside_union=False):
        modifiers = ['nonatomic']
        if (mojom.IsArrayKind(kind) or mojom.IsStringKind(kind)
                or mojom.IsMapKind(kind) or mojom.IsStructKind(kind)
                or mojom.IsUnionKind(kind)):
            modifiers.append('copy')
        if ((inside_union and mojom.IsObjectKind(kind))
                or mojom.IsNullableKind(kind)):
            modifiers.append('nullable')
        return ', '.join(modifiers)

    def _ObjcPropertyNeedsDefaultValueAssignment(self, field):
        kind = field.kind
        typemap = MojoTypemapForKind(kind)
        if not field.default and mojom.IsNullableKind(kind):
            return False
        default = typemap.DefaultObjCValue(field.default)
        if isinstance(typemap, EnumMojoTypemap) and default is None:
            return True # Always give valid first case
        if default is None:
            return False
        if typemap is NumberMojoTypemap and field.default == 0:
            # 0 by default anyways
            return False
        return True

    def _GetObjCPropertyDefaultValue(self, field):
        kind = field.kind
        typemap = MojoTypemapForKind(kind)
        if not field.default and mojom.IsNullableKind(kind):
            return 'nil'
        default = typemap.DefaultObjCValue(field.default)
        if (isinstance(typemap, EnumMojoTypemap) and default is None
            and len(kind.fields) > 0):
            return "%s%s" % (typemap.ObjCWrappedType(),
                             self._ObjCEnumFormatter(kind.fields[0].name))
        return default

    def _GetObjCUnionNullReturnValue(self, kind):
        typemap = MojoTypemapForKind(kind)
        if mojom.IsEnumKind(kind):
            return 'static_cast<%s>(0)' % typemap.ObjCWrappedType()
        if mojom.IsObjectKind(kind) or mojom.IsAnyInterfaceKind(kind):
            return 'nil'
        return '0'

    def _GetObjCWrapperType(self, kind, objectType=False):
        typemap = MojoTypemapForKind(kind, objectType)
        return typemap.ObjCWrappedType()

    def _GetObjCArgumentModifiers(self, kind, inside_callback=False):
        if mojom.IsNullableKind(kind):
            return "nullable" if not inside_callback else "_Nullable"
        return ""

    def _ObjCPropertyFormatter(self, value):
        """ snake case to camel case, and replaces reserved names """
        name = UnderToCamel(value, lower_initial=True)
        # A set of reserved names by Obj-C which shouldn't be used as property
        # names
        reserved = {
            'description': 'desc',
            'debugDescription': 'debugDesc',
            'hash': 'hash_',
            'isProxy': 'proxy',
            'zone': 'zone_',
            'class': 'class_',
            'dealloc': 'dealloc_',
            'finalize': 'finalize_',
            'copy': 'copy_'
        }
        if name in reserved:
            return reserved[name]
        return name

    def _ObjCMethodNameFormatter(self, method):
        name = method.name
        if name.lower().startswith('get'):
            # Obj-C doesn't use the word `get` in getters
            name = name[3:]
        return UnderToCamel(name, lower_initial=True)

    def _ObjCEnumFormatter(self, value):
        """ Formats uppercased, k-prefixed & snake case to upper camel case """
        name = value
        if len(value) >= 2 and value[0] == "k" and value[1].isupper():
            # k-prefixed
            name = value[1:]
        return UnderToCamel(name)

    def _ObjCToCppAssign(self, field, obj=None):
        kind = field.kind
        accessor = "%s%s" % (obj + "." if obj else "",
                             self._ObjCPropertyFormatter(field.name))
        typemap = MojoTypemapForKind(kind)
        if typemap is None:
            raise Exception("No typemap found for the given kind: %s" % kind)
        cpp_assign = typemap.ObjCToCpp(accessor)
        if mojom.IsNullableKind(kind):
            if ((self._IsTypemappedKind(kind)
                 and not self.typemap[self._GetFullMojomNameForKind(
                     kind)]["nullable_is_same_type"])
                    or not isinstance(typemap,
                                      (StructMojoTypemap, UnionMojoTypemap))):
                cpp_assign = "%s ? std::make_optional(%s) : std::nullopt" % (
                    accessor, cpp_assign)
            else:
                cpp_assign = "%s ? %s : nullptr" % (accessor, cpp_assign)
        return cpp_assign

    def _CppToObjCAssign(self, field, prefix=None, suffix=None):
        kind = field.kind
        accessor = "%s%s%s" % (prefix if prefix else "",
                               field.name, (suffix if suffix else ""))
        typemap = MojoTypemapForKind(kind)
        if mojom.IsNullableKind(kind):
            if ((self._IsTypemappedKind(kind)
                 and not self.typemap[self._GetFullMojomNameForKind(
                     kind)]["nullable_is_same_type"])
                    or not isinstance(typemap,
                                      (StructMojoTypemap, UnionMojoTypemap))):
                value_accessor = "%s.value()" % accessor
            else:
                value_accessor = accessor
            return "%s ? %s : nil" % (
                accessor, typemap.CppToObjC(value_accessor))
        return typemap.CppToObjC(accessor)

    def _GetFullMojomNameForKind(self, kind):
        return kind.qualified_name

    def _IsTypemappedKind(self, kind):
        return hasattr(kind, "name") and \
            self._GetFullMojomNameForKind(kind) in self.typemap

    def _ShouldPassParamByValue(self, kind):
        return ((not mojom.IsReferenceKind(kind)) or self._IsMoveOnlyKind(kind)
                or self._IsCopyablePassByValue(kind))

    def _IsCopyablePassByValue(self, kind):
        if not self._IsTypemappedKind(kind):
            return False
        return self.typemap[self._GetFullMojomNameForKind(
            kind)]["copyable_pass_by_value"]

    def _IsMoveOnlyKind(self, kind):
        if self._IsTypemappedKind(kind):
            if mojom.IsEnumKind(kind):
                return False
            return self.typemap[self._GetFullMojomNameForKind(
                kind)]["move_only"]
        if mojom.IsStructKind(kind) or mojom.IsUnionKind(kind):
            return True
        if mojom.IsArrayKind(kind):
            return self._IsMoveOnlyKind(kind.kind)
        if mojom.IsMapKind(kind):
            return (self._IsMoveOnlyKind(kind.value_kind)
                    or self._IsMoveOnlyKind(kind.key_kind))
        if mojom.IsAnyHandleOrInterfaceKind(kind):
            return True
        return False

    def _ConstObjCAssign(self, constant):
        kind = constant.kind
        # Obj-C only supports a handful of constant types
        if mojom.IsStringKind(kind):
            # string constant value come with quotes already
            return '@%s' % constant.value
        if kind in _kind_to_nsnumber_getter:
            return constant.value
        raise Exception(
            "Obj-C constant cannot be generated for the given kind: %s" % kind)

    def _GetJinjaExports(self):
        all_structs = [item for item in
                       self.module.structs if item.name not in
                       self.excludedTypes]
        all_interfaces = [item for item in
                          self.module.interfaces if item.name not in
                          self.excludedTypes]
        all_unions = [item for item in
                      self.module.unions if item.name not in
                      self.excludedTypes]
        all_enums = list(self.module.enums)
        for struct in all_structs:
            all_enums.extend(struct.enums)
            # This allows us to only generate Obj-C++ wrappers for types
            # actually used within other types
            for field in struct.fields:
                if (field.kind.module is not None and
                        field.kind.module.namespace != self.module.namespace):
                    if (mojom.IsStructKind(field.kind) and
                            field.kind.module == self.module):
                        all_structs.append(field.kind)
                        all_enums.extend(field.kind.enums)
                    elif mojom.IsEnumKind(field.kind):
                        all_enums.append(field.kind)

        receivers = set()
        for interface in all_interfaces:
            for method in interface.methods:
                for param in method.parameters:
                    if mojom.IsPendingRemoteKind(param.kind):
                        receivers.add(param.kind.kind)

        # We handle imports from mojo base types with custom typemaps, so only
        # other Brave imports should only be included
        brave_imports = [i for i in self.module.imports if
                         i.path.startswith('brave/')]

        for interface in self.module.interfaces:
            all_enums.extend(interface.enums)
        return {
            "all_enums": all_enums,
            "enums": self.module.enums,
            "imports": brave_imports,
            "interfaces": all_interfaces,
            "interface_bridges": receivers,
            "kinds": self.module.kinds,
            "module": self.module,
            "module_name": os.path.basename(self.module.path),
            "class_prefix": ObjCPrefixFromModule(self.module),
            "structs": all_structs,
            "unions": all_unions,
            "constants": self.module.constants,
            "generate_namespace": self.generateNamespace
        }

    @UseJinja("module.h.tmpl")
    def _GenerateModuleHeader(self):
        return self._GetJinjaExports()

    @UseJinja("module+private.h.tmpl")
    def _GeneratePrivateModuleHeader(self):
        return self._GetJinjaExports()

    @UseJinja("module.mm.tmpl")
    def _GenerateModuleSource(self):
        return self._GetJinjaExports()

    def Write(self, contents, output_dir, filename):
        full_path = os.path.join(output_dir, filename)
        WriteFile(contents, full_path)

    def GenerateFiles(self, output_dir):
        if output_dir is None or len(output_dir) == 0:
            raise Exception("No output directory given to generate files")

        self.module.Stylize(generator.Stylizer())
        name = os.path.basename(self.module.path)

        self.Write(self._GenerateModuleHeader(), output_dir, "%s.objc.h" % name)
        self.Write(self._GeneratePrivateModuleHeader(), output_dir,
                   "%s.objc+private.h" % name)
        self.Write(self._GenerateModuleSource(), output_dir,
                   "%s.objc.mm" % name)
