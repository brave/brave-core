# pylint: disable=import-error,too-many-return-statements,no-self-use,too-many-branches

import os

import mojom.generate.module as mojom
import mojom.generate.generator as generator
from mojom.generate.generator import WriteFile
from mojom.generate.template_expander import UseJinja

class MojoTypemap(object):
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
        return "base::TimeDelta::FromSecondsD(%s.timeIntervalSince1970)" % accessor
    def CppToObjC(self, accessor):
        return "[NSDate dateWithTimeIntervalSince1970:%s.InSecondsF()]" % accessor

class URLMojoTypemap(MojoTypemap):
    @staticmethod
    def IsMojoType(kind):
        return (mojom.IsStructKind(kind) and
                kind.qualified_name == 'url.mojom.Url')
    def ObjCWrappedType(self):
        return "NSURL*"
    def ExpectedCppType(self):
        return "net::GURL"
    def DefaultObjCValue(self, default):
        return "[[NSURL alloc] init]"
    def ObjCToCpp(self, accessor):
        return "net::GURLWithNSURL(%s)" % accessor
    def CppToObjC(self, accessor):
        return "net::NSURLWithGURL(%s)" % accessor

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
        return "NSNumber*" if self.is_inside_container else _kind_to_c_type[self.kind]
    def ExpectedCppType(self):
        return _kind_to_c_type[self.kind]
    def ObjCToCpp(self, accessor):
        if self.is_inside_container:
            return "%s.%s" % (accessor, _kind_to_nsnumber_getter[self.kind])
        return accessor
    def DefaultObjCValue(self, default):
        return default # Default for primatives not needed, array/map default is empty
    def CppToObjC(self, accessor):
        if self.is_inside_container:
            return "@(%s)" % accessor
        return accessor

