import os
import mojom.generate.module as mojom
import mojom.generate.generator as generator
from mojom.generate.generator import WriteFile
from mojom.generate.template_expander import UseJinja

_kind_to_objc_type = {
  mojom.BOOL:   "bool",
  mojom.INT8:   "int8_t",
  mojom.UINT8:  "uint8_t",
  mojom.INT16:  "int16_t",
  mojom.UINT16: "uint16_t",
  mojom.INT32:  "int32_t",
  mojom.UINT32: "uint32_t",
  mojom.FLOAT:  "float",
  mojom.INT64:  "int64_t",
  mojom.UINT64: "uint64_t",
  mojom.DOUBLE: "double",
}

_kind_to_nsnumber_getter = {
  mojom.BOOL:   "boolValue",
  mojom.INT8:   "charValue",
  mojom.UINT8:  "unsignedCharValue",
  mojom.INT16:  "shortValue",
  mojom.UINT16: "unsignedShortValue",
  mojom.INT32:  "intValue",
  mojom.UINT32: "unsignedIntValue",
  mojom.FLOAT:  "floatValue",
  mojom.INT64:  "longLongValue",
  mojom.UINT64: "unsignedLongLongValue",
  mojom.DOUBLE: "doubleValue",
};

class Generator(generator.Generator):
  def __init__(self, *args, **kwargs):
    super(Generator, self).__init__(*args, **kwargs)
    self.class_prefix = "BAT"

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
      "objc_enum_formatter": self._ObjCEnumFormatter,
      "cpp_to_objc_assign": self._CppToObjCAssign,
      "objc_to_cpp_assign": self._ObjCToCppAssign,
      "swift_enum_name_formatter": self._SwiftEnumNameFormatter,
    }
    return objc_filters

  def _GetObjCPropertyModifiers(self, kind):
    modifiers = ['nonatomic']
    if (mojom.IsArrayKind(kind) or mojom.IsStringKind(kind) or
       mojom.IsMapKind(kind) or self._IsMojomStruct(kind)):
      modifiers.append('copy')
    if mojom.IsNullableKind(kind):
      modifiers.append('nullable')
    return ', '.join(modifiers)

  def _ObjcPropertyNeedsDefaultValueAssignment(self, field):
    kind = field.kind
    if not field.default:
      # If there's no specified default, only make defaults for required types
      return (not mojom.IsNullableKind(kind) and
             (mojom.IsStringKind(kind) or mojom.IsArrayKind(kind) or
              mojom.IsMapKind(kind) or self._IsMojomStruct(kind)))

    if self._IsObjCNumberKind(kind) and field.default == 0:
      # 0 by default anyways
      return False

    return True

  def _GetObjCPropertyDefaultValue(self, field):
    kind = field.kind
    if mojom.IsNullableKind(kind) and not field.default:
      return 'nil'
    if self._IsObjCNumberKind(kind):
      return '0' if not field.default else field.default
    if mojom.IsEnumKind(kind):
      value = '0' if not field.default else field.default.field.value
      return 'static_cast<%s%s>(%s)' % (self.class_prefix, kind.name, value)
    if mojom.IsStringKind(kind):
      return '@""' if not field.default else '@%s' % field.default
    if mojom.IsArrayKind(kind):
      return '@[]'
    if mojom.IsMapKind(kind):
      return '@{}'
    if self._IsMojomStruct(kind):
      return "[[%s%s alloc] init]" % (self.class_prefix, kind.name)
    raise Exception("Unrecognized kind %s" % kind.spec)

  def _IsMojomStruct(self, kind):
    """ Whether or not a kind is a struct that has an Obj-C++ counterpart generated """
    if not mojom.IsStructKind(kind):
      return False
    return kind.mojom_name in map(lambda s: s.mojom_name, self.module.structs)

  def _GetObjCWrapperType(self, kind, objectType = False):
    if self._IsMojomStruct(kind):
      return "%s%s *" % (self.class_prefix, kind.name)
    if mojom.IsEnumKind(kind):
      return "%s%s" % (self.class_prefix, kind.name)
    if mojom.IsInterfaceKind(kind):
      return "id<%s%s>" % (self.class_prefix, kind.name)
    if mojom.IsStringKind(kind):
      return "NSString *"
    if mojom.IsArrayKind(kind):
      return "NSArray<%s> *" % self._GetObjCWrapperType(kind.kind, True)
    if mojom.IsMapKind(kind):
      return "NSDictionary<%s, %s> *" % \
      (self._GetObjCWrapperType(kind.key_kind, True),
       self._GetObjCWrapperType(kind.value_kind, True))
    if self._IsObjCNumberKind(kind):
      if objectType:
        return "NSNumber *"
      else:
        return _kind_to_objc_type[kind]
    raise Exception("Unrecognized kind %s" % kind)

  def _ObjCPropertyFormatter(self, str):
    """ snake case to camel case, and replaces reserved names """
    parts = str.split('_')
    name = parts[0].lower() + ''.join(map(lambda p: p.capitalize(), parts[1:]))
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

  def _SwiftEnumNameFormatter(self, enum):
    name = enum.name
    # A set of reserved names that would conflict with commonly used standard Swift types
    reserved = [
      # Swift
      'Character',
      'Collection',
      'Result',
      # SwiftUI
      'Environment',
      # Foundation
      'AffineTransform',
      'Array',
      'Calendar',
      'CharacterSet',
      'Data',
      'DateComponents',
      'DateInterval',
      'Date',
      'Decimal',
      'Dictionary',
      'IndexPath',
      'IndexSet',
      'Locale',
      'Measurement',
      'Notification',
      'PersonNameComponents',
      'Set',
      'String',
      'TimeZone',
      'URL',
      'URLComponents',
      'URLQueryItem',
      'URLRequest',
      'UUID',
    ]
    if name in reserved:
      # Keep them prefixed in Swift
      return "%s%s" % (self.class_prefix, name)
    return name

  def _ObjCEnumFormatter(self, str):
    """ Formats uppercased, k-prefixed & snake case to upper camel case """
    name = str
    if len(str) >= 2 and str[0] == "k" and str[1].isupper():
      # k-prefixed is already cased correctly
      return str[1:]
    return ''.join(map(lambda p: p.capitalize(), name.split('_')))

  def _IsObjCNumberKind(self, kind):
    return kind in _kind_to_nsnumber_getter

  def _CppPtrToObjCTransform(self, kind, obj):
    args = (self.class_prefix, kind.name, kind.name, obj)
    return "[[%s%s alloc] initWith%s:*%s]" % args

  def _ObjCToCppAssign(self, field, obj):
    kind = field.kind
    accessor = "%s.%s" % (obj, self._ObjCPropertyFormatter(field.name))

    if self._IsObjCNumberKind(kind):
      return accessor
    if self._IsMojomStruct(kind):
      if mojom.IsNullableKind(kind):
        return "%s != nil ? %s.cppObjPtr : nullptr" % (accessor, accessor)
      else:
        return "%s.cppObjPtr" % accessor
    if mojom.IsEnumKind(kind):
      return "static_cast<%s::%s>(%s)" % (self._CppNamespace(), kind.name, accessor)
    if mojom.IsStringKind(kind):
      return "%s.UTF8String" % accessor
    if mojom.IsArrayKind(kind):
      # Only currently supporting: `[string]`, `[number]`, and `[struct]`
      array_kind = kind.kind
      if mojom.IsStringKind(array_kind):
        return "VectorFromNSArray(%s)" % accessor
      if self._IsObjCNumberKind(array_kind):
        return """^{
          std::vector<%s> array;
          for (NSNumber *number in %s) {
            array.push_back(number.%s);
          }
          return array;
        }()""" % (_kind_to_objc_type[array_kind], accessor, _kind_to_nsnumber_getter[array_kind])
      elif mojom.IsStringKind(array_kind):
        return """^{
          std::vector<std::string> array;
          for (NSString *string in %s) {
            array.push_back(string.UTF8String);
          }
          return array;
        }()""" % accessor
      elif self._IsMojomStruct(array_kind):
        return """^{
            std::vector<%s::%sPtr> array;
            for (%s%s *obj in %s) {
              array.push_back(obj.cppObjPtr);
            }
            return array;
          }()""" % (self._CppNamespace(), array_kind.name, self.class_prefix, array_kind.name, accessor)
      else:
        raise Exception("Unsupported array kind %s" % array_kind.spec)
    if mojom.IsMapKind(kind):
      # Only currently supporting: `{string: string}`, `{string: number}`, and
      # `{string: struct}`
      key_kind = kind.key_kind
      val_kind = kind.value_kind
      if mojom.IsStringKind(key_kind):
        if self._IsObjCNumberKind(val_kind):
          return """^{
            base::flat_map<std::string, %s> map;
            for (NSString *key in %s) {
              map[key.UTF8String] = %s[key].%s;
            }
            return map;
          }()""" % (_kind_to_objc_type[val_kind], accessor, accessor, _kind_to_nsnumber_getter[val_kind])
        elif mojom.IsStringKind(val_kind):
          return """^{
            base::flat_map<std::string, std::string> map;
            for (NSString *key in %s) {
              map[key.UTF8String] = %s[key].UTF8String;
            }
            return map;
          }()""" % (accessor, accessor)
        elif self._IsMojomStruct(val_kind):
          return """^{
            base::flat_map<std::string, %s::%sPtr> map;
            for (NSString *key in %s) {
              map[key.UTF8String] = %s[key].cppObjPtr;
            }
            return map;
          }()""" % (self._CppNamespace(), val_kind.name, accessor, accessor)
        else:
          raise Exception("Unsupported dictionary value kind %s" % val_kind.spec)
      else:
        raise Exception("Unsupported dictionary key kind %s" % key_kind.spec)
    return "%s" % accessor

  def _CppToObjCAssign(self, field, obj):
    kind = field.kind
    accessor = "%s.%s" % (obj, field.name)
    if self._IsObjCNumberKind(kind):
      return accessor
    if self._IsMojomStruct(kind):
      args = (self.class_prefix, kind.name, kind.name, accessor)
      base = "[[%s%s alloc] initWith%s:*%s]" % args
      if mojom.IsNullableKind(kind):
        return """^%s%s *{
          if (%s.get() != nullptr) {
            return %s;
          }
          return nil;
        }()""" % (self.class_prefix, kind.name, accessor, base)
      else:
        return base
    if mojom.IsEnumKind(kind):
      return "static_cast<%s%s>(%s)" % (self.class_prefix, kind.name, accessor)
    if mojom.IsStringKind(kind):
      return "[NSString stringWithUTF8String:%s.c_str()]" % accessor
    if mojom.IsArrayKind(kind):
      # Only currently supporting: `[string]`, `[number]`, and `[struct]`
      array_kind = kind.kind
      if mojom.IsStringKind(array_kind) or self._IsObjCNumberKind(array_kind):
        return "NSArrayFromVector(%s)" % accessor
      elif self._IsMojomStruct(array_kind):
        # Mojo IDL array<struct> actually creates a
        # `std::vector<mojom::StructPtr<struct>>``, instead of just a plain
        # vector of `struct`, which needs to be handled appropriately as
        # `StructPtr` does not allow copy and assign, and acts like a unique_ptr
        return """^{
          const auto a = [NSMutableArray new];
          for (const auto& o : %s) {
            [a addObject:%s];
          }
          return a;
        }()""" % (accessor, self._CppPtrToObjCTransform(array_kind, 'o'))
      else:
        raise Exception("Unsupported array kind %s" % array_kind.spec)
    if mojom.IsMapKind(kind):
      # Only currently supporting: `{string: string}`, `{string: number}`, and
      # `{string: struct}`
      key_kind = kind.key_kind
      val_kind = kind.value_kind
      if mojom.IsStringKind(key_kind):
        if mojom.IsStringKind(val_kind) or self._IsObjCNumberKind(val_kind):
          return "NSDictionaryFromMap(%s)" % accessor
        elif self._IsMojomStruct(val_kind):
          # Mojo IDL map<*, struct> actually creates a
          # `base::flat_map<*, mojom::StructPtr<struct>>``, instead of just a
          # plain std::map of `struct`, which needs to be handled appropriately
          # as `StructPtr` does not allow copy and assign, and acts like a
          # unique_ptr
          return """^{
            const auto d = [NSMutableDictionary new];
            for (const auto& item : %s) {
              d[[NSString stringWithUTF8String:item.first.c_str()]] = %s;
            }
            return d;
          }()""" % (accessor, self._CppPtrToObjCTransform(val_kind, 'item.second'))
        else:
          raise Exception("Unsupported dictionary value kind %s" % val_kind.spec)
      else:
        raise Exception("Unsupported dictionary key kind %s" % key_kind.spec)

    return "%s" % accessor

  def _CppNamespace(self):
    return str(self.module.namespace).replace(".", "::")

  def _GetJinjaExports(self):
    all_enums = list(self.module.enums)
    for struct in self.module.structs:
      all_enums.extend(struct.enums)
    for interface in self.module.interfaces:
      all_enums.extend(interface.enums)
    return {
      "all_enums": all_enums,
      "enums": self.module.enums,
      "imports": self.module.imports,
      "interfaces": self.module.interfaces,
      "kinds": self.module.kinds,
      "module": self.module,
      "module_name": os.path.basename(self.module.path),
      "module_include_path": self.module_include_path,
      "cpp_namespace": self._CppNamespace(),
      "class_prefix": self.class_prefix,
      # "namespaces_as_array": NamespaceToArray(self.module.namespace),
      "structs": self.module.structs,
      "unions": self.module.unions,
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

  def Write(self, contents, filename):
    if self.output_dir is None:
      print(contents)
      return
    full_path = os.path.join(self.output_dir, filename)
    WriteFile(contents, full_path)

  def GenerateFiles(self, output_dir = None):
    self.output_dir = output_dir
    self.module.Stylize(generator.Stylizer())
    name = os.path.basename(self.module.path)

    self.Write(self._GenerateModuleHeader(),
                  "%s.objc.h" % name)
    self.Write(self._GeneratePrivateModuleHeader(),
                  "%s.objc+private.h" % name)
    self.Write(self._GenerateModuleSource(),
                  "%s.objc.mm" % name)
