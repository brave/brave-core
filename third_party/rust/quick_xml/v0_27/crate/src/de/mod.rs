//! Serde `Deserializer` module.
//!
//! Due to the complexity of the XML standard and the fact that serde was developed
//! with JSON in mind, not all serde concepts apply smoothly to XML. This leads to
//! that fact that some XML concepts are inexpressible in terms of serde derives
//! and may require manual deserialization.
//!
//! The most notable restriction is the ability to distinguish between _elements_
//! and _attributes_, as no other format used by serde has such a conception.
//!
//! Due to that the mapping is performed in a best effort manner.
//!
//!
//!
//! Table of Contents
//! =================
//! - [Mapping XML to Rust types](#mapping-xml-to-rust-types)
//!   - [Optional attributes and elements](#optional-attributes-and-elements)
//!   - [Choices (`xs:choice` XML Schema type)](#choices-xschoice-xml-schema-type)
//!   - [Sequences (`xs:all` and `xs:sequence` XML Schema types)](#sequences-xsall-and-xssequence-xml-schema-types)
//! - [Composition Rules](#composition-rules)
//! - [Difference between `$text` and `$value` special names](#difference-between-text-and-value-special-names)
//!   - [`$text`](#text)
//!   - [`$value`](#value)
//!     - [Primitives and sequences of primitives](#primitives-and-sequences-of-primitives)
//!     - [Structs and sequences of structs](#structs-and-sequences-of-structs)
//!     - [Enums and sequences of enums](#enums-and-sequences-of-enums)
//! - [Frequently Used Patterns](#frequently-used-patterns)
//!   - [`<element>` lists](#element-lists)
//!
//!
//!
//! Mapping XML to Rust types
//! =========================
//!
//! Type names are never considered when deserializing, so you can name your
//! types as you wish. Other general rules:
//! - `struct` field name could be represented in XML only as an attribute name
//!   or an element name;
//! - `enum` variant name could be represented in XML only as an attribute name
//!   or an element name;
//! - the unit struct, unit type `()` and unit enum variant can be deserialized
//!   from any valid XML content:
//!   - attribute and element names;
//!   - attribute and element values;
//!   - text or CDATA content (including mixed text and CDATA content).
//!
//! <div style="background:rgba(120,145,255,0.45);padding:0.75em;">
//!
//! NOTE: examples, marked with `FIXME:` does not work yet -- any PRs that fixes
//! that are welcome! The message after marker is a test failure message.
//! Also, all that tests are marked with an `ignore` option, although their
//! compiles. This is by intention, because rustdoc marks such blocks with
//! an information icon unlike `no_run` blocks.
//!
//! </div>
//!
//! <table>
//! <thead>
//! <tr><th>To parse all these XML's...</th><th>...use that Rust type(s)</th></tr>
//! </thead>
//! <tbody style="vertical-align:top;">
//! <tr>
//! <td>
//! Content of attributes and text / CDATA content of elements (including mixed
//! text and CDATA content):
//!
//! ```xml
//! <... ...="content" />
//! ```
//! ```xml
//! <...>content</...>
//! ```
//! ```xml
//! <...><![CDATA[content]]></...>
//! ```
//! ```xml
//! <...>text<![CDATA[cdata]]>text</...>
//! ```
//! <div style="background:rgba(80, 240, 100, 0.20);padding:0.75em;">
//!
//! Merging of the text / CDATA content is tracked in the issue [#474] and
//! will be available in the next release.
//! </div>
//! </td>
//! <td>
//!
//! You can use any type that can be deserialized from an `&str`, for example:
//! - [`String`] and [`&str`]
//! - [`Cow<str>`]
//! - [`u32`], [`f32`] and other numeric types
//! - `enum`s, like
//!   ```ignore
//!   // FIXME: #474, merging mixed text / CDATA
//!   // content does not work yet
//!   # use pretty_assertions::assert_eq;
//!   # use serde::Deserialize;
//!   # #[derive(Debug, PartialEq)]
//!   #[derive(Deserialize)]
//!   enum Language {
//!     Rust,
//!     Cpp,
//!     #[serde(other)]
//!     Other,
//!   }
//!   # #[derive(Debug, PartialEq, Deserialize)]
//!   # struct X { #[serde(rename = "$text")] x: Language }
//!   # assert_eq!(X { x: Language::Rust  }, quick_xml::de::from_str("<x>Rust</x>").unwrap());
//!   # assert_eq!(X { x: Language::Cpp   }, quick_xml::de::from_str("<x>C<![CDATA[p]]>p</x>").unwrap());
//!   # assert_eq!(X { x: Language::Other }, quick_xml::de::from_str("<x><![CDATA[other]]></x>").unwrap());
//!   ```
//!
//! <div style="background:rgba(120,145,255,0.45);padding:0.75em;">
//!
//! NOTE: deserialization to non-owned types (i.e. borrow from the input),
//! such as `&str`, is possible only if you parse document in the UTF-8
//! encoding and content does not contain entity references such as `&amp;`,
//! or character references such as `&#xD;`, as well as text content represented
//! by one piece of [text] or [CDATA] element.
//! </div>
//! <!-- TODO: document an error type returned -->
//!
//! [text]: Event::Text
//! [CDATA]: Event::CData
//! </td>
//! </tr>
//! <!-- 2 ===================================================================================== -->
//! <tr>
//! <td>
//!
//! Content of attributes and text / CDATA content of elements (including mixed
//! text and CDATA content), which represents a space-delimited lists, as
//! specified in the XML Schema specification for [`xs:list`] `simpleType`:
//!
//! ```xml
//! <... ...="element1 element2 ..." />
//! ```
//! ```xml
//! <...>
//!   element1
//!   element2
//!   ...
//! </...>
//! ```
//! ```xml
//! <...><![CDATA[
//!   element1
//!   element2
//!   ...
//! ]]></...>
//! ```
//! <div style="background:rgba(80, 240, 100, 0.20);padding:0.75em;">
//!
//! Merging of the text / CDATA content is tracked in the issue [#474] and
//! will be available in the next release.
//! </div>
//!
//! [`xs:list`]: https://www.w3.org/TR/xmlschema11-2/#list-datatypes
//! </td>
//! <td>
//!
//! Use any type that deserialized using [`deserialize_seq()`] call, for example:
//!
//! ```
//! // FIXME: #474, merging mixed text / CDATA
//! // content does not work yet
//! type List = Vec<u32>;
//! ```
//!
//! See the next row to learn where in your struct definition you should
//! use that type.
//!
//! According to the XML Schema specification, delimiters for elements is one
//! or more space (`' '`, `'\r'`, `'\n'`, and `'\t'`) character(s).
//!
//! <div style="background:rgba(120,145,255,0.45);padding:0.75em;">
//!
//! NOTE: according to the XML Schema restrictions, you cannot escape those
//! white-space characters, so list elements will _never_ contain them.
//! In practice you will usually use `xs:list`s for lists of numbers or enumerated
//! values which looks like identifiers in many languages, for example, `item`,
//! `some_item` or `some-item`, so that shouldn't be a problem.
//!
//! NOTE: according to the XML Schema specification, list elements can be
//! delimited only by spaces. Other delimiters (for example, commas) are not
//! allowed.
//!
//! </div>
//!
//! [`deserialize_seq()`]: de::Deserializer::deserialize_seq
//! </td>
//! </tr>
//! <!-- 3 ===================================================================================== -->
//! <tr>
//! <td>
//! A typical XML with attributes. The root tag name does not matter:
//!
//! ```xml
//! <any-tag one="..." two="..."/>
//! ```
//! </td>
//! <td>
//!
//! A structure where each XML attribute is mapped to a field with a name
//! starting with `@`. Because Rust identifiers do not permit the `@` character,
//! you should use the `#[serde(rename = "@...")]` attribute to rename it.
//! The name of the struct itself does not matter:
//!
//! ```
//! # use serde::Deserialize;
//! # type T = ();
//! # type U = ();
//! // Get both attributes
//! # #[derive(Debug, PartialEq)]
//! #[derive(Deserialize)]
//! struct AnyName {
//!   #[serde(rename = "@one")]
//!   one: T,
//!
//!   #[serde(rename = "@two")]
//!   two: U,
//! }
//! # quick_xml::de::from_str::<AnyName>(r#"<any-tag one="..." two="..."/>"#).unwrap();
//! ```
//! ```
//! # use serde::Deserialize;
//! # type T = ();
//! // Get only the one attribute, ignore the other
//! # #[derive(Debug, PartialEq)]
//! #[derive(Deserialize)]
//! struct AnyName {
//!   #[serde(rename = "@one")]
//!   one: T,
//! }
//! # quick_xml::de::from_str::<AnyName>(r#"<any-tag one="..." two="..."/>"#).unwrap();
//! # quick_xml::de::from_str::<AnyName>(r#"<any-tag one="..."/>"#).unwrap();
//! # quick_xml::de::from_str::<AnyName>(r#"<any-tag one="..."><one>...</one></any-tag>"#).unwrap();
//! ```
//! ```
//! # use serde::Deserialize;
//! // Ignore all attributes
//! // You can also use the `()` type (unit type)
//! # #[derive(Debug, PartialEq)]
//! #[derive(Deserialize)]
//! struct AnyName;
//! # quick_xml::de::from_str::<AnyName>(r#"<any-tag one="..." two="..."/>"#).unwrap();
//! # quick_xml::de::from_str::<AnyName>(r#"<any-tag one="..."><one>...</one></any-tag>"#).unwrap();
//! # quick_xml::de::from_str::<AnyName>(r#"<any-tag><one>...</one><two>...</two></any-tag>"#).unwrap();
//! ```
//!
//! All these structs can be used to deserialize from an XML on the
//! left side depending on amount of information that you want to get.
//! Of course, you can combine them with elements extractor structs (see below).
//!
//! <div style="background:rgba(120,145,255,0.45);padding:0.75em;">
//!
//! NOTE: XML allows you to have an attribute and an element with the same name
//! inside the one element. quick-xml deals with that by prepending a `@` prefix
//! to the name of attributes.
//! </div>
//! </td>
//! </tr>
//! <!-- 4 ===================================================================================== -->
//! <tr>
//! <td>
//! A typical XML with child elements. The root tag name does not matter:
//!
//! ```xml
//! <any-tag>
//!   <one>...</one>
//!   <two>...</two>
//! </any-tag>
//! ```
//! </td>
//! <td>
//! A structure where an each XML child element are mapped to the field.
//! Each element name becomes a name of field. The name of the struct itself
//! does not matter:
//!
//! ```
//! # use serde::Deserialize;
//! # type T = ();
//! # type U = ();
//! // Get both elements
//! # #[derive(Debug, PartialEq)]
//! #[derive(Deserialize)]
//! struct AnyName {
//!   one: T,
//!   two: U,
//! }
//! # quick_xml::de::from_str::<AnyName>(r#"<any-tag><one>...</one><two>...</two></any-tag>"#).unwrap();
//! #
//! # quick_xml::de::from_str::<AnyName>(r#"<any-tag one="..." two="..."/>"#).unwrap_err();
//! # quick_xml::de::from_str::<AnyName>(r#"<any-tag one="..."><two>...</two></any-tag>"#).unwrap_err();
//! ```
//! ```
//! # use serde::Deserialize;
//! # type T = ();
//! // Get only the one element, ignore the other
//! # #[derive(Debug, PartialEq)]
//! #[derive(Deserialize)]
//! struct AnyName {
//!   one: T,
//! }
//! # quick_xml::de::from_str::<AnyName>(r#"<any-tag><one>...</one><two>...</two></any-tag>"#).unwrap();
//! # quick_xml::de::from_str::<AnyName>(r#"<any-tag one="..."><one>...</one></any-tag>"#).unwrap();
//! ```
//! ```
//! # use serde::Deserialize;
//! // Ignore all elements
//! // You can also use the `()` type (unit type)
//! # #[derive(Debug, PartialEq)]
//! #[derive(Deserialize)]
//! struct AnyName;
//! # quick_xml::de::from_str::<AnyName>(r#"<any-tag one="..." two="..."/>"#).unwrap();
//! # quick_xml::de::from_str::<AnyName>(r#"<any-tag><one>...</one><two>...</two></any-tag>"#).unwrap();
//! # quick_xml::de::from_str::<AnyName>(r#"<any-tag one="..."><two>...</two></any-tag>"#).unwrap();
//! # quick_xml::de::from_str::<AnyName>(r#"<any-tag one="..."><one>...</one></any-tag>"#).unwrap();
//! ```
//!
//! All these structs can be used to deserialize from an XML on the
//! left side depending on amount of information that you want to get.
//! Of course, you can combine them with attributes extractor structs (see above).
//!
//! <div style="background:rgba(120,145,255,0.45);padding:0.75em;">
//!
//! NOTE: XML allows you to have an attribute and an element with the same name
//! inside the one element. quick-xml deals with that by prepending a `@` prefix
//! to the name of attributes.
//! </div>
//! </td>
//! </tr>
//! <!-- 5 ===================================================================================== -->
//! <tr>
//! <td>
//! An XML with an attribute and a child element named equally:
//!
//! ```xml
//! <any-tag field="...">
//!   <field>...</field>
//! </any-tag>
//! ```
//! </td>
//! <td>
//!
//! You MUST specify `#[serde(rename = "@field")]` on a field that will be used
//! for an attribute:
//!
//! ```
//! # use pretty_assertions::assert_eq;
//! # use serde::Deserialize;
//! # type T = ();
//! # type U = ();
//! # #[derive(Debug, PartialEq)]
//! #[derive(Deserialize)]
//! struct AnyName {
//!   #[serde(rename = "@field")]
//!   attribute: T,
//!   field: U,
//! }
//! # assert_eq!(
//! #   AnyName { attribute: (), field: () },
//! #   quick_xml::de::from_str(r#"
//! #     <any-tag field="...">
//! #       <field>...</field>
//! #     </any-tag>
//! #   "#).unwrap(),
//! # );
//! ```
//! </td>
//! </tr>
//! <!-- ======================================================================================= -->
//! <tr><th colspan="2">
//!
//! ## Optional attributes and elements
//!
//! </th></tr>
//! <tr><th>To parse all these XML's...</th><th>...use that Rust type(s)</th></tr>
//! <!-- 6 ===================================================================================== -->
//! <tr>
//! <td>
//! An optional XML attribute that you want to capture.
//! The root tag name does not matter:
//!
//! ```xml
//! <any-tag optional="..."/>
//! ```
//! ```xml
//! <any-tag/>
//! ```
//! </td>
//! <td>
//!
//! A structure with an optional field, renamed according to the requirements
//! for attributes:
//!
//! ```
//! # use pretty_assertions::assert_eq;
//! # use serde::Deserialize;
//! # type T = ();
//! # #[derive(Debug, PartialEq)]
//! #[derive(Deserialize)]
//! struct AnyName {
//!   #[serde(rename = "@optional")]
//!   optional: Option<T>,
//! }
//! # assert_eq!(AnyName { optional: Some(()) }, quick_xml::de::from_str(r#"<any-tag optional="..."/>"#).unwrap());
//! # assert_eq!(AnyName { optional: None     }, quick_xml::de::from_str(r#"<any-tag/>"#).unwrap());
//! ```
//! When the XML attribute is present, type `T` will be deserialized from
//! an attribute value (which is a string). Note, that if `T = String` or other
//! string type, the empty attribute is mapped to a `Some("")`, whereas `None`
//! represents the missed attribute:
//! ```xml
//! <any-tag optional="..."/><!-- Some("...") -->
//! <any-tag optional=""/>   <!-- Some("") -->
//! <any-tag/>               <!-- None -->
//! ```
//! </td>
//! </tr>
//! <!-- 7 ===================================================================================== -->
//! <tr>
//! <td>
//! An optional XML elements that you want to capture.
//! The root tag name does not matter:
//!
//! ```xml
//! <any-tag/>
//!   <optional>...</optional>
//! </any-tag>
//! ```
//! ```xml
//! <any-tag/>
//!   <optional/>
//! </any-tag>
//! ```
//! ```xml
//! <any-tag/>
//! ```
//! </td>
//! <td>
//!
//! A structure with an optional field:
//!
//! ```
//! # use pretty_assertions::assert_eq;
//! # use serde::Deserialize;
//! # type T = ();
//! # #[derive(Debug, PartialEq)]
//! #[derive(Deserialize)]
//! struct AnyName {
//!   optional: Option<T>,
//! }
//! # assert_eq!(AnyName { optional: Some(()) }, quick_xml::de::from_str(r#"<any-tag><optional>...</optional></any-tag>"#).unwrap());
//! # assert_eq!(AnyName { optional: None     }, quick_xml::de::from_str(r#"<any-tag/>"#).unwrap());
//! ```
//! When the XML element is present, type `T` will be deserialized from an
//! element (which is a string or a multi-mapping -- i.e. mapping which can have
//! duplicated keys).
//! <div style="background:rgba(80, 240, 100, 0.20);padding:0.75em;">
//!
//! Currently some edge cases exists described in the issue [#497].
//! </div>
//! </td>
//! </tr>
//! <!-- ======================================================================================= -->
//! <tr><th colspan="2">
//!
//! ## Choices (`xs:choice` XML Schema type)
//!
//! </th></tr>
//! <tr><th>To parse all these XML's...</th><th>...use that Rust type(s)</th></tr>
//! <!-- 8 ===================================================================================== -->
//! <tr>
//! <td>
//! An XML with different root tag names:
//!
//! ```xml
//! <one field1="...">...</one>
//! ```
//! ```xml
//! <two>
//!   <field2>...</field2>
//! </two>
//! ```
//! </td>
//! <td>
//!
//! An enum where each variant have a name of the possible root tag. The name of
//! the enum itself does not matter.
//!
//! All these structs can be used to deserialize from any XML on the
//! left side depending on amount of information that you want to get:
//!
//! ```
//! # use pretty_assertions::assert_eq;
//! # use serde::Deserialize;
//! # type T = ();
//! # type U = ();
//! # #[derive(Debug, PartialEq)]
//! #[derive(Deserialize)]
//! #[serde(rename_all = "snake_case")]
//! enum AnyName {
//!   One { #[serde(rename = "@field1")] field1: T },
//!   Two { field2: U },
//! }
//! # assert_eq!(AnyName::One { field1: () }, quick_xml::de::from_str(r#"<one field1="...">...</one>"#).unwrap());
//! # assert_eq!(AnyName::Two { field2: () }, quick_xml::de::from_str(r#"<two><field2>...</field2></two>"#).unwrap());
//! ```
//! ```
//! # use pretty_assertions::assert_eq;
//! # use serde::Deserialize;
//! # type T = ();
//! # #[derive(Debug, PartialEq)]
//! #[derive(Deserialize)]
//! struct Two {
//!   field2: T,
//! }
//! # #[derive(Debug, PartialEq)]
//! #[derive(Deserialize)]
//! #[serde(rename_all = "snake_case")]
//! enum AnyName {
//!   // `field1` content discarded
//!   One,
//!   Two(Two),
//! }
//! # assert_eq!(AnyName::One,                     quick_xml::de::from_str(r#"<one field1="...">...</one>"#).unwrap());
//! # assert_eq!(AnyName::Two(Two { field2: () }), quick_xml::de::from_str(r#"<two><field2>...</field2></two>"#).unwrap());
//! ```
//! ```
//! # use pretty_assertions::assert_eq;
//! # use serde::Deserialize;
//! # #[derive(Debug, PartialEq)]
//! #[derive(Deserialize)]
//! #[serde(rename_all = "snake_case")]
//! enum AnyName {
//!   One,
//!   // the <two> will be mapped to this
//!   #[serde(other)]
//!   Other,
//! }
//! # assert_eq!(AnyName::One,   quick_xml::de::from_str(r#"<one field1="...">...</one>"#).unwrap());
//! # assert_eq!(AnyName::Other, quick_xml::de::from_str(r#"<two><field2>...</field2></two>"#).unwrap());
//! ```
//! <div style="background:rgba(120,145,255,0.45);padding:0.75em;">
//!
//! NOTE: You should have variants for all possible tag names in your enum
//! or have an `#[serde(other)]` variant.
//! <!-- TODO: document an error type if that requirement is violated -->
//! </div>
//! </td>
//! </tr>
//! <!-- 9 ===================================================================================== -->
//! <tr>
//! <td>
//!
//! `<xs:choice>` embedded in the other element, and at the same time you want
//! to get access to other attributes that can appear in the same container
//! (`<any-tag>`). Also this case can be described, as if you want to choose
//! Rust enum variant based on a tag name:
//!
//! ```xml
//! <any-tag field="...">
//!   <one>...</one>
//! </any-tag>
//! ```
//! ```xml
//! <any-tag field="...">
//!   <two>...</two>
//! </any-tag>
//! ```
//! </td>
//! <td>
//!
//! A structure with a field which type is an `enum`.
//!
//! Names of the enum, struct, and struct field with `Choice` type does not matter:
//!
//! ```
//! # use pretty_assertions::assert_eq;
//! # use serde::Deserialize;
//! # type T = ();
//! # #[derive(Debug, PartialEq)]
//! #[derive(Deserialize)]
//! #[serde(rename_all = "snake_case")]
//! enum Choice {
//!   One,
//!   Two,
//! }
//! # #[derive(Debug, PartialEq)]
//! #[derive(Deserialize)]
//! struct AnyName {
//!   #[serde(rename = "@field")]
//!   field: T,
//!
//!   #[serde(rename = "$value")]
//!   any_name: Choice,
//! }
//! # assert_eq!(
//! #   AnyName { field: (), any_name: Choice::One },
//! #   quick_xml::de::from_str(r#"<any-tag field="..."><one>...</one></any-tag>"#).unwrap(),
//! # );
//! # assert_eq!(
//! #   AnyName { field: (), any_name: Choice::Two },
//! #   quick_xml::de::from_str(r#"<any-tag field="..."><two>...</two></any-tag>"#).unwrap(),
//! # );
//! ```
//! </td>
//! </tr>
//! <!-- 10 ==================================================================================== -->
//! <tr>
//! <td>
//!
//! `<xs:choice>` embedded in the other element, and at the same time you want
//! to get access to other elements that can appear in the same container
//! (`<any-tag>`). Also this case can be described, as if you want to choose
//! Rust enum variant based on a tag name:
//!
//! ```xml
//! <any-tag>
//!   <field>...</field>
//!   <one>...</one>
//! </any-tag>
//! ```
//! ```xml
//! <any-tag>
//!   <two>...</two>
//!   <field>...</field>
//! </any-tag>
//! ```
//! </td>
//! <td>
//!
//! A structure with a field which type is an `enum`.
//!
//! Names of the enum, struct, and struct field with `Choice` type does not matter:
//!
//! ```
//! # use pretty_assertions::assert_eq;
//! # use serde::Deserialize;
//! # type T = ();
//! # #[derive(Debug, PartialEq)]
//! #[derive(Deserialize)]
//! #[serde(rename_all = "snake_case")]
//! enum Choice {
//!   One,
//!   Two,
//! }
//! # #[derive(Debug, PartialEq)]
//! #[derive(Deserialize)]
//! struct AnyName {
//!   field: T,
//!
//!   #[serde(rename = "$value")]
//!   any_name: Choice,
//! }
//! # assert_eq!(
//! #   AnyName { field: (), any_name: Choice::One },
//! #   quick_xml::de::from_str(r#"<any-tag><field>...</field><one>...</one></any-tag>"#).unwrap(),
//! # );
//! # assert_eq!(
//! #   AnyName { field: (), any_name: Choice::Two },
//! #   quick_xml::de::from_str(r#"<any-tag><two>...</two><field>...</field></any-tag>"#).unwrap(),
//! # );
//! ```
//!
//! <div style="background:rgba(120,145,255,0.45);padding:0.75em;">
//!
//! NOTE: if your `Choice` enum would contain an `#[serde(other)]`
//! variant, element `<field>` will be mapped to the `field` and not to the enum
//! variant.
//! </div>
//!
//! </td>
//! </tr>
//! <!-- 11 ==================================================================================== -->
//! <tr>
//! <td>
//!
//! `<xs:choice>` encapsulated in other element with a fixed name:
//!
//! ```xml
//! <any-tag field="...">
//!   <choice>
//!     <one>...</one>
//!   </choice>
//! </any-tag>
//! ```
//! ```xml
//! <any-tag field="...">
//!   <choice>
//!     <two>...</two>
//!   </choice>
//! </any-tag>
//! ```
//! </td>
//! <td>
//!
//! A structure with a field of an intermediate type with one field of `enum` type.
//! Actually, this example is not necessary, because you can construct it by yourself
//! using the composition rules that were described above. However the XML construction
//! described here is very common, so it is shown explicitly.
//!
//! Names of the enum and struct does not matter:
//!
//! ```
//! # use pretty_assertions::assert_eq;
//! # use serde::Deserialize;
//! # type T = ();
//! # #[derive(Debug, PartialEq)]
//! #[derive(Deserialize)]
//! #[serde(rename_all = "snake_case")]
//! enum Choice {
//!   One,
//!   Two,
//! }
//! # #[derive(Debug, PartialEq)]
//! #[derive(Deserialize)]
//! struct Holder {
//!   #[serde(rename = "$value")]
//!   any_name: Choice,
//! }
//! # #[derive(Debug, PartialEq)]
//! #[derive(Deserialize)]
//! struct AnyName {
//!   #[serde(rename = "@field")]
//!   field: T,
//!
//!   choice: Holder,
//! }
//! # assert_eq!(
//! #   AnyName { field: (), choice: Holder { any_name: Choice::One } },
//! #   quick_xml::de::from_str(r#"<any-tag field="..."><choice><one>...</one></choice></any-tag>"#).unwrap(),
//! # );
//! # assert_eq!(
//! #   AnyName { field: (), choice: Holder { any_name: Choice::Two } },
//! #   quick_xml::de::from_str(r#"<any-tag field="..."><choice><two>...</two></choice></any-tag>"#).unwrap(),
//! # );
//! ```
//! </td>
//! </tr>
//! <!-- 12 ==================================================================================== -->
//! <tr>
//! <td>
//!
//! `<xs:choice>` encapsulated in other element with a fixed name:
//!
//! ```xml
//! <any-tag>
//!   <field>...</field>
//!   <choice>
//!     <one>...</one>
//!   </choice>
//! </any-tag>
//! ```
//! ```xml
//! <any-tag>
//!   <choice>
//!     <two>...</two>
//!   </choice>
//!   <field>...</field>
//! </any-tag>
//! ```
//! </td>
//! <td>
//!
//! A structure with a field of an intermediate type with one field of `enum` type.
//! Actually, this example is not necessary, because you can construct it by yourself
//! using the composition rules that were described above. However the XML construction
//! described here is very common, so it is shown explicitly.
//!
//! Names of the enum and struct does not matter:
//!
//! ```
//! # use pretty_assertions::assert_eq;
//! # use serde::Deserialize;
//! # type T = ();
//! # #[derive(Debug, PartialEq)]
//! #[derive(Deserialize)]
//! #[serde(rename_all = "snake_case")]
//! enum Choice {
//!   One,
//!   Two,
//! }
//! # #[derive(Debug, PartialEq)]
//! #[derive(Deserialize)]
//! struct Holder {
//!   #[serde(rename = "$value")]
//!   any_name: Choice,
//! }
//! # #[derive(Debug, PartialEq)]
//! #[derive(Deserialize)]
//! struct AnyName {
//!   field: T,
//!
//!   choice: Holder,
//! }
//! # assert_eq!(
//! #   AnyName { field: (), choice: Holder { any_name: Choice::One } },
//! #   quick_xml::de::from_str(r#"<any-tag><field>...</field><choice><one>...</one></choice></any-tag>"#).unwrap(),
//! # );
//! # assert_eq!(
//! #   AnyName { field: (), choice: Holder { any_name: Choice::Two } },
//! #   quick_xml::de::from_str(r#"<any-tag><choice><two>...</two></choice><field>...</field></any-tag>"#).unwrap(),
//! # );
//! ```
//! </td>
//! </tr>
//! <!-- ======================================================================================== -->
//! <tr><th colspan="2">
//!
//! ## Sequences (`xs:all` and `xs:sequence` XML Schema types)
//!
//! </th></tr>
//! <tr><th>To parse all these XML's...</th><th>...use that Rust type(s)</th></tr>
//! <!-- 13 ==================================================================================== -->
//! <tr>
//! <td>
//! A sequence inside of a tag without a dedicated name:
//!
//! ```xml
//! <any-tag/>
//! ```
//! ```xml
//! <any-tag>
//!   <item/>
//! </any-tag>
//! ```
//! ```xml
//! <any-tag>
//!   <item/>
//!   <item/>
//!   <item/>
//! </any-tag>
//! ```
//! </td>
//! <td>
//!
//! A structure with a field which have a sequence type, for example, [`Vec`].
//! Because XML syntax does not distinguish between empty sequences and missed
//! elements, we should indicate that on the Rust side, because serde will require
//! that field `item` exists. You can do that in two possible ways:
//!
//! Use the `#[serde(default)]` attribute for a [field] or the entire [struct]:
//! ```
//! # use pretty_assertions::assert_eq;
//! # use serde::Deserialize;
//! # type Item = ();
//! # #[derive(Debug, PartialEq)]
//! #[derive(Deserialize)]
//! struct AnyName {
//!   #[serde(default)]
//!   item: Vec<Item>,
//! }
//! # assert_eq!(
//! #   AnyName { item: vec![] },
//! #   quick_xml::de::from_str(r#"<any-tag/>"#).unwrap(),
//! # );
//! # assert_eq!(
//! #   AnyName { item: vec![()] },
//! #   quick_xml::de::from_str(r#"<any-tag><item/></any-tag>"#).unwrap(),
//! # );
//! # assert_eq!(
//! #   AnyName { item: vec![(), (), ()] },
//! #   quick_xml::de::from_str(r#"<any-tag><item/><item/><item/></any-tag>"#).unwrap(),
//! # );
//! ```
//!
//! Use the [`Option`]. In that case inner array will always contains at least one
//! element after deserialization:
//! ```ignore
//! // FIXME: #510,
//! //        UnexpectedEnd([97, 110, 121, 45, 116, 97, 103])
//! # use pretty_assertions::assert_eq;
//! # use serde::Deserialize;
//! # type Item = ();
//! # #[derive(Debug, PartialEq)]
//! #[derive(Deserialize)]
//! struct AnyName {
//!   item: Option<Vec<Item>>,
//! }
//! # assert_eq!(
//! #   AnyName { item: None },
//! #   quick_xml::de::from_str(r#"<any-tag/>"#).unwrap(),
//! # );
//! # assert_eq!(
//! #   AnyName { item: Some(vec![()]) },
//! #   quick_xml::de::from_str(r#"<any-tag><item/></any-tag>"#).unwrap(),
//! # );
//! # assert_eq!(
//! #   AnyName { item: Some(vec![(), (), ()]) },
//! #   quick_xml::de::from_str(r#"<any-tag><item/><item/><item/></any-tag>"#).unwrap(),
//! # );
//! ```
//! <div style="background:rgba(80, 240, 100, 0.20);padding:0.75em;">
//!
//! Currently not working. The bug is tracked in [#510].
//! </div>
//!
//! See also [Frequently Used Patterns](#element-lists).
//!
//! [field]: https://serde.rs/field-attrs.html#default
//! [struct]: https://serde.rs/container-attrs.html#default
//! </td>
//! </tr>
//! <!-- 14 ==================================================================================== -->
//! <tr>
//! <td>
//! A sequence with a strict order, probably with a mixed content
//! (text / CDATA and tags):
//!
//! ```xml
//! <one>...</one>
//! text
//! <![CDATA[cdata]]>
//! <two>...</two>
//! <one>...</one>
//! ```
//! <div style="background:rgba(120,145,255,0.45);padding:0.75em;">
//!
//! NOTE: this is just an example for showing mapping. XML does not allow
//! multiple root tags -- you should wrap the sequence into a tag.
//! </div>
//! </td>
//! <td>
//!
//! All elements mapped to the heterogeneous sequential type: tuple or named tuple.
//! Each element of the tuple should be able to be deserialized from the nested
//! element content (`...`), except the enum types which would be deserialized
//! from the full element (`<one>...</one>`), so they could use the element name
//! to choose the right variant:
//!
//! ```ignore
//! // FIXME: #474
//! # use pretty_assertions::assert_eq;
//! # use serde::Deserialize;
//! # type One = ();
//! # type Two = ();
//! # /*
//! type One = ...;
//! type Two = ...;
//! # */
//! # #[derive(Debug, PartialEq)]
//! #[derive(Deserialize)]
//! struct AnyName(One, String, Two, One);
//! # assert_eq!(
//! #   AnyName((), "text cdata".into(), (), ()),
//! #   quick_xml::de::from_str(r#"<one>...</one>text <![CDATA[cdata]]><two>...</two><one>...</one>"#).unwrap(),
//! # );
//! ```
//! ```ignore
//! // FIXME: #474, Custom("unknown variant `two`,
//! //                      expected `one`")
//! # use pretty_assertions::assert_eq;
//! # use serde::Deserialize;
//! # #[derive(Debug, PartialEq)]
//! #[derive(Deserialize)]
//! #[serde(rename_all = "snake_case")]
//! enum Choice {
//!   One,
//! }
//! # type Two = ();
//! # /*
//! type Two = ...;
//! # */
//! type AnyName = (Choice, String, Two, Choice);
//! # assert_eq!(
//! #   (Choice::One, "text cdata".to_string(), (), Choice::One),
//! #   quick_xml::de::from_str(r#"<one>...</one>text <![CDATA[cdata]]><two>...</two><one>...</one>"#).unwrap(),
//! # );
//! ```
//! <div style="background:rgba(120,145,255,0.45);padding:0.75em;">
//!
//! NOTE: consequent text and CDATA nodes are merged into the one text node,
//! so you cannot have two adjacent string types in your sequence.
//! </div>
//! <div style="background:rgba(80, 240, 100, 0.20);padding:0.75em;">
//!
//! Merging of the text / CDATA content is tracked in the issue [#474] and
//! will be available in the next release.
//! </div>
//! </td>
//! </tr>
//! <!-- 15 ==================================================================================== -->
//! <tr>
//! <td>
//! A sequence with a non-strict order, probably with a mixed content
//! (text / CDATA and tags).
//!
//! ```xml
//! <one>...</one>
//! text
//! <![CDATA[cdata]]>
//! <two>...</two>
//! <one>...</one>
//! ```
//! <div style="background:rgba(120,145,255,0.45);padding:0.75em;">
//!
//! NOTE: this is just an example for showing mapping. XML does not allow
//! multiple root tags -- you should wrap the sequence into a tag.
//! </div>
//! </td>
//! <td>
//! A homogeneous sequence of elements with a fixed or dynamic size:
//!
//! ```ignore
//! // FIXME: #474
//! # use pretty_assertions::assert_eq;
//! # use serde::Deserialize;
//! # #[derive(Debug, PartialEq)]
//! #[derive(Deserialize)]
//! #[serde(rename_all = "snake_case")]
//! enum Choice {
//!   One,
//!   Two,
//!   #[serde(other)]
//!   Other,
//! }
//! type AnyName = [Choice; 4];
//! # assert_eq!(
//! #   [Choice::One, Choice::Other, Choice::Two, Choice::One],
//! #   quick_xml::de::from_str::<AnyName>(r#"<one>...</one>text <![CDATA[cdata]]><two>...</two><one>...</one>"#).unwrap(),
//! # );
//! ```
//! ```ignore
//! // FIXME: Custom("unknown variant `text`, expected
//! //                one of `one`, `two`, `$value`")
//! # use pretty_assertions::assert_eq;
//! # use serde::Deserialize;
//! # #[derive(Debug, PartialEq)]
//! #[derive(Deserialize)]
//! #[serde(rename_all = "snake_case")]
//! enum Choice {
//!   One,
//!   Two,
//!   #[serde(rename = "$value")]
//!   Other(String),
//! }
//! type AnyName = Vec<Choice>;
//! # assert_eq!(
//! #   vec![
//! #     Choice::One,
//! #     Choice::Other("text cdata".into()),
//! #     Choice::Two,
//! #     Choice::One,
//! #   ],
//! #   quick_xml::de::from_str::<AnyName>(r#"<one>...</one>text <![CDATA[cdata]]><two>...</two><one>...</one>"#).unwrap(),
//! # );
//! ```
//! <div style="background:rgba(120,145,255,0.45);padding:0.75em;">
//!
//! NOTE: consequent text and CDATA nodes are merged into the one text node,
//! so you cannot have two adjacent string types in your sequence.
//! </div>
//! <div style="background:rgba(80, 240, 100, 0.20);padding:0.75em;">
//!
//! Merging of the text / CDATA content is tracked in the issue [#474] and
//! will be available in the next release.
//! </div>
//! </td>
//! </tr>
//! <!-- 16 ==================================================================================== -->
//! <tr>
//! <td>
//! A sequence with a strict order, probably with a mixed content,
//! (text and tags) inside of the other element:
//!
//! ```xml
//! <any-tag attribute="...">
//!   <one>...</one>
//!   text
//!   <![CDATA[cdata]]>
//!   <two>...</two>
//!   <one>...</one>
//! </any-tag>
//! ```
//! </td>
//! <td>
//!
//! A structure where all child elements mapped to the one field which have
//! a heterogeneous sequential type: tuple or named tuple. Each element of the
//! tuple should be able to be deserialized from the full element (`<one>...</one>`).
//!
//! You MUST specify `#[serde(rename = "$value")]` on that field:
//!
//! ```ignore
//! // FIXME: #474, Custom("duplicate field `$value`")
//! # use pretty_assertions::assert_eq;
//! # use serde::Deserialize;
//! # type One = ();
//! # type Two = ();
//! # /*
//! type One = ...;
//! type Two = ...;
//! # */
//!
//! # #[derive(Debug, PartialEq)]
//! #[derive(Deserialize)]
//! struct AnyName {
//!   #[serde(rename = "@attribute")]
//! # attribute: (),
//! # /*
//!   attribute: ...,
//! # */
//!   // Does not (yet?) supported by the serde
//!   // https://github.com/serde-rs/serde/issues/1905
//!   // #[serde(flatten)]
//!   #[serde(rename = "$value")]
//!   any_name: (One, String, Two, One),
//! }
//! # assert_eq!(
//! #   AnyName { attribute: (), any_name: ((), "text cdata".into(), (), ()) },
//! #   quick_xml::de::from_str("\
//! #     <any-tag attribute='...'>\
//! #       <one>...</one>\
//! #       text \
//! #       <![CDATA[cdata]]>\
//! #       <two>...</two>\
//! #       <one>...</one>\
//! #     </any-tag>"
//! #   ).unwrap(),
//! # );
//! ```
//! ```ignore
//! // FIXME: #474, Custom("duplicate field `$value`")
//! # use pretty_assertions::assert_eq;
//! # use serde::Deserialize;
//! # type One = ();
//! # type Two = ();
//! # /*
//! type One = ...;
//! type Two = ...;
//! # */
//!
//! # #[derive(Debug, PartialEq)]
//! #[derive(Deserialize)]
//! struct NamedTuple(One, String, Two, One);
//!
//! # #[derive(Debug, PartialEq)]
//! #[derive(Deserialize)]
//! struct AnyName {
//!   #[serde(rename = "@attribute")]
//! # attribute: (),
//! # /*
//!   attribute: ...,
//! # */
//!   // Does not (yet?) supported by the serde
//!   // https://github.com/serde-rs/serde/issues/1905
//!   // #[serde(flatten)]
//!   #[serde(rename = "$value")]
//!   any_name: NamedTuple,
//! }
//! # assert_eq!(
//! #   AnyName { attribute: (), any_name: NamedTuple((), "text cdata".into(), (), ()) },
//! #   quick_xml::de::from_str("\
//! #     <any-tag attribute='...'>\
//! #       <one>...</one>\
//! #       text \
//! #       <![CDATA[cdata]]>\
//! #       <two>...</two>\
//! #       <one>...</one>\
//! #     </any-tag>"
//! #   ).unwrap(),
//! # );
//! ```
//! <div style="background:rgba(120,145,255,0.45);padding:0.75em;">
//!
//! NOTE: consequent text and CDATA nodes are merged into the one text node,
//! so you cannot have two adjacent string types in your sequence.
//! </div>
//! <div style="background:rgba(80, 240, 100, 0.20);padding:0.75em;">
//!
//! Merging of the text / CDATA content is tracked in the issue [#474] and
//! will be available in the next release.
//! </div>
//! </td>
//! </tr>
//! <!-- 17 ==================================================================================== -->
//! <tr>
//! <td>
//! A sequence with a non-strict order, probably with a mixed content
//! (text / CDATA and tags) inside of the other element:
//!
//! ```xml
//! <any-tag>
//!   <one>...</one>
//!   text
//!   <![CDATA[cdata]]>
//!   <two>...</two>
//!   <one>...</one>
//! </any-tag>
//! ```
//! </td>
//! <td>
//!
//! A structure where all child elements mapped to the one field which have
//! a homogeneous sequential type: array-like container. A container type `T`
//! should be able to be deserialized from the nested element content (`...`),
//! except if it is an enum type which would be deserialized from the full
//! element (`<one>...</one>`).
//!
//! You MUST specify `#[serde(rename = "$value")]` on that field:
//!
//! ```ignore
//! // FIXME: Custom("unknown variant `text`, expected
//! //                one of `one`, `two`, `$value`")
//! # use pretty_assertions::assert_eq;
//! # use serde::Deserialize;
//! # #[derive(Debug, PartialEq)]
//! #[derive(Deserialize)]
//! #[serde(rename_all = "snake_case")]
//! enum Choice {
//!   One,
//!   Two,
//!   #[serde(rename = "$value")]
//!   Other(String),
//! }
//! # #[derive(Debug, PartialEq)]
//! #[derive(Deserialize)]
//! struct AnyName {
//!   #[serde(rename = "@attribute")]
//! # attribute: (),
//! # /*
//!   attribute: ...,
//! # */
//!   // Does not (yet?) supported by the serde
//!   // https://github.com/serde-rs/serde/issues/1905
//!   // #[serde(flatten)]
//!   #[serde(rename = "$value")]
//!   any_name: [Choice; 4],
//! }
//! # assert_eq!(
//! #   AnyName { attribute: (), any_name: [
//! #     Choice::One,
//! #     Choice::Other("text cdata".into()),
//! #     Choice::Two,
//! #     Choice::One,
//! #   ] },
//! #   quick_xml::de::from_str("\
//! #     <any-tag attribute='...'>\
//! #       <one>...</one>\
//! #       text \
//! #       <![CDATA[cdata]]>\
//! #       <two>...</two>\
//! #       <one>...</one>\
//! #     </any-tag>"
//! #   ).unwrap(),
//! # );
//! ```
//! ```ignore
//! // FIXME: Custom("unknown variant `text`, expected
//! //                one of `one`, `two`, `$value`")
//! # use pretty_assertions::assert_eq;
//! # use serde::Deserialize;
//! # #[derive(Debug, PartialEq)]
//! #[derive(Deserialize)]
//! #[serde(rename_all = "snake_case")]
//! enum Choice {
//!   One,
//!   Two,
//!   #[serde(rename = "$value")]
//!   Other(String),
//! }
//! # #[derive(Debug, PartialEq)]
//! #[derive(Deserialize)]
//! struct AnyName {
//!   #[serde(rename = "@attribute")]
//! # attribute: (),
//! # /*
//!   attribute: ...,
//! # */
//!   // Does not (yet?) supported by the serde
//!   // https://github.com/serde-rs/serde/issues/1905
//!   // #[serde(flatten)]
//!   #[serde(rename = "$value")]
//!   any_name: Vec<Choice>,
//! }
//! # assert_eq!(
//! #   AnyName { attribute: (), any_name: vec![
//! #     Choice::One,
//! #     Choice::Other("text cdata".into()),
//! #     Choice::Two,
//! #     Choice::One,
//! #   ] },
//! #   quick_xml::de::from_str("\
//! #     <any-tag attribute='...'>\
//! #       <one>...</one>\
//! #       text \
//! #       <![CDATA[cdata]]>\
//! #       <two>...</two>\
//! #       <one>...</one>\
//! #     </any-tag>"
//! #   ).unwrap(),
//! # );
//! ```
//! <div style="background:rgba(120,145,255,0.45);padding:0.75em;">
//!
//! NOTE: consequent text and CDATA nodes are merged into the one text node,
//! so you cannot have two adjacent string types in your sequence.
//! </div>
//! <div style="background:rgba(80, 240, 100, 0.20);padding:0.75em;">
//!
//! Merging of the text / CDATA content is tracked in the issue [#474] and
//! will be available in the next release.
//! </div>
//! </td>
//! </tr>
//! </tbody>
//! </table>
//!
//!
//!
//! Composition Rules
//! =================
//!
//! XML format is very different from other formats supported by `serde`.
//! One such difference it is how data in the serialized form is related to
//! the Rust type. Usually each byte in the data can be associated only with
//! one field in the data structure. However, XML is an exception.
//!
//! For example, took this XML:
//!
//! ```xml
//! <any>
//!   <key attr="value"/>
//! </any>
//! ```
//!
//! and try to deserialize it to the struct `AnyName`:
//!
//! ```no_run
//! # use serde::Deserialize;
//! #[derive(Deserialize)]
//! struct AnyName { // AnyName calls `deserialize_struct` on `<any><key attr="value"/></any>`
//!                  //                         Used data:          ^^^^^^^^^^^^^^^^^^^
//!   key: Inner,    // Inner   calls `deserialize_struct` on `<key attr="value"/>`
//!                  //                         Used data:          ^^^^^^^^^^^^
//! }
//! #[derive(Deserialize)]
//! struct Inner {
//!   #[serde(rename = "@attr")]
//!   attr: String,  // String  calls `deserialize_string` on `value`
//!                  //                         Used data:     ^^^^^
//! }
//! ```
//!
//! Comments shows what methods of a [`Deserializer`] called by each struct
//! `deserialize` method and which input their seen. **Used data** shows, what
//! content is actually used for deserializing. As you see, name of the inner
//! `<key>` tag used both as a map key / outer struct field name and as part
//! of the inner struct (although _value_ of the tag, i.e. `key` is not used
//! by it).
//!
//!
//!
//! Difference between `$text` and `$value` special names
//! =====================================================
//!
//! quick-xml supports two special names for fields -- `$text` and `$value`.
//! Although they may seem the same, there is a distinction. Two different
//! names is required mostly for serialization, because quick-xml should know
//! how you want to serialize certain constructs, which could be represented
//! through XML in multiple different ways.
//!
//! The only difference in how complex types and sequences are serialized.
//! If you doubt which one you should select, begin with [`$value`](#value).
//!
//! ## `$text`
//! `$text` is used when you want to write your XML as a text or a CDATA content.
//! More formally, field with that name represents simple type definition with
//! `{variety} = atomic` or `{variety} = union` whose basic members are all atomic,
//! as described in the [specification].
//!
//! As a result, not all types of such fields can be serialized. Only serialization
//! of following types are supported:
//! - all primitive types (strings, numbers, booleans)
//! - unit variants of enumerations (serializes to a name of a variant)
//! - newtypes (delegates serialization to inner type)
//! - [`Option`] of above (`None` serializes to nothing)
//! - sequences (including tuples and tuple variants of enumerations) of above,
//!   excluding `None` and empty string elements (because it will not be possible
//!   to deserialize them back). The elements are separated by space(s)
//! - unit type `()` and unit structs (serializes to nothing)
//!
//! Complex types, such as structs and maps, are not supported in this field.
//! If you want them, you should use `$value`.
//!
//! Sequences serialized to a space-delimited string, that is why only certain
//! types are allowed in this mode:
//!
//! ```
//! # use serde::{Deserialize, Serialize};
//! # use quick_xml::de::from_str;
//! # use quick_xml::se::to_string;
//! #[derive(Deserialize, Serialize, PartialEq, Debug)]
//! struct AnyName {
//!     #[serde(rename = "$text")]
//!     field: Vec<usize>,
//! }
//!
//! let obj = AnyName { field: vec![1, 2, 3] };
//! let xml = to_string(&obj).unwrap();
//! assert_eq!(xml, "<AnyName>1 2 3</AnyName>");
//!
//! let object: AnyName = from_str(&xml).unwrap();
//! assert_eq!(object, obj);
//! ```
//!
//! ## `$value`
//! <div style="background:rgba(120,145,255,0.45);padding:0.75em;">
//!
//! NOTE: a name `#content` would better explain the purpose of that field,
//! but `$value` is used for compatibility with other XML serde crates, which
//! uses that name. This allow you to switch XML crate more smoothly if required.
//! </div>
//!
//! Representation of primitive types in `$value` does not differ from their
//! representation in `$text` field. The difference is how sequences are serialized.
//! `$value` serializes each sequence item as a separate XML element. The name
//! of that element is taken from serialized type, and because only `enum`s provide
//! such name (their variant name), only they should be used for such fields.
//!
//! `$value` fields does not support `struct` types with fields, the serialization
//! of such types would end with an `Err(Unsupported)`. Unit structs and unit
//! type `()` serializing to nothing and can be deserialized from any content.
//!
//! Serialization and deserialization of `$value` field performed as usual, except
//! that name for an XML element will be given by the serialized type, instead of
//! field. The latter allow to serialize enumerated types, where variant is encoded
//! as a tag name, and, so, represent an XSD `xs:choice` schema by the Rust `enum`.
//!
//! In the example below, field will be serialized as `<field/>`, because elements
//! get their names from the field name. It cannot be deserialized, because `Enum`
//! expects elements `<A/>`, `<B/>` or `<C/>`, but `AnyName` looked only for `<field/>`:
//!
//! ```no_run
//! # use serde::{Deserialize, Serialize};
//! #[derive(Deserialize, Serialize)]
//! enum Enum { A, B, C }
//!
//! #[derive(Deserialize, Serialize)]
//! struct AnyName {
//!     // <field/>
//!     field: Enum,
//! }
//! ```
//!
//! If you rename field to `$value`, then `field` would be serialized as `<A/>`,
//! `<B/>` or `<C/>`, depending on the its content. It is also possible to
//! deserialize it from the same elements:
//!
//! ```no_run
//! # use serde::{Deserialize, Serialize};
//! # #[derive(Deserialize, Serialize)]
//! # enum Enum { A, B, C }
//! #
//! #[derive(Deserialize, Serialize)]
//! struct AnyName {
//!     // <A/>, <B/> or <C/>
//!     #[serde(rename = "$value")]
//!     field: Enum,
//! }
//! ```
//!
//! ### Primitives and sequences of primitives
//!
//! Sequences serialized to a list of elements. Note, that types that does not
//! produce their own tag (i. e. primitives) are written as is, without delimiters:
//!
//! ```
//! # use serde::{Deserialize, Serialize};
//! # use quick_xml::de::from_str;
//! # use quick_xml::se::to_string;
//! #[derive(Deserialize, Serialize, PartialEq, Debug)]
//! struct AnyName {
//!     #[serde(rename = "$value")]
//!     field: Vec<usize>,
//! }
//!
//! let obj = AnyName { field: vec![1, 2, 3] };
//! let xml = to_string(&obj).unwrap();
//! // Note, that types that does not produce their own tag are written as is!
//! assert_eq!(xml, "<AnyName>123</AnyName>");
//!
//! let object: AnyName = from_str("<AnyName>123</AnyName>").unwrap();
//! assert_eq!(object, AnyName { field: vec![123] });
//!
//! // `1 2 3` is mapped to a single `usize` element
//! // It is impossible to deserialize list of primitives to such field
//! from_str::<AnyName>("<AnyName>1 2 3</AnyName>").unwrap_err();
//! ```
//!
//! A particular case of that example is a string `$value` field, which probably
//! would be a most used example of that attribute:
//!
//! ```
//! # use serde::{Deserialize, Serialize};
//! # use quick_xml::de::from_str;
//! # use quick_xml::se::to_string;
//! #[derive(Deserialize, Serialize, PartialEq, Debug)]
//! struct AnyName {
//!     #[serde(rename = "$value")]
//!     field: String,
//! }
//!
//! let obj = AnyName { field: "content".to_string() };
//! let xml = to_string(&obj).unwrap();
//! assert_eq!(xml, "<AnyName>content</AnyName>");
//! ```
//!
//! ### Structs and sequences of structs
//!
//! Note, that structures does not have serializable name as well (name of the
//! type are never used), so it is impossible to serialize non-unit struct or
//! sequence of non-unit structs in `$value` field. (sequences of) unit structs
//! are serialized as empty string, although, because units itself serializing
//! to nothing:
//!
//! ```
//! # use serde::{Deserialize, Serialize};
//! # use quick_xml::de::from_str;
//! # use quick_xml::se::to_string;
//! #[derive(Deserialize, Serialize, PartialEq, Debug)]
//! struct Unit;
//!
//! #[derive(Deserialize, Serialize, PartialEq, Debug)]
//! struct AnyName {
//!     // #[serde(default)] is required to deserialization of empty lists
//!     // This is a general note, not related to $value
//!     #[serde(rename = "$value", default)]
//!     field: Vec<Unit>,
//! }
//!
//! let obj = AnyName { field: vec![Unit, Unit, Unit] };
//! let xml = to_string(&obj).unwrap();
//! assert_eq!(xml, "<AnyName/>");
//!
//! let object: AnyName = from_str("<AnyName/>").unwrap();
//! assert_eq!(object, AnyName { field: vec![] });
//!
//! let object: AnyName = from_str("<AnyName></AnyName>").unwrap();
//! assert_eq!(object, AnyName { field: vec![] });
//!
//! let object: AnyName = from_str("<AnyName><A/><B/><C/></AnyName>").unwrap();
//! assert_eq!(object, AnyName { field: vec![Unit, Unit, Unit] });
//! ```
//!
//! ### Enums and sequences of enums
//!
//! Enumerations uses the variant name as an element name:
//!
//! ```
//! # use serde::{Deserialize, Serialize};
//! # use quick_xml::de::from_str;
//! # use quick_xml::se::to_string;
//! #[derive(Deserialize, Serialize, PartialEq, Debug)]
//! struct AnyName {
//!     #[serde(rename = "$value")]
//!     field: Vec<Enum>,
//! }
//!
//! #[derive(Deserialize, Serialize, PartialEq, Debug)]
//! enum Enum { A, B, C }
//!
//! let obj = AnyName { field: vec![Enum::A, Enum::B, Enum::C] };
//! let xml = to_string(&obj).unwrap();
//! assert_eq!(
//!     xml,
//!     "<AnyName>\
//!         <A/>\
//!         <B/>\
//!         <C/>\
//!      </AnyName>"
//! );
//!
//! let object: AnyName = from_str(&xml).unwrap();
//! assert_eq!(object, obj);
//! ```
//!
//! ----------------------------------------------------------------------------
//!
//! You can have either `$text` or `$value` field in your structs. Unfortunately,
//! that is not enforced, so you can theoretically have both, but you should
//! avoid that.
//!
//!
//!
//! Frequently Used Patterns
//! ========================
//!
//! Some XML constructs used so frequent, that it is worth to document the recommended
//! way to represent them in the Rust. The sections below describes them.
//!
//! ## `<element>` lists
//! Many XML formats wrap lists of elements in the additional container,
//! although this is not required by the XML rules:
//!
//! ```xml
//! <root>
//!   <field1/>
//!   <field2/>
//!   <list><!-- Container -->
//!     <element/>
//!     <element/>
//!     <element/>
//!   </list>
//!   <field3/>
//! </root>
//! ```
//! In this case, there is a great desire to describe this XML in this way:
//! ```
//! /// Represents <element/>
//! type Element = ();
//!
//! /// Represents <root>...</root>
//! struct AnyName {
//!     // Incorrect
//!     list: Vec<Element>,
//! }
//! ```
//! This will not work, because potentially `<list>` element can have attributes
//! and other elements inside. You should define the struct for the `<list>`
//! explicitly, as you do that in the XSD for that XML:
//! ```
//! /// Represents <element/>
//! type Element = ();
//!
//! /// Represents <root>...</root>
//! struct AnyName {
//!     // Correct
//!     list: List,
//! }
//! /// Represents <list>...</list>
//! struct List {
//!     element: Vec<Element>,
//! }
//! ```
//!
//! If you want to simplify your API, you could write a simple function for unwrapping
//! inner list and apply it via [`deserialize_with`]:
//!
//! ```
//! # use pretty_assertions::assert_eq;
//! use quick_xml::de::from_str;
//! use serde::{Deserialize, Deserializer};
//!
//! /// Represents <element/>
//! type Element = ();
//!
//! /// Represents <root>...</root>
//! #[derive(Deserialize, Debug, PartialEq)]
//! struct AnyName {
//!     #[serde(deserialize_with = "unwrap_list")]
//!     list: Vec<Element>,
//! }
//!
//! fn unwrap_list<'de, D>(deserializer: D) -> Result<Vec<Element>, D::Error>
//! where
//!     D: Deserializer<'de>,
//! {
//!     /// Represents <list>...</list>
//!     #[derive(Deserialize)]
//!     struct List {
//!         // default allows empty list
//!         #[serde(default)]
//!         element: Vec<Element>,
//!     }
//!     Ok(List::deserialize(deserializer)?.element)
//! }
//!
//! assert_eq!(
//!     AnyName { list: vec![(), (), ()] },
//!     from_str("
//!         <root>
//!           <list>
//!             <element/>
//!             <element/>
//!             <element/>
//!           </list>
//!         </root>
//!     ").unwrap(),
//! );
//! ```
//!
//! Instead of writing such functions manually, you also could try <https://lib.rs/crates/serde-query>.
//!
//! [specification]: https://www.w3.org/TR/xmlschema11-1/#Simple_Type_Definition
//! [`deserialize_with`]: https://serde.rs/field-attrs.html#deserialize_with
//! [#474]: https://github.com/tafia/quick-xml/issues/474
//! [#497]: https://github.com/tafia/quick-xml/issues/497
//! [#510]: https://github.com/tafia/quick-xml/issues/510

