//! Working with custom sections.

use crate::passes::Roots;
use crate::tombstone_arena::{Id, Tombstone, TombstoneArena};
use crate::CodeTransform;
use crate::IdsToIndices;
use std::any::Any;
use std::borrow::Cow;
use std::fmt::{self, Debug};
use std::hash::{Hash, Hasher};
use std::marker::PhantomData;

/// A trait for implementing [custom
/// sections](https://webassembly.github.io/spec/core/binary/modules.html#binary-customsec).
///
/// Custom sections are added to a `walrus::Module` via
/// `my_module.custom_sections.add(my_custom_section)`.
pub trait CustomSection: WalrusAny + Debug + Send + Sync {
    /// Get this custom section's name.
    ///
    /// For example ".debug_info" for one of the DWARF custom sections or "name"
    /// for the [names custom
    /// section](https://webassembly.github.io/spec/core/appendix/custom.html#name-section).
    fn name(&self) -> &str;

    /// Get the data payload for this custom section.
    ///
    /// This should *not* include the section header with id=0, the custom
    /// section's name, or the count of how many bytes are in the
    /// payload. `walrus` will handle these for you.
    fn data(&self, ids_to_indices: &IdsToIndices) -> Cow<[u8]>;

    /// Add any core wasm roots to the provided `roots` argument.
    ///
    /// This function will add any referenced core wasm items into the `Roots`
    /// array provided.
    ///
    /// The default provided method does nothing.
    fn add_gc_roots(&self, _roots: &mut Roots) {}

    /// Apply the given code transformations to this custom section.
    ///
    /// If the module was not configured with `preserve_code_transform = true`,
    /// then this method is never called.
    ///
    /// This method is called after we have emitted the non-custom Wasm
    /// sections, just before a custom section's data is emitted into the Wasm
    /// binary. If this custom section references offsets in the Wasm code, this
    /// is a chance to update them so they are valid for the new, transformed
    /// Wasm code that is being emitted.
    ///
    /// For example, DWARF debug info references Wasm instructions via offsets
    /// into the code section, and we can use these transforms to fix those
    /// offsets after having transformed various functions and instructions.
    ///
    /// The default provided method does nothing.
    fn apply_code_transform(&mut self, transform: &CodeTransform) {
        let _ = transform;
    }
}

/// A wrapper trait around `any` but implemented for all types that already
/// implement `Any`. You shouldn't need to implement this type yourself as it
/// should automatically be implemented.
pub trait WalrusAny: Any + Send + Sync {
    #[doc(hidden)]
    fn walrus_into_any(self: Box<Self>) -> Box<dyn Any + Send + 'static>;
    #[doc(hidden)]
    fn walrus_as_any(&self) -> &(dyn Any + Send + Sync);
    #[doc(hidden)]
    fn walrus_as_any_mut(&mut self) -> &mut (dyn Any + Send + Sync);
}

impl<T: Any + Send + Sync> WalrusAny for T {
    fn walrus_into_any(self: Box<Self>) -> Box<dyn Any + Send + 'static> {
        self
    }
    fn walrus_as_any(&self) -> &(dyn Any + Send + Sync) {
        self
    }
    fn walrus_as_any_mut(&mut self) -> &mut (dyn Any + Send + Sync) {
        self
    }
}

impl dyn CustomSection {
    /// Convert this custom section to `Box<Any>` to do dynamic downcasting
    pub fn into_any(self: Box<Self>) -> Box<dyn Any + Send + 'static> {
        self.walrus_into_any()
    }

    /// Convert this custom section to `&Any` to do dynamic downcasting
    pub fn as_any(&self) -> &(dyn Any + Send + Sync) {
        self.walrus_as_any()
    }

    /// Convert this custom section to `&mut Any` to do dynamic downcasting
    pub fn as_any_mut(&mut self) -> &mut (dyn Any + Send + Sync) {
        self.walrus_as_any_mut()
    }
}

/// A raw, unparsed custom section.
#[derive(Clone, Debug, PartialEq, Eq)]
pub struct RawCustomSection {
    /// This custom section's name.
    pub name: String,

    /// This custom section's raw data.
    pub data: Vec<u8>,
}

impl CustomSection for RawCustomSection {
    fn name(&self) -> &str {
        &self.name
    }

    fn data(&self, _: &IdsToIndices) -> Cow<[u8]> {
        self.data.as_slice().into()
    }
}

