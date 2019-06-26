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
      "objc_wrapper_type": self._GetObjCWrapperType,
      "objc_property_formatter": self._ObjCPropertyFormatter,
      "objc_enum_formatter": self._ObjCEnumFormatter,
      "cpp_to_objc_assign": self._CppToObjCAssign,
    }
    return objc_filters

  def _GetObjCPropertyModifiers(self, kind):
    modifiers = ['nonatomic']
    if (mojom.IsArrayKind(kind) or mojom.IsStringKind(kind) or
       mojom.IsMapKind(kind)):
      modifiers.append('copy')
    if mojom.IsNullableKind(kind):
      modifiers.append('nullable')
    return ', '.join(modifiers)

  def _GetObjCWrapperType(self, kind, objectType = False):
    if kind in self.module.structs:
      return "%s%s *" % (self.class_prefix, kind.name)
    if mojom.IsEnumKind(kind):
      return "%s%s" % (self.class_prefix, kind.name)
    if mojom.IsInterfaceKind(kind):
      return "id<%s%s>" % (self.class_prefix, kind.name)
    if mojom.IsStringKind(kind):
      return "NSString *";
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
    raise Exception("Unrecognized kind %s" % kind.spec)

  def _ObjCPropertyFormatter(self, str):
    """ snake case to camel case """
    parts = str.split('_')
    return parts[0].lower() + ''.join(map(lambda p: p.capitalize(), parts[1:]))

  def _ObjCEnumFormatter(self, str):
    """ uppercased & snake case to upper camel case """
    return ''.join(map(lambda p: p.capitalize(), str.split('_')))

  def _IsObjCNumberKind(self, kind):
    return (mojom.IsIntegralKind(kind) or mojom.IsDoubleKind(kind) or
            mojom.IsFloatKind(kind))

  def _CppPtrToObjCTransform(self, kind, obj):
    args = (self.class_prefix, kind.name, kind.name, obj)
    return "[[%s%s alloc] initWith%s:*%s]" % args

  def _CppToObjCAssign(self, field, obj):
    kind = field.kind
    accessor = "%s.%s" % (obj, field.name)
    if self._IsObjCNumberKind(kind):
      return accessor
    if kind in self.module.structs:
      args = (self.class_prefix, kind.name, kind.name, accessor)
      return "[[%s%s alloc] initWith%s:*%s]" % args
    if mojom.IsEnumKind(kind):
      return "static_cast<%s%s>(%s)" % (self.class_prefix, kind.name, accessor)
    if mojom.IsStringKind(kind):
      return "[NSString stringWithUTF8String:%s.c_str()]" % accessor
    if mojom.IsArrayKind(kind):
      # Only currently supporting: `[string]`, `[number]`, and `[struct]`
      array_kind = kind.kind
      if mojom.IsStringKind(array_kind) or self._IsObjCNumberKind(array_kind):
        return "NSArrayFromVector(%s)" % accessor
      elif array_kind in self.module.structs:
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
        elif val_kind in self.module.structs:
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

  def _GetJinjaExports(self):
    all_enums = list(self.module.enums)
    for struct in self.module.structs:
      all_enums.extend(struct.enums)
    for interface in self.module.interfaces:
      all_enums.extend(interface.enums)
    cpp_namespace = str(self.module.namespace).replace(".", "::")
    return {
      "all_enums": all_enums,
      "enums": self.module.enums,
      "imports": self.module.imports,
      "interfaces": self.module.interfaces,
      "kinds": self.module.kinds,
      "module": self.module,
      "cpp_namespace": cpp_namespace,
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