// Macros should be defined before the modules that using them
// Also, macros should be imported before using them
use serde::serde_if_integer128;

macro_rules! deserialize_type {
    ($deserialize:ident => $visit:ident, $($mut:tt)?) => {
        fn $deserialize<V>($($mut)? self, visitor: V) -> Result<V::Value, DeError>
        where
            V: Visitor<'de>,
        {
            // No need to unescape because valid integer representations cannot be escaped
            let text = self.read_string(false)?;
            visitor.$visit(text.parse()?)
        }
    };
}

/// Implement deserialization methods for scalar types, such as numbers, strings,
/// byte arrays, booleans and identifiers.
macro_rules! deserialize_primitives {
    ($($mut:tt)?) => {
        deserialize_type!(deserialize_i8 => visit_i8, $($mut)?);
        deserialize_type!(deserialize_i16 => visit_i16, $($mut)?);
        deserialize_type!(deserialize_i32 => visit_i32, $($mut)?);
        deserialize_type!(deserialize_i64 => visit_i64, $($mut)?);

        deserialize_type!(deserialize_u8 => visit_u8, $($mut)?);
        deserialize_type!(deserialize_u16 => visit_u16, $($mut)?);
        deserialize_type!(deserialize_u32 => visit_u32, $($mut)?);
        deserialize_type!(deserialize_u64 => visit_u64, $($mut)?);

        serde_if_integer128! {
            deserialize_type!(deserialize_i128 => visit_i128, $($mut)?);
            deserialize_type!(deserialize_u128 => visit_u128, $($mut)?);
        }

        deserialize_type!(deserialize_f32 => visit_f32, $($mut)?);
        deserialize_type!(deserialize_f64 => visit_f64, $($mut)?);

        fn deserialize_bool<V>($($mut)? self, visitor: V) -> Result<V::Value, DeError>
        where
            V: Visitor<'de>,
        {
            // No need to unescape because valid boolean representations cannot be escaped
            let text = self.read_string(false)?;

            str2bool(&text, visitor)
        }

        /// Representation of owned strings the same as [non-owned](#method.deserialize_str).
        fn deserialize_string<V>(self, visitor: V) -> Result<V::Value, DeError>
        where
            V: Visitor<'de>,
        {
            self.deserialize_str(visitor)
        }

        /// Character represented as [strings](#method.deserialize_str).
        fn deserialize_char<V>(self, visitor: V) -> Result<V::Value, DeError>
        where
            V: Visitor<'de>,
        {
            self.deserialize_str(visitor)
        }

        fn deserialize_str<V>($($mut)? self, visitor: V) -> Result<V::Value, DeError>
        where
            V: Visitor<'de>,
        {
            let text = self.read_string(true)?;
            match text {
                Cow::Borrowed(string) => visitor.visit_borrowed_str(string),
                Cow::Owned(string) => visitor.visit_string(string),
            }
        }

        /// Returns [`DeError::Unsupported`]
        fn deserialize_bytes<V>(self, _visitor: V) -> Result<V::Value, DeError>
        where
            V: Visitor<'de>,
        {
            Err(DeError::Unsupported("binary data content is not supported by XML format".into()))
        }

        /// Forwards deserialization to the [`deserialize_bytes`](#method.deserialize_bytes).
        fn deserialize_byte_buf<V>(self, visitor: V) -> Result<V::Value, DeError>
        where
            V: Visitor<'de>,
        {
            self.deserialize_bytes(visitor)
        }

        /// Identifiers represented as [strings](#method.deserialize_str).
        fn deserialize_identifier<V>(self, visitor: V) -> Result<V::Value, DeError>
        where
            V: Visitor<'de>,
        {
            self.deserialize_str(visitor)
        }
    };
}