/// A common trait for custom section identifiers.
///
/// Used in the `ModuleCustomSections::get` family of methods to perform type
/// conversions from `dyn CustomSection` trait objects into the concrete
/// `Self::CustomSection` type instance.
///
/// You shouldn't implement this yourself. Instead use `TypedCustomSectionId<T>`
/// or `UntypedCustomSectionId`.
pub trait CustomSectionId {
    /// The concrete custom section type that this id gets out of a
    /// `ModuleCustomSections`.
    type CustomSection: ?Sized;

    #[doc(hidden)]
    fn into_inner_id(self) -> Id<Option<Box<dyn CustomSection>>>;
    #[doc(hidden)]
    fn section(s: &dyn CustomSection) -> Option<&Self::CustomSection>;
    #[doc(hidden)]
    fn section_mut(s: &mut dyn CustomSection) -> Option<&mut Self::CustomSection>;
    #[doc(hidden)]
    fn section_box(s: Box<dyn CustomSection>) -> Option<Box<Self::CustomSection>>;
}

/// The id of some `CustomSection` instance in a `ModuleCustomSections`.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub struct UntypedCustomSectionId(Id<Option<Box<dyn CustomSection>>>);

impl CustomSectionId for UntypedCustomSectionId {
    type CustomSection = dyn CustomSection;

    fn into_inner_id(self) -> Id<Option<Box<dyn CustomSection>>> {
        self.0
    }

    fn section(s: &dyn CustomSection) -> Option<&dyn CustomSection> {
        Some(s)
    }

    fn section_mut(s: &mut dyn CustomSection) -> Option<&mut dyn CustomSection> {
        Some(s)
    }

    fn section_box(s: Box<dyn CustomSection>) -> Option<Box<dyn CustomSection>> {
        Some(s)
    }
}

/// The id of a `CustomSection` instance with a statically-known type in a
/// `ModuleCustomSections`.
pub struct TypedCustomSectionId<T>
where
    T: CustomSection,
{
    id: UntypedCustomSectionId,
    _phantom: PhantomData<T>,
}

impl<T> Debug for TypedCustomSectionId<T>
where
    T: CustomSection,
{
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        f.debug_tuple("TypedCustomSectionId")
            .field(&self.id)
            .finish()
    }
}

impl<T> Clone for TypedCustomSectionId<T>
where
    T: CustomSection,
{
    fn clone(&self) -> Self {
        *self
    }
}

impl<T> Copy for TypedCustomSectionId<T> where T: CustomSection {}

impl<T> PartialEq for TypedCustomSectionId<T>
where
    T: CustomSection,
{
    fn eq(&self, rhs: &Self) -> bool {
        self.id == rhs.id
    }
}

impl<T> Eq for TypedCustomSectionId<T> where T: CustomSection {}

impl<T> Hash for TypedCustomSectionId<T>
where
    T: CustomSection,
{
    fn hash<H: Hasher>(&self, state: &mut H) {
        self.id.hash(state);
    }
}

impl<T> CustomSectionId for TypedCustomSectionId<T>
where
    T: CustomSection,
{
    type CustomSection = T;

    fn into_inner_id(self) -> Id<Option<Box<dyn CustomSection>>> {
        self.id.0
    }

    fn section(s: &dyn CustomSection) -> Option<&T> {
        s.as_any().downcast_ref::<T>()
    }

    fn section_mut(s: &mut dyn CustomSection) -> Option<&mut T> {
        s.as_any_mut().downcast_mut::<T>()
    }

    fn section_box(s: Box<dyn CustomSection>) -> Option<Box<T>> {
        s.into_any().downcast().ok()
    }
}

impl<T> From<TypedCustomSectionId<T>> for UntypedCustomSectionId
where
    T: CustomSection,
{
    #[inline]
    fn from(a: TypedCustomSectionId<T>) -> Self {
        a.id
    }
}

impl Tombstone for Option<Box<dyn CustomSection>> {
    fn on_delete(&mut self) {
        *self = None;
    }
}

/// A collection of custom sections inside a Wasm module.
///
/// To add parse and emit your own custom section:
///
/// * Define a `MyCustomSection` type to represent your custom section.
///
/// * Implement the `walrus::CustomSection` trait for your `MyCustomSection`
///   type.
///
/// * When working with a `walrus::Module` named `my_module`, use
///   `my_module.customs.take_raw("my_custom_section_name")` to take ownership
///   of the raw custom section if it is present in the Wasm module.
///
/// * Parse that into your own `MyCustomSection` type in whatever way is
///   appropriate.
///
/// * Do whatever kinds of inspection and manipulation of your `MyCustomSection`
///   type you need to do.
///
/// * Use `my_module.customs.add(my_custom_section)` to add the custom section
///   back into the module, so `walrus` can emit the processed/updated version
///   of the custom section.
#[derive(Debug, Default)]
pub struct ModuleCustomSections {
    arena: TombstoneArena<Option<Box<dyn CustomSection>>>,
}