class ArrayMojoTypemap(MojoTypemap):
    def __init__(self, kind, is_inside_container):
        super(ArrayMojoTypemap, self).__init__(kind, is_inside_container=is_inside_container)
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
        args = (self.wrappedTypemap.ExpectedCppType(),
                self.wrappedTypemap.ObjCWrappedType(),
                accessor, self.wrappedTypemap.ObjCToCpp("obj"))
        return """^{
            std::vector<%s> array;
            for (%s obj in %s) {
                array.push_back(%s);
            }
            return array;
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
        args = (ObjCPrefixFromKind(self.kind), self.kind.name, self.kind.name, accessor)
        return "[[%s%s alloc] initWith%s:*%s]" % args

class EnumMojoTypemap(MojoTypemap):
    @staticmethod
    def IsMojoType(kind):
        return mojom.IsEnumKind(kind)
    def ObjCWrappedType(self):
        return "%s%s" % (ObjCPrefixFromKind(self.kind), self.kind.name)
    def ExpectedCppType(self):
        return "%s::%s" % (CppNamespaceFromKind(self.kind), self.kind.name)
    def DefaultObjCValue(self, default):
        if default is None:
            return None
        return self.CppToObjC("%s::%s" % (self.ExpectedCppType(), default.name))
    def ObjCToCpp(self, accessor):
        return "static_cast<%s>(%s)" % (self.ExpectedCppType(), accessor)
    def CppToObjC(self, accessor):
        return "static_cast<%s>(%s)" % (self.ObjCWrappedType(), accessor)

class DictionaryMojoTypemap(MojoTypemap):
    def __init__(self, kind, is_inside_container):
        super(DictionaryMojoTypemap, self).__init__(kind, is_inside_container=is_inside_container)
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
        args = (self.keyTypemap.ExpectedCppType(), self.valueTypemap.ExpectedCppType(),
                self.keyTypemap.ObjCWrappedType(), accessor,
                self.keyTypemap.ObjCToCpp("key"),
                self.valueTypemap.ObjCToCpp("%s[key]" % accessor))
        return """^{
            base::flat_map<%s, %s> map;
            for (%s key in %s) {
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

class PendingRemoteMojoTypemap(MojoTypemap):
    @staticmethod
    def IsMojoType(kind):
        return mojom.IsPendingRemoteKind(kind)
    def ObjCWrappedType(self):
        return "id<%s%s>" % (ObjCPrefixFromKind(self.kind.kind), self.kind.kind.name)
    def ExpectedCppType(self):
        return "%s::%s" % (CppNamespaceFromKind(self.kind.kind), self.kind.kind.name)
    def DefaultObjCValue(self, default):
        return None
    def ObjCToCpp(self, accessor):
        name = ObjCPrefixFromKind(self.kind.kind) + self.kind.kind.name + "Bridge"
        args = (name, accessor, UnderToLowerCamel(name))
        return """^{
            auto bridge = std::make_unique<%s>(%s);
            auto bridgePtr = bridge.get();
            self->_%sReceivers.push_back(std::move(bridge));
            return bridgePtr->GetRemote();
        }()""" % args
    def CppToObjC(self, accessor):
        return None

_mojo_typemaps = [
    StringMojoTypemap,
    TimeMojoTypemap,
    TimeDeltaMojoTypemap,
    URLMojoTypemap,
    NumberMojoTypemap,
    EnumMojoTypemap,
    ArrayMojoTypemap,
    DictionaryMojoTypemap,
    StructMojoTypemap,
    PendingRemoteMojoTypemap,
]

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
    def __init__(self, *args, **kwargs):
        super(Generator, self).__init__(*args, **kwargs)

    @staticmethod
    def GetTemplatePrefix():
        return "objc_templates"

    def GetFilters(self):
        objc_filters = {
            "objc_property_modifiers": self._GetObjCPropertyModifiers,
            "objc_property_default": self._GetObjCPropertyDefaultValue,
            "objc_property_needs_default_assignment": self._ObjcPropertyNeedsDefaultValueAssignment,
            "objc_wrapper_type": self._GetObjCWrapperType,
            "objc_property_formatter": self._ObjCPropertyFormatter,
            "objc_method_name_formatter": self._ObjCMethodNameFormatter,
            "objc_enum_formatter": self._ObjCEnumFormatter,
            "cpp_to_objc_assign": self._CppToObjCAssign,
            "const_objc_assign": self._ConstObjCAssign,
            "objc_to_cpp_assign": self._ObjCToCppAssign,
            "expected_cpp_param_type": self._GetExpectedCppParamType,
            "cpp_namespace_from_kind": CppNamespaceFromKind,
            "under_to_camel": UnderToCamel,
            "under_to_lower_camel": UnderToLowerCamel,
            "interface_remote_sets": self._GetInterfaceRemoteSets,
        }
        return objc_filters

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
        def is_move_only_kind(kind):
            if mojom.IsStructKind(kind) or mojom.IsUnionKind(kind):
                return True
            if mojom.IsArrayKind(kind):
                return is_move_only_kind(kind.kind)
            if mojom.IsMapKind(kind):
                return (is_move_only_kind(kind.value_kind) or
                        is_move_only_kind(kind.key_kind))
            if mojom.IsAnyHandleOrInterfaceKind(kind):
                return True
            return False
        should_pass_param_by_value = (not mojom.IsReferenceKind(kind)) or is_move_only_kind(kind)
        typemap = MojoTypemapForKind(kind, False)
        typestring = typemap.ExpectedCppType()
        return typestring if should_pass_param_by_value else "const %s&" % typestring

    def _GetObjCPropertyModifiers(self, kind):
        modifiers = ['nonatomic']
        if (mojom.IsArrayKind(kind) or mojom.IsStringKind(kind) or
                mojom.IsMapKind(kind) or mojom.IsStructKind(kind)):
            modifiers.append('copy')
        if mojom.IsNullableKind(kind):
            modifiers.append('nullable')
        return ', '.join(modifiers)

    def _ObjcPropertyNeedsDefaultValueAssignment(self, field):
        kind = field.kind
        typemap = MojoTypemapForKind(kind)
        if not field.default and mojom.IsNullableKind(kind):
            return False
        if typemap.DefaultObjCValue(field.default) is None:
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
        return typemap.DefaultObjCValue(field.default)

    def _GetObjCWrapperType(self, kind, objectType=False):
        typemap = MojoTypemapForKind(kind, objectType)
        return typemap.ObjCWrappedType()

    def _ObjCPropertyFormatter(self, value):
        """ snake case to camel case, and replaces reserved names """
        name = UnderToCamel(value, lower_initial=True)
        # A set of reserved names by Obj-C which shouldn't be used as property names
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
        accessor = "%s%s" % (obj + "." if obj else "", self._ObjCPropertyFormatter(field.name))
        typemap = MojoTypemapForKind(kind)
        if typemap is None:
            raise Exception("No typemap found for the given kind: %s" % kind)
        return typemap.ObjCToCpp(accessor)

    def _CppToObjCAssign(self, field, obj=None):
        kind = field.kind
        accessor = "%s%s" % (obj + "." if obj else "", field.name)
        typemap = MojoTypemapForKind(kind)
        return typemap.CppToObjC(accessor)

    def _ConstObjCAssign(self, constant):
        kind = constant.kind
        # Obj-C only supports a handful of constant types
        if mojom.IsStringKind(kind):
            return '@%s' % constant.value # string constant value come with quotes already
        if kind in _kind_to_nsnumber_getter:
            return constant.value
        raise Exception("Obj-C constant cannot be generated for the given kind: %s" % kind)

    def _GetJinjaExports(self):
        all_structs = [item for item in self.module.structs if item.name not in self.excludedTypes]
        all_interfaces = [item for item in
                          self.module.interfaces if item.name not in self.excludedTypes]
        all_enums = list(self.module.enums)
        for struct in all_structs:
            all_enums.extend(struct.enums)
            # This allows us to only generate Obj-C++ wrappers for types actually used
            # within other types
            for field in struct.fields:
                if (field.kind.module is not None and
                        field.kind.module.namespace != self.module.namespace):
                    if mojom.IsStructKind(field.kind) and field.kind.module == self.module:
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

        for interface in self.module.interfaces:
            all_enums.extend(interface.enums)
        return {
            "all_enums": all_enums,
            "enums": self.module.enums,
            "imports": self.module.imports,
            "interfaces": all_interfaces,
            "interface_bridges": receivers,
            "kinds": self.module.kinds,
            "module": self.module,
            "module_name": os.path.basename(self.module.path),
            "class_prefix": ObjCPrefixFromModule(self.module),
            "structs": all_structs,
            "unions": self.module.unions,
            "constants": self.module.constants
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
        self.Write(self._GenerateModuleSource(), output_dir, "%s.objc.mm" % name)