mod escape;
mod key;
mod map;
mod simple_type;
mod var;

pub use crate::errors::serialize::DeError;
use crate::{
    encoding::Decoder,
    errors::Error,
    events::{BytesCData, BytesEnd, BytesStart, BytesText, Event},
    name::QName,
    reader::Reader,
};
use serde::de::{self, Deserialize, DeserializeOwned, DeserializeSeed, SeqAccess, Visitor};
use std::borrow::Cow;
#[cfg(feature = "overlapped-lists")]
use std::collections::VecDeque;
use std::io::BufRead;
#[cfg(feature = "overlapped-lists")]
use std::num::NonZeroUsize;

/// Data represented by a text node or a CDATA node. XML markup is not expected
pub(crate) const TEXT_KEY: &str = "$text";
/// Data represented by any XML markup inside
pub(crate) const VALUE_KEY: &str = "$value";

/// Simplified event which contains only these variants that used by deserializer
#[derive(Debug, PartialEq, Eq)]
pub enum DeEvent<'a> {
    /// Start tag (with attributes) `<tag attr="value">`.
    Start(BytesStart<'a>),
    /// End tag `</tag>`.
    End(BytesEnd<'a>),
    /// Escaped character data between `Start` and `End` element.
    Text(BytesText<'a>),
    /// Unescaped character data between `Start` and `End` element,
    /// stored in `<![CDATA[...]]>`.
    CData(BytesCData<'a>),
    /// End of XML document.
    Eof,
}

////////////////////////////////////////////////////////////////////////////////////////////////////

/// A structure that deserializes XML into Rust values.
pub struct Deserializer<'de, R>
where
    R: XmlRead<'de>,
{
    /// An XML reader that streams events into this deserializer
    reader: R,

    /// When deserializing sequences sometimes we have to skip unwanted events.
    /// That events should be stored and then replayed. This is a replay buffer,
    /// that streams events while not empty. When it exhausted, events will
    /// requested from [`Self::reader`].
    #[cfg(feature = "overlapped-lists")]
    read: VecDeque<DeEvent<'de>>,
    /// When deserializing sequences sometimes we have to skip events, because XML
    /// is tolerant to elements order and even if in the XSD order is strictly
    /// specified (using `xs:sequence`) most of XML parsers allows order violations.
    /// That means, that elements, forming a sequence, could be overlapped with
    /// other elements, do not related to that sequence.
    ///
    /// In order to support this, deserializer will scan events and skip unwanted
    /// events, store them here. After call [`Self::start_replay()`] all events
    /// moved from this to [`Self::read`].
    #[cfg(feature = "overlapped-lists")]
    write: VecDeque<DeEvent<'de>>,
    /// Maximum number of events that can be skipped when processing sequences
    /// that occur out-of-order. This field is used to prevent potential
    /// denial-of-service (DoS) attacks which could cause infinite memory
    /// consumption when parsing a very large amount of XML into a sequence field.
    #[cfg(feature = "overlapped-lists")]
    limit: Option<NonZeroUsize>,

    #[cfg(not(feature = "overlapped-lists"))]
    peek: Option<DeEvent<'de>>,
}

/// Deserialize an instance of type `T` from a string of XML text.
pub fn from_str<'de, T>(s: &'de str) -> Result<T, DeError>
where
    T: Deserialize<'de>,
{
    let mut de = Deserializer::from_str(s);
    T::deserialize(&mut de)
}

/// Deserialize from a reader. This method will do internal copies of data
/// readed from `reader`. If you want have a `&str` input and want to borrow
/// as much as possible, use [`from_str`].
pub fn from_reader<R, T>(reader: R) -> Result<T, DeError>
where
    R: BufRead,
    T: DeserializeOwned,
{
    let mut de = Deserializer::from_reader(reader);
    T::deserialize(&mut de)
}

// TODO: According to the https://www.w3.org/TR/xmlschema-2/#boolean,
// valid boolean representations are only "true", "false", "1", and "0"
fn str2bool<'de, V>(value: &str, visitor: V) -> Result<V::Value, DeError>
where
    V: de::Visitor<'de>,
{
    match value {
        "true" | "1" | "True" | "TRUE" | "t" | "Yes" | "YES" | "yes" | "y" => {
            visitor.visit_bool(true)
        }
        "false" | "0" | "False" | "FALSE" | "f" | "No" | "NO" | "no" | "n" => {
            visitor.visit_bool(false)
        }
        _ => Err(DeError::InvalidBoolean(value.into())),
    }
}

fn deserialize_bool<'de, V>(value: &[u8], decoder: Decoder, visitor: V) -> Result<V::Value, DeError>
where
    V: Visitor<'de>,
{
    #[cfg(feature = "encoding")]
    {
        let value = decoder.decode(value)?;
        // No need to unescape because valid boolean representations cannot be escaped
        str2bool(value.as_ref(), visitor)
    }

    #[cfg(not(feature = "encoding"))]
    {
        // No need to unescape because valid boolean representations cannot be escaped
        match value {
            b"true" | b"1" | b"True" | b"TRUE" | b"t" | b"Yes" | b"YES" | b"yes" | b"y" => {
                visitor.visit_bool(true)
            }
            b"false" | b"0" | b"False" | b"FALSE" | b"f" | b"No" | b"NO" | b"no" | b"n" => {
                visitor.visit_bool(false)
            }
            e => Err(DeError::InvalidBoolean(decoder.decode(e)?.into())),
        }
    }
}

impl<'de, R> Deserializer<'de, R>
where
    R: XmlRead<'de>,
{
    /// Create an XML deserializer from one of the possible quick_xml input sources.
    ///
    /// Typically it is more convenient to use one of these methods instead:
    ///
    ///  - [`Deserializer::from_str`]
    ///  - [`Deserializer::from_reader`]
    fn new(reader: R) -> Self {
        Deserializer {
            reader,

            #[cfg(feature = "overlapped-lists")]
            read: VecDeque::new(),
            #[cfg(feature = "overlapped-lists")]
            write: VecDeque::new(),
            #[cfg(feature = "overlapped-lists")]
            limit: None,

            #[cfg(not(feature = "overlapped-lists"))]
            peek: None,
        }
    }

    /// Set the maximum number of events that could be skipped during deserialization
    /// of sequences.
    ///
    /// If `<element>` contains more than specified nested elements, `$text` or
    /// CDATA nodes, then [`DeError::TooManyEvents`] will be returned during
    /// deserialization of sequence field (any type that uses [`deserialize_seq`]
    /// for the deserialization, for example, `Vec<T>`).
    ///
    /// This method can be used to prevent a [DoS] attack and infinite memory
    /// consumption when parsing a very large XML to a sequence field.
    ///
    /// It is strongly recommended to set limit to some value when you parse data
    /// from untrusted sources. You should choose a value that your typical XMLs
    /// can have _between_ different elements that corresponds to the same sequence.
    ///
    /// # Examples
    ///
    /// Let's imagine, that we deserialize such structure:
    /// ```
    /// struct List {
    ///   item: Vec<()>,
    /// }
    /// ```
    ///
    /// The XML that we try to parse look like this:
    /// ```xml
    /// <any-name>
    ///   <item/>
    ///   <!-- Bufferization starts at this point -->
    ///   <another-item>
    ///     <some-element>with text</some-element>
    ///     <yet-another-element/>
    ///   </another-item>
    ///   <!-- Buffer will be emptied at this point; 7 events were buffered -->
    ///   <item/>
    ///   <!-- There is nothing to buffer, because elements follows each other -->
    ///   <item/>
    /// </any-name>
    /// ```
    ///
    /// There, when we deserialize the `item` field, we need to buffer 7 events,
    /// before we can deserialize the second `<item/>`:
    ///
    /// - `<another-item>`
    /// - `<some-element>`
    /// - `$text(with text)`
    /// - `</some-element>`
    /// - `<yet-another-element/>` (virtual start event)
    /// - `<yet-another-element/>` (virtual end event)
    /// - `</another-item>`
    ///
    /// Note, that `<yet-another-element/>` internally represented as 2 events:
    /// one for the start tag and one for the end tag. In the future this can be
    /// eliminated, but for now we use [auto-expanding feature] of a reader,
    /// because this simplifies deserializer code.
    ///
    /// [`deserialize_seq`]: serde::Deserializer::deserialize_seq
    /// [DoS]: https://en.wikipedia.org/wiki/Denial-of-service_attack
    /// [auto-expanding feature]: Reader::expand_empty_elements
    #[cfg(feature = "overlapped-lists")]
    pub fn event_buffer_size(&mut self, limit: Option<NonZeroUsize>) -> &mut Self {
        self.limit = limit;
        self
    }

    #[cfg(feature = "overlapped-lists")]
    fn peek(&mut self) -> Result<&DeEvent<'de>, DeError> {
        if self.read.is_empty() {
            self.read.push_front(self.reader.next()?);
        }
        if let Some(event) = self.read.front() {
            return Ok(event);
        }
        // SAFETY: `self.read` was filled in the code above.
        // NOTE: Can be replaced with `unsafe { std::hint::unreachable_unchecked() }`
        // if unsafe code will be allowed
        unreachable!()
    }
    #[cfg(not(feature = "overlapped-lists"))]
    fn peek(&mut self) -> Result<&DeEvent<'de>, DeError> {
        if self.peek.is_none() {
            self.peek = Some(self.reader.next()?);
        }
        match self.peek.as_ref() {
            Some(v) => Ok(v),
            // SAFETY: a `None` variant for `self.peek` would have been replaced
            // by a `Some` variant in the code above.
            // TODO: Can be replaced with `unsafe { std::hint::unreachable_unchecked() }`
            // if unsafe code will be allowed
            None => unreachable!(),
        }
    }

    fn next(&mut self) -> Result<DeEvent<'de>, DeError> {
        // Replay skipped or peeked events
        #[cfg(feature = "overlapped-lists")]
        if let Some(event) = self.read.pop_front() {
            return Ok(event);
        }
        #[cfg(not(feature = "overlapped-lists"))]
        if let Some(e) = self.peek.take() {
            return Ok(e);
        }
        self.reader.next()
    }

    /// Returns the mark after which all events, skipped by [`Self::skip()`] call,
    /// should be replayed after calling [`Self::start_replay()`].
    #[cfg(feature = "overlapped-lists")]
    #[inline]
    #[must_use = "returned checkpoint should be used in `start_replay`"]
    fn skip_checkpoint(&self) -> usize {
        self.write.len()
    }

    /// Extracts XML tree of events from and stores them in the skipped events
    /// buffer from which they can be retrieved later. You MUST call
    /// [`Self::start_replay()`] after calling this to give access to the skipped
    /// events and release internal buffers.
    #[cfg(feature = "overlapped-lists")]
    fn skip(&mut self) -> Result<(), DeError> {
        let event = self.next()?;
        self.skip_event(event)?;
        match self.write.back() {
            // Skip all subtree, if we skip a start event
            Some(DeEvent::Start(e)) => {
                let end = e.name().as_ref().to_owned();
                let mut depth = 0;
                loop {
                    let event = self.next()?;
                    match event {
                        DeEvent::Start(ref e) if e.name().as_ref() == end => {
                            self.skip_event(event)?;
                            depth += 1;
                        }
                        DeEvent::End(ref e) if e.name().as_ref() == end => {
                            self.skip_event(event)?;
                            if depth == 0 {
                                break;
                            }
                            depth -= 1;
                        }
                        DeEvent::Eof => {
                            self.skip_event(event)?;
                            break;
                        }
                        _ => self.skip_event(event)?,
                    }
                }
            }
            _ => (),
        }
        Ok(())
    }

    #[cfg(feature = "overlapped-lists")]
    #[inline]
    fn skip_event(&mut self, event: DeEvent<'de>) -> Result<(), DeError> {
        if let Some(max) = self.limit {
            if self.write.len() >= max.get() {
                return Err(DeError::TooManyEvents(max));
            }
        }
        self.write.push_back(event);
        Ok(())
    }

    /// Moves buffered events, skipped after given `checkpoint` from [`Self::write`]
    /// skip buffer to [`Self::read`] buffer.
    ///
    /// After calling this method, [`Self::peek()`] and [`Self::next()`] starts
    /// return events that was skipped previously by calling [`Self::skip()`],
    /// and only when all that events will be consumed, the deserializer starts
    /// to drain events from underlying reader.
    ///
    /// This method MUST be called if any number of [`Self::skip()`] was called
    /// after [`Self::new()`] or `start_replay()` or you'll lost events.
    #[cfg(feature = "overlapped-lists")]
    fn start_replay(&mut self, checkpoint: usize) {
        if checkpoint == 0 {
            self.write.append(&mut self.read);
            std::mem::swap(&mut self.read, &mut self.write);
        } else {
            let mut read = self.write.split_off(checkpoint);
            read.append(&mut self.read);
            self.read = read;
        }
    }

    #[inline]
    fn read_string(&mut self, unescape: bool) -> Result<Cow<'de, str>, DeError> {
        self.read_string_impl(unescape, true)
    }

    /// Consumes a one XML element or an XML tree, returns associated text or
    /// an empty string.
    ///
    /// If `allow_start` is `false`, then only one event is consumed. If that
    /// event is [`DeEvent::Start`], then [`DeError::UnexpectedStart`] is returned.
    ///
    /// If `allow_start` is `true`, then first text of CDATA event inside it is
    /// returned and all other content is skipped until corresponding end tag
    /// will be consumed.
    ///
    /// # Handling events
    ///
    /// The table below shows how events is handled by this method:
    ///
    /// |Event             |XML                        |Handling
    /// |------------------|---------------------------|----------------------------------------
    /// |[`DeEvent::Start`]|`<tag>...</tag>`           |if `allow_start == true`, result determined by the second table, otherwise emits [`UnexpectedStart("tag")`](DeError::UnexpectedStart)
    /// |[`DeEvent::End`]  |`</any-tag>`               |Emits [`UnexpectedEnd("any-tag")`](DeError::UnexpectedEnd)
    /// |[`DeEvent::Text`] |`text content`             |Unescapes `text content` and returns it
    /// |[`DeEvent::CData`]|`<![CDATA[cdata content]]>`|Returns `cdata content` unchanged
    /// |[`DeEvent::Eof`]  |                           |Emits [`UnexpectedEof`](DeError::UnexpectedEof)
    ///
    /// Second event, consumed if [`DeEvent::Start`] was received and `allow_start == true`:
    ///
    /// |Event             |XML                        |Handling
    /// |------------------|---------------------------|----------------------------------------------------------------------------------
    /// |[`DeEvent::Start`]|`<any-tag>...</any-tag>`   |Emits [`UnexpectedStart("any-tag")`](DeError::UnexpectedStart)
    /// |[`DeEvent::End`]  |`</tag>`                   |Returns an empty slice, if close tag matched the open one
    /// |[`DeEvent::End`]  |`</any-tag>`               |Emits [`UnexpectedEnd("any-tag")`](DeError::UnexpectedEnd)
    /// |[`DeEvent::Text`] |`text content`             |Unescapes `text content` and returns it, consumes events up to `</tag>`
    /// |[`DeEvent::CData`]|`<![CDATA[cdata content]]>`|Returns `cdata content` unchanged, consumes events up to `</tag>`
    /// |[`DeEvent::Eof`]  |                           |Emits [`UnexpectedEof`](DeError::UnexpectedEof)
    fn read_string_impl(
        &mut self,
        unescape: bool,
        allow_start: bool,
    ) -> Result<Cow<'de, str>, DeError> {
        match self.next()? {
            DeEvent::Text(e) => Ok(e.decode(unescape)?),
            DeEvent::CData(e) => Ok(e.decode()?),
            DeEvent::Start(e) if allow_start => {
                // allow one nested level
                let inner = self.next()?;
                let t = match inner {
                    DeEvent::Text(t) => t.decode(unescape)?,
                    DeEvent::CData(t) => t.decode()?,
                    DeEvent::Start(s) => {
                        return Err(DeError::UnexpectedStart(s.name().as_ref().to_owned()))
                    }
                    // We can get End event in case of `<tag></tag>` or `<tag/>` input
                    // Return empty text in that case
                    DeEvent::End(end) if end.name() == e.name() => {
                        return Ok("".into());
                    }
                    DeEvent::End(end) => {
                        return Err(DeError::UnexpectedEnd(end.name().as_ref().to_owned()))
                    }
                    DeEvent::Eof => return Err(DeError::UnexpectedEof),
                };
                self.read_to_end(e.name())?;
                Ok(t)
            }
            DeEvent::Start(e) => Err(DeError::UnexpectedStart(e.name().as_ref().to_owned())),
            DeEvent::End(e) => Err(DeError::UnexpectedEnd(e.name().as_ref().to_owned())),
            DeEvent::Eof => Err(DeError::UnexpectedEof),
        }
    }

    /// Drops all events until event with [name](BytesEnd::name()) `name` won't be
    /// dropped. This method should be called after [`Self::next()`]
    #[cfg(feature = "overlapped-lists")]
    fn read_to_end(&mut self, name: QName) -> Result<(), DeError> {
        let mut depth = 0;
        loop {
            match self.read.pop_front() {
                Some(DeEvent::Start(e)) if e.name() == name => {
                    depth += 1;
                }
                Some(DeEvent::End(e)) if e.name() == name => {
                    if depth == 0 {
                        break;
                    }
                    depth -= 1;
                }

                // Drop all other skipped events
                Some(_) => continue,

                // If we do not have skipped events, use effective reading that will
                // not allocate memory for events
                None => {
                    // We should close all opened tags, because we could buffer
                    // Start events, but not the corresponding End events. So we
                    // keep reading events until we exit all nested tags.
                    // `read_to_end()` will return an error if an Eof was encountered
                    // preliminary (in case of malformed XML).
                    //
                    // <tag><tag></tag></tag>
                    // ^^^^^^^^^^             - buffered in `self.read`, when `self.read_to_end()` is called, depth = 2
                    //           ^^^^^^       - read by the first call of `self.reader.read_to_end()`
                    //                 ^^^^^^ - read by the second call of `self.reader.read_to_end()`
                    loop {
                        self.reader.read_to_end(name)?;
                        if depth == 0 {
                            break;
                        }
                        depth -= 1;
                    }
                    break;
                }
            }
        }
        Ok(())
    }
    #[cfg(not(feature = "overlapped-lists"))]
    fn read_to_end(&mut self, name: QName) -> Result<(), DeError> {
        // First one might be in self.peek
        match self.next()? {
            DeEvent::Start(e) => self.reader.read_to_end(e.name())?,
            DeEvent::End(e) if e.name() == name => return Ok(()),
            _ => (),
        }
        self.reader.read_to_end(name)
    }
}