impl ModuleCustomSections {
    /// Add a new custom section to the module.
    pub fn add<T>(&mut self, custom_section: T) -> TypedCustomSectionId<T>
    where
        T: CustomSection,
    {
        let id = self
            .arena
            .alloc(Some(Box::new(custom_section) as Box<dyn CustomSection>));
        TypedCustomSectionId {
            id: UntypedCustomSectionId(id),
            _phantom: PhantomData,
        }
    }

    /// Remove a custom section from the module.
    pub fn delete<I>(&mut self, id: I) -> Option<Box<I::CustomSection>>
    where
        I: CustomSectionId,
    {
        let id = id.into_inner_id();
        let ret = self.arena.get_mut(id)?.take()?;
        self.arena.delete(id);
        I::section_box(ret)
    }

    /// Take a raw, unparsed custom section out of this module.
    pub fn remove_raw(&mut self, name: &str) -> Option<RawCustomSection> {
        let id = self
            .arena
            .iter()
            .filter(|(_id, s)| {
                if let Some(s) = s {
                    s.as_any().is::<RawCustomSection>() && s.name() == name
                } else {
                    false
                }
            })
            .map(|(id, _)| id)
            .next()?;
        let section = self.arena[id].take().unwrap();
        self.arena.delete(id);
        let raw = section.into_any().downcast::<RawCustomSection>().unwrap();
        Some(*raw)
    }

    /// Try and get a shared reference to a custom section that is in this
    /// `ModuleCustomSections`.
    ///
    /// Returns `None` if the section associated with the given `id` has been
    /// taken or deleted, or if it is present but not an instance of
    /// `T::CustomSection`.
    pub fn get<T>(&self, id: T) -> Option<&T::CustomSection>
    where
        T: CustomSectionId,
    {
        self.arena
            .get(id.into_inner_id())
            .and_then(|s| T::section(&**s.as_ref().unwrap()))
    }

    /// Try and get an exclusive reference to a custom section that is in this
    /// `ModuleCustomSections`.
    ///
    /// Returns `None` if the section associated with the given `id` has been
    /// taken or deleted, or if it is present but not an instance of
    /// `T::CustomSection`.
    pub fn get_mut<T>(&mut self, id: T) -> Option<&mut T::CustomSection>
    where
        T: CustomSectionId,
    {
        self.arena
            .get_mut(id.into_inner_id())
            .and_then(|s| T::section_mut(&mut **s.as_mut().unwrap()))
    }

    /// Iterate over shared references to custom sections and their ids.
    pub fn iter(&self) -> impl Iterator<Item = (UntypedCustomSectionId, &dyn CustomSection)> {
        self.arena
            .iter()
            .flat_map(|(id, s)| Some((UntypedCustomSectionId(id), &**s.as_ref()?)))
    }

    /// Iterate over exclusive references to custom sections and their ids.
    pub fn iter_mut(
        &mut self,
    ) -> impl Iterator<Item = (UntypedCustomSectionId, &mut dyn CustomSection)> {
        self.arena
            .iter_mut()
            .flat_map(|(id, s)| Some((UntypedCustomSectionId(id), &mut **s.as_mut()?)))
    }

    /// Remove a custom section (by type) from the module.
    ///
    /// If there are multiple custom sections of the type `T` only the first one
    /// is removed.
    pub fn delete_typed<T>(&mut self) -> Option<Box<T>>
    where
        T: CustomSection,
    {
        let (id, _) = self.iter().find(|(_, s)| s.as_any().is::<T>())?;
        self.delete(id)?.into_any().downcast().ok()
    }

    /// Get a shared reference to a custom section, by type.
    ///
    /// If there are multiple custom sections of the type `T` this returns a
    /// reference to the first one.
    pub fn get_typed<T>(&self) -> Option<&T>
    where
        T: CustomSection,
    {
        self.iter()
            .filter_map(|(_, s)| s.as_any().downcast_ref())
            .next()
    }

    /// Get a mutable reference to a custom section, by type.
    ///
    /// If there are multiple custom sections of the type `T` this returns a
    /// reference to the first one.
    pub fn get_typed_mut<T>(&mut self) -> Option<&mut T>
    where
        T: CustomSection,
    {
        self.iter_mut()
            .filter_map(|(_, s)| s.as_any_mut().downcast_mut())
            .next()
    }
}