impl<'de> Deserializer<'de, SliceReader<'de>> {
    /// Create new deserializer that will borrow data from the specified string
    pub fn from_str(s: &'de str) -> Self {
        let mut reader = Reader::from_str(s);
        reader
            .expand_empty_elements(true)
            .check_end_names(true)
            .trim_text(true);
        Self::new(SliceReader { reader })
    }
}

impl<'de, R> Deserializer<'de, IoReader<R>>
where
    R: BufRead,
{
    /// Create new deserializer that will copy data from the specified reader
    /// into internal buffer. If you already have a string use [`Self::from_str`]
    /// instead, because it will borrow instead of copy. If you have `&[u8]` which
    /// is known to represent UTF-8, you can decode it first before using [`from_str`].
    pub fn from_reader(reader: R) -> Self {
        let mut reader = Reader::from_reader(reader);
        reader
            .expand_empty_elements(true)
            .check_end_names(true)
            .trim_text(true);

        Self::new(IoReader {
            reader,
            buf: Vec::new(),
        })
    }
}

impl<'de, 'a, R> de::Deserializer<'de> for &'a mut Deserializer<'de, R>
where
    R: XmlRead<'de>,
{
    type Error = DeError;

    deserialize_primitives!();

    fn deserialize_struct<V>(
        self,
        _name: &'static str,
        fields: &'static [&'static str],
        visitor: V,
    ) -> Result<V::Value, DeError>
    where
        V: Visitor<'de>,
    {
        match self.next()? {
            DeEvent::Start(e) => {
                let name = e.name().as_ref().to_vec();
                let map = map::MapAccess::new(self, e, fields)?;
                let value = visitor.visit_map(map)?;
                self.read_to_end(QName(&name))?;
                Ok(value)
            }
            DeEvent::End(e) => Err(DeError::UnexpectedEnd(e.name().as_ref().to_owned())),
            DeEvent::Text(_) | DeEvent::CData(_) => Err(DeError::ExpectedStart),
            DeEvent::Eof => Err(DeError::UnexpectedEof),
        }
    }

    /// Unit represented in XML as a `xs:element` or text/CDATA content.
    /// Any content inside `xs:element` is ignored and skipped.
    ///
    /// Produces unit struct from any of following inputs:
    /// - any `<tag ...>...</tag>`
    /// - any `<tag .../>`
    /// - any text content
    /// - any CDATA content
    ///
    /// # Events handling
    ///
    /// |Event             |XML                        |Handling
    /// |------------------|---------------------------|-------------------------------------------
    /// |[`DeEvent::Start`]|`<tag>...</tag>`           |Calls `visitor.visit_unit()`, consumes all events up to corresponding `End` event
    /// |[`DeEvent::End`]  |`</tag>`                   |Emits [`UnexpectedEnd("tag")`](DeError::UnexpectedEnd)
    /// |[`DeEvent::Text`] |`text content`             |Calls `visitor.visit_unit()`. Text content is ignored
    /// |[`DeEvent::CData`]|`<![CDATA[cdata content]]>`|Calls `visitor.visit_unit()`. CDATA content is ignored
    /// |[`DeEvent::Eof`]  |                           |Emits [`UnexpectedEof`](DeError::UnexpectedEof)
    fn deserialize_unit<V>(self, visitor: V) -> Result<V::Value, DeError>
    where
        V: Visitor<'de>,
    {
        match self.next()? {
            DeEvent::Start(s) => {
                self.read_to_end(s.name())?;
                visitor.visit_unit()
            }
            DeEvent::Text(_) | DeEvent::CData(_) => visitor.visit_unit(),
            DeEvent::End(e) => Err(DeError::UnexpectedEnd(e.name().as_ref().to_owned())),
            DeEvent::Eof => Err(DeError::UnexpectedEof),
        }
    }

    /// Representation of the names units the same as [unnamed units](#method.deserialize_unit)
    fn deserialize_unit_struct<V>(
        self,
        _name: &'static str,
        visitor: V,
    ) -> Result<V::Value, DeError>
    where
        V: Visitor<'de>,
    {
        self.deserialize_unit(visitor)
    }

    fn deserialize_newtype_struct<V>(
        self,
        _name: &'static str,
        visitor: V,
    ) -> Result<V::Value, DeError>
    where
        V: Visitor<'de>,
    {
        self.deserialize_tuple(1, visitor)
    }

    /// Representation of tuples the same as [sequences](#method.deserialize_seq).
    fn deserialize_tuple<V>(self, _len: usize, visitor: V) -> Result<V::Value, DeError>
    where
        V: Visitor<'de>,
    {
        self.deserialize_seq(visitor)
    }

    /// Representation of named tuples the same as [unnamed tuples](#method.deserialize_tuple).
    fn deserialize_tuple_struct<V>(
        self,
        _name: &'static str,
        len: usize,
        visitor: V,
    ) -> Result<V::Value, DeError>
    where
        V: Visitor<'de>,
    {
        self.deserialize_tuple(len, visitor)
    }

    fn deserialize_enum<V>(
        self,
        _name: &'static str,
        _variants: &'static [&'static str],
        visitor: V,
    ) -> Result<V::Value, DeError>
    where
        V: Visitor<'de>,
    {
        let value = visitor.visit_enum(var::EnumAccess::new(self))?;
        Ok(value)
    }

    fn deserialize_seq<V>(self, visitor: V) -> Result<V::Value, DeError>
    where
        V: Visitor<'de>,
    {
        visitor.visit_seq(self)
    }

    fn deserialize_map<V>(self, visitor: V) -> Result<V::Value, DeError>
    where
        V: Visitor<'de>,
    {
        self.deserialize_struct("", &[], visitor)
    }

    fn deserialize_option<V>(self, visitor: V) -> Result<V::Value, DeError>
    where
        V: Visitor<'de>,
    {
        match self.peek()? {
            DeEvent::Text(t) if t.is_empty() => visitor.visit_none(),
            DeEvent::CData(t) if t.is_empty() => visitor.visit_none(),
            DeEvent::Eof => visitor.visit_none(),
            _ => visitor.visit_some(self),
        }
    }

    /// Always call `visitor.visit_unit()` because returned value ignored in any case.
    ///
    /// This method consumes any single [event][DeEvent] except the [`Start`][DeEvent::Start]
    /// event, in which case all events up to corresponding [`End`][DeEvent::End] event will
    /// be consumed.
    ///
    /// This method returns error if current event is [`End`][DeEvent::End] or [`Eof`][DeEvent::Eof]
    fn deserialize_ignored_any<V>(self, visitor: V) -> Result<V::Value, DeError>
    where
        V: Visitor<'de>,
    {
        match self.next()? {
            DeEvent::Start(e) => self.read_to_end(e.name())?,
            DeEvent::End(e) => return Err(DeError::UnexpectedEnd(e.name().as_ref().to_owned())),
            DeEvent::Eof => return Err(DeError::UnexpectedEof),
            _ => (),
        }
        visitor.visit_unit()
    }

    fn deserialize_any<V>(self, visitor: V) -> Result<V::Value, DeError>
    where
        V: Visitor<'de>,
    {
        match self.peek()? {
            DeEvent::Start(_) => self.deserialize_map(visitor),
            // Redirect to deserialize_unit in order to consume an event and return an appropriate error
            DeEvent::End(_) | DeEvent::Eof => self.deserialize_unit(visitor),
            _ => self.deserialize_string(visitor),
        }
    }
}

/// An accessor to sequence elements forming a value for top-level sequence of XML
/// elements.
///
/// Technically, multiple top-level elements violates XML rule of only one top-level
/// element, but we consider this as several concatenated XML documents.
impl<'de, 'a, R> SeqAccess<'de> for &'a mut Deserializer<'de, R>
where
    R: XmlRead<'de>,
{
    type Error = DeError;

    fn next_element_seed<T>(&mut self, seed: T) -> Result<Option<T::Value>, Self::Error>
    where
        T: DeserializeSeed<'de>,
    {
        match self.peek()? {
            DeEvent::Eof => Ok(None),

            // Start(tag), End(tag), Text, CData
            _ => seed.deserialize(&mut **self).map(Some),
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////

/// Trait used by the deserializer for iterating over input. This is manually
/// "specialized" for iterating over `&[u8]`.
///
/// You do not need to implement this trait, it is needed to abstract from
/// [borrowing](SliceReader) and [copying](IoReader) data sources and reuse code in
/// deserializer
pub trait XmlRead<'i> {
    /// Return an input-borrowing event.
    fn next(&mut self) -> Result<DeEvent<'i>, DeError>;

    /// Skips until end element is found. Unlike `next()` it will not allocate
    /// when it cannot satisfy the lifetime.
    fn read_to_end(&mut self, name: QName) -> Result<(), DeError>;

    /// A copy of the reader's decoder used to decode strings.
    fn decoder(&self) -> Decoder;
}

/// XML input source that reads from a std::io input stream.
///
/// You cannot create it, it is created automatically when you call
/// [`Deserializer::from_reader`]
pub struct IoReader<R: BufRead> {
    reader: Reader<R>,
    buf: Vec<u8>,
}

impl<'i, R: BufRead> XmlRead<'i> for IoReader<R> {
    fn next(&mut self) -> Result<DeEvent<'static>, DeError> {
        let event = loop {
            let e = self.reader.read_event_into(&mut self.buf)?;
            match e {
                Event::Start(e) => break Ok(DeEvent::Start(e.into_owned())),
                Event::End(e) => break Ok(DeEvent::End(e.into_owned())),
                Event::Text(e) => break Ok(DeEvent::Text(e.into_owned())),
                Event::CData(e) => break Ok(DeEvent::CData(e.into_owned())),
                Event::Eof => break Ok(DeEvent::Eof),

                _ => self.buf.clear(),
            }
        };

        self.buf.clear();

        event
    }

    fn read_to_end(&mut self, name: QName) -> Result<(), DeError> {
        match self.reader.read_to_end_into(name, &mut self.buf) {
            Err(Error::UnexpectedEof(_)) => Err(DeError::UnexpectedEof),
            Err(e) => Err(e.into()),
            Ok(_) => Ok(()),
        }
    }

    fn decoder(&self) -> Decoder {
        self.reader.decoder()
    }
}

/// XML input source that reads from a slice of bytes and can borrow from it.
///
/// You cannot create it, it is created automatically when you call
/// [`Deserializer::from_str`].
pub struct SliceReader<'de> {
    reader: Reader<&'de [u8]>,
}

impl<'de> XmlRead<'de> for SliceReader<'de> {
    fn next(&mut self) -> Result<DeEvent<'de>, DeError> {
        loop {
            let e = self.reader.read_event()?;
            match e {
                Event::Start(e) => break Ok(DeEvent::Start(e)),
                Event::End(e) => break Ok(DeEvent::End(e)),
                Event::Text(e) => break Ok(DeEvent::Text(e)),
                Event::CData(e) => break Ok(DeEvent::CData(e)),
                Event::Eof => break Ok(DeEvent::Eof),

                _ => (),
            }
        }
    }

    fn read_to_end(&mut self, name: QName) -> Result<(), DeError> {
        match self.reader.read_to_end(name) {
            Err(Error::UnexpectedEof(_)) => Err(DeError::UnexpectedEof),
            Err(e) => Err(e.into()),
            Ok(_) => Ok(()),
        }
    }

    fn decoder(&self) -> Decoder {
        self.reader.decoder()
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use pretty_assertions::assert_eq;

    #[cfg(feature = "overlapped-lists")]
    mod skip {
        use super::*;
        use crate::de::DeEvent::*;
        use crate::events::{BytesEnd, BytesText};
        use pretty_assertions::assert_eq;

        /// Checks that `peek()` and `read()` behaves correctly after `skip()`
        #[test]
        fn read_and_peek() {
            let mut de = Deserializer::from_str(
                r#"
                <root>
                    <inner>
                        text
                        <inner/>
                    </inner>
                    <next/>
                    <target/>
                </root>
                "#,
            );

            // Initial conditions - both are empty
            assert_eq!(de.read, vec![]);
            assert_eq!(de.write, vec![]);

            assert_eq!(de.next().unwrap(), Start(BytesStart::new("root")));
            assert_eq!(de.peek().unwrap(), &Start(BytesStart::new("inner")));

            // Mark that start_replay() should begin replay from this point
            let checkpoint = de.skip_checkpoint();
            assert_eq!(checkpoint, 0);

            // Should skip first <inner> tree
            de.skip().unwrap();
            assert_eq!(de.read, vec![]);
            assert_eq!(
                de.write,
                vec![
                    Start(BytesStart::new("inner")),
                    Text(BytesText::from_escaped("text")),
                    Start(BytesStart::new("inner")),
                    End(BytesEnd::new("inner")),
                    End(BytesEnd::new("inner")),
                ]
            );

            // Consume <next/>. Now unconsumed XML looks like:
            //
            //   <inner>
            //     text
            //     <inner/>
            //   </inner>
            //   <target/>
            // </root>
            assert_eq!(de.next().unwrap(), Start(BytesStart::new("next")));
            assert_eq!(de.next().unwrap(), End(BytesEnd::new("next")));

            // We finish writing. Next call to `next()` should start replay that messages:
            //
            //   <inner>
            //     text
            //     <inner/>
            //   </inner>
            //
            // and after that stream that messages:
            //
            //   <target/>
            // </root>
            de.start_replay(checkpoint);
            assert_eq!(
                de.read,
                vec![
                    Start(BytesStart::new("inner")),
                    Text(BytesText::from_escaped("text")),
                    Start(BytesStart::new("inner")),
                    End(BytesEnd::new("inner")),
                    End(BytesEnd::new("inner")),
                ]
            );
            assert_eq!(de.write, vec![]);
            assert_eq!(de.next().unwrap(), Start(BytesStart::new("inner")));

            // Mark that start_replay() should begin replay from this point
            let checkpoint = de.skip_checkpoint();
            assert_eq!(checkpoint, 0);

            // Skip `$text` node and consume <inner/> after it
            de.skip().unwrap();
            assert_eq!(
                de.read,
                vec![
                    Start(BytesStart::new("inner")),
                    End(BytesEnd::new("inner")),
                    End(BytesEnd::new("inner")),
                ]
            );
            assert_eq!(
                de.write,
                vec![
                    // This comment here to keep the same formatting of both arrays
                    // otherwise rustfmt suggest one-line it
                    Text(BytesText::from_escaped("text")),
                ]
            );

            assert_eq!(de.next().unwrap(), Start(BytesStart::new("inner")));
            assert_eq!(de.next().unwrap(), End(BytesEnd::new("inner")));

            // We finish writing. Next call to `next()` should start replay messages:
            //
            //     text
            //   </inner>
            //
            // and after that stream that messages:
            //
            //   <target/>
            // </root>
            de.start_replay(checkpoint);
            assert_eq!(
                de.read,
                vec![
                    Text(BytesText::from_escaped("text")),
                    End(BytesEnd::new("inner")),
                ]
            );
            assert_eq!(de.write, vec![]);
            assert_eq!(de.next().unwrap(), Text(BytesText::from_escaped("text")));
            assert_eq!(de.next().unwrap(), End(BytesEnd::new("inner")));
            assert_eq!(de.next().unwrap(), Start(BytesStart::new("target")));
            assert_eq!(de.next().unwrap(), End(BytesEnd::new("target")));
            assert_eq!(de.next().unwrap(), End(BytesEnd::new("root")));
            assert_eq!(de.next().unwrap(), Eof);
        }

        /// Checks that `read_to_end()` behaves correctly after `skip()`
        #[test]
        fn read_to_end() {
            let mut de = Deserializer::from_str(
                r#"
                <root>
                    <skip>
                        text
                        <skip/>
                    </skip>
                    <target>
                        <target/>
                    </target>
                </root>
                "#,
            );

            // Initial conditions - both are empty
            assert_eq!(de.read, vec![]);
            assert_eq!(de.write, vec![]);

            assert_eq!(de.next().unwrap(), Start(BytesStart::new("root")));

            // Mark that start_replay() should begin replay from this point
            let checkpoint = de.skip_checkpoint();
            assert_eq!(checkpoint, 0);

            // Skip the <skip> tree
            de.skip().unwrap();
            assert_eq!(de.read, vec![]);
            assert_eq!(
                de.write,
                vec![
                    Start(BytesStart::new("skip")),
                    Text(BytesText::from_escaped("text")),
                    Start(BytesStart::new("skip")),
                    End(BytesEnd::new("skip")),
                    End(BytesEnd::new("skip")),
                ]
            );

            // Drop all events thet represents <target> tree. Now unconsumed XML looks like:
            //
            //   <skip>
            //     text
            //     <skip/>
            //   </skip>
            // </root>
            assert_eq!(de.next().unwrap(), Start(BytesStart::new("target")));
            de.read_to_end(QName(b"target")).unwrap();
            assert_eq!(de.read, vec![]);
            assert_eq!(
                de.write,
                vec![
                    Start(BytesStart::new("skip")),
                    Text(BytesText::from_escaped("text")),
                    Start(BytesStart::new("skip")),
                    End(BytesEnd::new("skip")),
                    End(BytesEnd::new("skip")),
                ]
            );

            // We finish writing. Next call to `next()` should start replay that messages:
            //
            //   <skip>
            //     text
            //     <skip/>
            //   </skip>
            //
            // and after that stream that messages:
            //
            // </root>
            de.start_replay(checkpoint);
            assert_eq!(
                de.read,
                vec![
                    Start(BytesStart::new("skip")),
                    Text(BytesText::from_escaped("text")),
                    Start(BytesStart::new("skip")),
                    End(BytesEnd::new("skip")),
                    End(BytesEnd::new("skip")),
                ]
            );
            assert_eq!(de.write, vec![]);

            assert_eq!(de.next().unwrap(), Start(BytesStart::new("skip")));
            de.read_to_end(QName(b"skip")).unwrap();

            assert_eq!(de.next().unwrap(), End(BytesEnd::new("root")));
            assert_eq!(de.next().unwrap(), Eof);
        }

        /// Checks that replay replayes only part of events
        /// Test for https://github.com/tafia/quick-xml/issues/435
        #[test]
        fn partial_replay() {
            let mut de = Deserializer::from_str(
                r#"
                <root>
                    <skipped-1/>
                    <skipped-2/>
                    <inner>
                        <skipped-3/>
                        <skipped-4/>
                        <target-2/>
                    </inner>
                    <target-1/>
                </root>
                "#,
            );

            // Initial conditions - both are empty
            assert_eq!(de.read, vec![]);
            assert_eq!(de.write, vec![]);

            assert_eq!(de.next().unwrap(), Start(BytesStart::new("root")));

            // start_replay() should start replay from this point
            let checkpoint1 = de.skip_checkpoint();
            assert_eq!(checkpoint1, 0);

            // Should skip first and second <skipped-N/> elements
            de.skip().unwrap(); // skipped-1
            de.skip().unwrap(); // skipped-2
            assert_eq!(de.read, vec![]);
            assert_eq!(
                de.write,
                vec![
                    Start(BytesStart::new("skipped-1")),
                    End(BytesEnd::new("skipped-1")),
                    Start(BytesStart::new("skipped-2")),
                    End(BytesEnd::new("skipped-2")),
                ]
            );

            ////////////////////////////////////////////////////////////////////////////////////////

            assert_eq!(de.next().unwrap(), Start(BytesStart::new("inner")));
            assert_eq!(de.peek().unwrap(), &Start(BytesStart::new("skipped-3")));
            assert_eq!(
                de.read,
                vec![
                    // This comment here to keep the same formatting of both arrays
                    // otherwise rustfmt suggest one-line it
                    Start(BytesStart::new("skipped-3")),
                ]
            );
            assert_eq!(
                de.write,
                vec![
                    Start(BytesStart::new("skipped-1")),
                    End(BytesEnd::new("skipped-1")),
                    Start(BytesStart::new("skipped-2")),
                    End(BytesEnd::new("skipped-2")),
                ]
            );

            // start_replay() should start replay from this point
            let checkpoint2 = de.skip_checkpoint();
            assert_eq!(checkpoint2, 4);

            // Should skip third and forth <skipped-N/> elements
            de.skip().unwrap(); // skipped-3
            de.skip().unwrap(); // skipped-4
            assert_eq!(de.read, vec![]);
            assert_eq!(
                de.write,
                vec![
                    // checkpoint 1
                    Start(BytesStart::new("skipped-1")),
                    End(BytesEnd::new("skipped-1")),
                    Start(BytesStart::new("skipped-2")),
                    End(BytesEnd::new("skipped-2")),
                    // checkpoint 2
                    Start(BytesStart::new("skipped-3")),
                    End(BytesEnd::new("skipped-3")),
                    Start(BytesStart::new("skipped-4")),
                    End(BytesEnd::new("skipped-4")),
                ]
            );
            assert_eq!(de.next().unwrap(), Start(BytesStart::new("target-2")));
            assert_eq!(de.next().unwrap(), End(BytesEnd::new("target-2")));
            assert_eq!(de.peek().unwrap(), &End(BytesEnd::new("inner")));
            assert_eq!(
                de.read,
                vec![
                    // This comment here to keep the same formatting of both arrays
                    // otherwise rustfmt suggest one-line it
                    End(BytesEnd::new("inner")),
                ]
            );
            assert_eq!(
                de.write,
                vec![
                    // checkpoint 1
                    Start(BytesStart::new("skipped-1")),
                    End(BytesEnd::new("skipped-1")),
                    Start(BytesStart::new("skipped-2")),
                    End(BytesEnd::new("skipped-2")),
                    // checkpoint 2
                    Start(BytesStart::new("skipped-3")),
                    End(BytesEnd::new("skipped-3")),
                    Start(BytesStart::new("skipped-4")),
                    End(BytesEnd::new("skipped-4")),
                ]
            );

            // Start replay events from checkpoint 2
            de.start_replay(checkpoint2);
            assert_eq!(
                de.read,
                vec![
                    Start(BytesStart::new("skipped-3")),
                    End(BytesEnd::new("skipped-3")),
                    Start(BytesStart::new("skipped-4")),
                    End(BytesEnd::new("skipped-4")),
                    End(BytesEnd::new("inner")),
                ]
            );
            assert_eq!(
                de.write,
                vec![
                    Start(BytesStart::new("skipped-1")),
                    End(BytesEnd::new("skipped-1")),
                    Start(BytesStart::new("skipped-2")),
                    End(BytesEnd::new("skipped-2")),
                ]
            );

            // Replayed events
            assert_eq!(de.next().unwrap(), Start(BytesStart::new("skipped-3")));
            assert_eq!(de.next().unwrap(), End(BytesEnd::new("skipped-3")));
            assert_eq!(de.next().unwrap(), Start(BytesStart::new("skipped-4")));
            assert_eq!(de.next().unwrap(), End(BytesEnd::new("skipped-4")));

            assert_eq!(de.next().unwrap(), End(BytesEnd::new("inner")));
            assert_eq!(de.read, vec![]);
            assert_eq!(
                de.write,
                vec![
                    Start(BytesStart::new("skipped-1")),
                    End(BytesEnd::new("skipped-1")),
                    Start(BytesStart::new("skipped-2")),
                    End(BytesEnd::new("skipped-2")),
                ]
            );

            ////////////////////////////////////////////////////////////////////////////////////////

            // New events
            assert_eq!(de.next().unwrap(), Start(BytesStart::new("target-1")));
            assert_eq!(de.next().unwrap(), End(BytesEnd::new("target-1")));

            assert_eq!(de.read, vec![]);
            assert_eq!(
                de.write,
                vec![
                    Start(BytesStart::new("skipped-1")),
                    End(BytesEnd::new("skipped-1")),
                    Start(BytesStart::new("skipped-2")),
                    End(BytesEnd::new("skipped-2")),
                ]
            );

            // Start replay events from checkpoint 1
            de.start_replay(checkpoint1);
            assert_eq!(
                de.read,
                vec![
                    Start(BytesStart::new("skipped-1")),
                    End(BytesEnd::new("skipped-1")),
                    Start(BytesStart::new("skipped-2")),
                    End(BytesEnd::new("skipped-2")),
                ]
            );
            assert_eq!(de.write, vec![]);

            // Replayed events
            assert_eq!(de.next().unwrap(), Start(BytesStart::new("skipped-1")));
            assert_eq!(de.next().unwrap(), End(BytesEnd::new("skipped-1")));
            assert_eq!(de.next().unwrap(), Start(BytesStart::new("skipped-2")));
            assert_eq!(de.next().unwrap(), End(BytesEnd::new("skipped-2")));

            assert_eq!(de.read, vec![]);
            assert_eq!(de.write, vec![]);

            // New events
            assert_eq!(de.next().unwrap(), End(BytesEnd::new("root")));
            assert_eq!(de.next().unwrap(), Eof);
        }

        /// Checks that limiting buffer size works correctly
        #[test]
        fn limit() {
            use serde::Deserialize;

            #[derive(Debug, Deserialize)]
            #[allow(unused)]
            struct List {
                item: Vec<()>,
            }

            let mut de = Deserializer::from_str(
                r#"
                <any-name>
                    <item/>
                    <another-item>
                        <some-element>with text</some-element>
                        <yet-another-element/>
                    </another-item>
                    <item/>
                    <item/>
                </any-name>
                "#,
            );
            de.event_buffer_size(NonZeroUsize::new(3));

            match List::deserialize(&mut de) {
                Err(DeError::TooManyEvents(count)) => assert_eq!(count.get(), 3),
                e => panic!("Expected `Err(TooManyEvents(3))`, but found {:?}", e),
            }
        }

        /// Without handling Eof in `skip` this test failed with memory allocation
        #[test]
        fn invalid_xml() {
            use crate::de::DeEvent::*;

            let mut de = Deserializer::from_str("<root>");

            // Cache all events
            let checkpoint = de.skip_checkpoint();
            de.skip().unwrap();
            de.start_replay(checkpoint);
            assert_eq!(de.read, vec![Start(BytesStart::new("root")), Eof]);
        }
    }

    mod read_to_end {
        use super::*;
        use crate::de::DeEvent::*;
        use pretty_assertions::assert_eq;

        #[test]
        fn complex() {
            let mut de = Deserializer::from_str(
                r#"
                <root>
                    <tag a="1"><tag>text</tag>content</tag>
                    <tag a="2"><![CDATA[cdata content]]></tag>
                    <self-closed/>
                </root>
                "#,
            );

            assert_eq!(de.next().unwrap(), Start(BytesStart::new("root")));

            assert_eq!(
                de.next().unwrap(),
                Start(BytesStart::from_content(r#"tag a="1""#, 3))
            );
            assert_eq!(de.read_to_end(QName(b"tag")).unwrap(), ());

            assert_eq!(
                de.next().unwrap(),
                Start(BytesStart::from_content(r#"tag a="2""#, 3))
            );
            assert_eq!(de.next().unwrap(), CData(BytesCData::new("cdata content")));
            assert_eq!(de.next().unwrap(), End(BytesEnd::new("tag")));

            assert_eq!(de.next().unwrap(), Start(BytesStart::new("self-closed")));
            assert_eq!(de.read_to_end(QName(b"self-closed")).unwrap(), ());

            assert_eq!(de.next().unwrap(), End(BytesEnd::new("root")));
            assert_eq!(de.next().unwrap(), Eof);
        }

        #[test]
        fn invalid_xml() {
            let mut de = Deserializer::from_str("<tag><tag></tag>");

            assert_eq!(de.next().unwrap(), Start(BytesStart::new("tag")));
            assert_eq!(de.peek().unwrap(), &Start(BytesStart::new("tag")));

            match de.read_to_end(QName(b"tag")) {
                Err(DeError::UnexpectedEof) => (),
                x => panic!("Expected `Err(UnexpectedEof)`, but found {:?}", x),
            }
            assert_eq!(de.next().unwrap(), Eof);
        }
    }

    #[test]
    fn borrowing_reader_parity() {
        let s = r#"
            <item name="hello" source="world.rs">Some text</item>
            <item2/>
            <item3 value="world" />
        "#;

        let mut reader1 = IoReader {
            reader: Reader::from_reader(s.as_bytes()),
            buf: Vec::new(),
        };
        let mut reader2 = SliceReader {
            reader: Reader::from_str(s),
        };

        loop {
            let event1 = reader1.next().unwrap();
            let event2 = reader2.next().unwrap();

            if let (DeEvent::Eof, DeEvent::Eof) = (&event1, &event2) {
                break;
            }

            assert_eq!(event1, event2);
        }
    }

    #[test]
    fn borrowing_reader_events() {
        let s = r#"
            <item name="hello" source="world.rs">Some text</item>
            <item2></item2>
            <item3/>
            <item4 value="world" />
        "#;

        let mut reader = SliceReader {
            reader: Reader::from_str(s),
        };

        reader
            .reader
            .trim_text(true)
            .expand_empty_elements(true)
            .check_end_names(true);

        let mut events = Vec::new();

        loop {
            let event = reader.next().unwrap();
            if let DeEvent::Eof = event {
                break;
            }
            events.push(event);
        }

        use crate::de::DeEvent::*;

        assert_eq!(
            events,
            vec![
                Start(BytesStart::from_content(
                    r#"item name="hello" source="world.rs""#,
                    4
                )),
                Text(BytesText::from_escaped("Some text")),
                End(BytesEnd::new("item")),
                Start(BytesStart::from_content("item2", 5)),
                End(BytesEnd::new("item2")),
                Start(BytesStart::from_content("item3", 5)),
                End(BytesEnd::new("item3")),
                Start(BytesStart::from_content(r#"item4 value="world" "#, 5)),
                End(BytesEnd::new("item4")),
            ]
        )
    }

    /// Ensures, that [`Deserializer::read_string()`] never can get an `End` event,
    /// because parser reports error early
    #[test]
    fn read_string() {
        match from_str::<String>(r#"</root>"#) {
            Err(DeError::InvalidXml(Error::EndEventMismatch { expected, found })) => {
                assert_eq!(expected, "");
                assert_eq!(found, "root");
            }
            x => panic!(
                r#"Expected `Err(InvalidXml(EndEventMismatch("", "root")))`, but found {:?}"#,
                x
            ),
        }

        let s: String = from_str(r#"<root></root>"#).unwrap();
        assert_eq!(s, "");

        match from_str::<String>(r#"<root></other>"#) {
            Err(DeError::InvalidXml(Error::EndEventMismatch { expected, found })) => {
                assert_eq!(expected, "root");
                assert_eq!(found, "other");
            }
            x => panic!(
                r#"Expected `Err(InvalidXml(EndEventMismatch("root", "other")))`, but found {:?}"#,
                x
            ),
        }
    }
}
