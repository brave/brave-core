use crate::{
    Env,
    errors::*,
    objects::{JIterator, JObject, Reference},
};

use std::ops::Deref;

#[cfg(doc)]
use crate::objects::JSet;

crate::bind_java_type! {
    pub JMap => "java.util.Map",
    methods = {
        /// Returns the number of key-value mappings in this map
        ///
        /// If the map contains more than `jint::MAX` mappings, returns `jint::MAX`.
        fn size() -> jint,
        /// Returns `true` if this map contains no key-value mappings
        fn is_empty() -> bool,
        /// Look up the value for a key
        priv fn _get(key: JObject) -> JObject,
        /// Get the value for a key, or return the default value if the key is not present
        ///
        /// # Throws
        ///
        /// - `ClassCastException` - if the key is of an inappropriate type for this map
        /// - `NullPointerException` - if the key is null and this map does not allow null keys
        fn get_or_default(key: JObject, default_value: JObject) -> JObject,
        // Associates the specified value with the specified key in this map
        priv fn _put(key: JObject, value: JObject) -> JObject,
        /// Copies all of the mappings from the specified map to this map
        ///
        /// # Throws
        ///
        /// - `UnsupportedOperationException` - if the putAll operation is not supported by this map
        /// - `ClassCastException` - if a key or value in the specified map is of an inappropriate type for this map
        /// - `NullPointerException` - if the given map is null, or if a key or value in the specified map is null and this map does not allow null keys or values
        /// - `IllegalArgumentException` - if some property of a key or value in the specified map prevents it from being stored by this map
        fn put_all(other_map: JMap),
        /// If the specified key is not already associated with a value, associate it with the given value.
        ///
        /// # Throws
        ///
        /// - `UnsupportedOperationException` - if the `putIfAbsent` operation is not supported by this map
        /// - `ClassCastException` - if the key or value are of an inappropriate type for this map
        /// - `NullPointerException` - if the key or value is null and this map does not allow null keys or values
        /// - `IllegalArgumentException` - if some property of the key or value prevents it from being stored by this map
        fn put_if_absent(key: JObject, value: JObject) -> JObject,
        /// Remove a mapping for a key from the map
        priv fn _remove(key: JObject) -> JObject,
        /// Removes the entry for the specified key only if it is currently mapped to the specified value
        ///
        /// # Throws
        ///
        /// - `UnsupportedOperationException` - if the remove operation is not supported by this map
        /// - `ClassCastException` - if the key is of an inappropriate type for this map
        /// - `NullPointerException` - if the key is null and this map does not allow null keys
        priv fn remove_value {
            name = "remove",
            sig = (key: JObject, value: JObject) -> bool,
        },
        /// Replaces the entry for the specified key only if it is currently mapped to some value
        ///
        /// # Throws
        ///
        /// - `UnsupportedOperationException` - if the replace operation is not supported by this map
        /// - `ClassCastException` - if the key or value are of an inappropriate type for this map
        /// - `NullPointerException` - if the key or value is null and this map does not allow null keys or values
        /// - `IllegalArgumentException` - if some property of the value prevents it from being stored by this map
        fn replace(key: JObject, value: JObject) -> JObject,
        /// Replaces the entry for the specified key only if currently mapped to a given value
        ///
        /// # Throws
        ///
        /// - `UnsupportedOperationException` - if the replace operation is not supported by this map
        /// - `ClassCastException` - if the key or value are of an inappropriate type for this map
        /// - `NullPointerException` - if the key, new_value or old_value is null and this map does not allow null keys or values
        /// - `IllegalArgumentException` - if some property of the value prevents it from being stored by this map
        fn replace_value {
            name = "replace",
            sig = (key: JObject, old_value: JObject, new_value: JObject) -> bool,
        },
        /// Removes all of the mappings from this map
        fn clear(),
        /// Determines if the map contains a mapping for the specified key
        ///
        /// # Throws
        /// - `ClassCastException` - if the key is of an inappropriate type for this map
        /// - `NullPointerException` - if the key is null and this map does not allow null keys
        fn contains_key(key: JObject) -> bool,
        /// Determines if the map maps one or more keys to the specified value
        ///
        /// # Throws
        /// - `ClassCastException` - if the key is of an inappropriate type for this map
        /// - `NullPointerException` - if the key is null and this map does not allow null keys
        fn contains_value(value: JObject) -> bool,
        /// Get a `JSet` view of the mappings contained in this map
        ///
        /// Returns a [JSet] view of the mappings contained in the map, which can be used to iterate over the key/value pairs.
        ///
        /// Also see [JSet::iterator] and [Self::iter]
        fn entry_set() -> JSet,
        /// Get a `JSet` view of the keys contained in this map
        fn key_set() -> JSet,
        /// Get a `JCollection` view of the values contained in this map
        fn values() -> JCollection,
    }
}

impl<'local> JMap<'local> {
    /// Cast a local reference to a `JMap`
    ///
    /// See [`JMap::cast_local`] for more information.
    #[deprecated(
        since = "0.22.0",
        note = "use JMap::cast_local instead or Env::new_cast_local_ref/cast_local/as_cast_local or Env::new_cast_global_ref/cast_global/as_cast_global"
    )]
    pub fn from_env<'any_local>(
        env: &mut Env<'_>,
        obj: impl Reference + Into<JObject<'any_local>> + AsRef<JObject<'any_local>>,
    ) -> Result<JMap<'any_local>> {
        env.cast_local::<JMap>(obj)
    }

    /// Look up the value for a key.
    ///
    /// Returns `Some` if a non-null value is found and `None` if a null pointer
    /// would be returned.
    ///
    /// If the map permits null values, this method cannot distinguish between a
    /// key that is not present and a key that is explicitly mapped to `null`.
    /// In that case, use [JMap::contains_key] to determine if the key is
    /// present.
    ///
    /// # Throws
    ///
    /// - `ClassCastException` - if the key is of an inappropriate type for this map
    /// - `NullPointerException` - if the key is null and this map does not allow null keys
    pub fn get<'env_local>(
        &self,
        env: &mut Env<'env_local>,
        key: &JObject,
    ) -> Result<Option<JObject<'env_local>>> {
        let value = self._get(env, key)?;
        if value.is_null() {
            Ok(None)
        } else {
            Ok(Some(value))
        }
    }

    /// Associates the specified value with the specified key in this map
    ///
    /// Returns `Some` with the old value if the key already existed and `None`
    /// if it's a new key.
    ///
    /// If the map permits null values, a `None` return value could also indicate
    /// that the previous value associated with the key was explicitly `null`.
    ///
    /// # Throws
    ///
    /// - `UnsupportedOperationException` - if the put operation is not supported by this map
    /// - `ClassCastException` - if the key or value are of an inappropriate type
    /// - `NullPointerException` - if the key or value is null and this map does not allow null keys or values
    /// - `IllegalArgumentException` - if some property of the key or value prevents it
    pub fn put<'env_local>(
        &self,
        env: &mut Env<'env_local>,
        key: &JObject,
        value: &JObject,
    ) -> Result<Option<JObject<'env_local>>> {
        let old = self._put(env, key, value)?;
        if old.is_null() {
            Ok(None)
        } else {
            Ok(Some(old))
        }
    }

    /// Remove a mapping for a key from the map
    ///
    /// Returns `Some` with the non-null removed value and `None` if there was no value
    /// for the key, or if the removed value was `null`.
    ///
    /// If the map permits null values, this method cannot distinguish between a
    /// key that was not present and a key that was explicitly mapped to `null`.
    ///
    /// # Throws
    ///
    /// - `UnsupportedOperationException` - if the remove operation is not supported by this map
    /// - `ClassCastException` - if the key is of an inappropriate type for this map
    /// - `NullPointerException` - if the key is null and this map does not allow null keys
    pub fn remove<'env_local>(
        &self,
        env: &mut Env<'env_local>,
        key: &JObject,
    ) -> Result<Option<JObject<'env_local>>> {
        let old = self._remove(env, key)?;
        if old.is_null() {
            Ok(None)
        } else {
            Ok(Some(old))
        }
    }

    /// Get key/value iterator for the map. This is done by getting the
    /// `EntrySet` from java and iterating over it.
    ///
    /// The returned iterator does not implement [`std::iter::Iterator`] and
    /// cannot be used with a `for` loop. This is because its `next` method uses
    /// a `&mut Env` to call the Java iterator. Use a `while let` loop instead:
    ///
    /// ```rust,no_run
    /// # use jni::{errors::Result, Env, objects::{JMap, JObject}};
    /// #
    /// # fn example(env: &mut Env, map: JMap) -> Result<()> {
    /// use jni::objects::IntoAuto as _; // for .auto()
    /// let mut iterator = map.iter(env)?;
    ///
    /// while let Some(entry) = iterator.next(env)? {
    ///     // Wrap as Auto<T> to avoid leaking while iterating
    ///     let key = entry.key(env)?.auto();
    ///     let value = entry.value(env)?.auto();
    ///
    ///     // Do something with `key` and `value` here.
    /// }
    /// # Ok(())
    /// # }
    /// ```
    ///
    /// Each call to `next` creates two new local references. To prevent
    /// excessive memory usage or overflow error, the local references should be
    /// deleted using [`Env::delete_local_ref`] or wrapped with
    /// [`crate::objects::IntoAuto::auto`] before the next loop iteration.
    /// Alternatively, if the map is known to have a small, predictable size,
    /// the loop could be wrapped in [`Env::with_local_frame`] to delete all of
    /// the local references at once.
    pub fn iter<'env_local>(&self, env: &mut Env<'env_local>) -> Result<JMapIter<'env_local>> {
        let set = self.entry_set(env)?;
        let iterator = set.iterator(env)?;

        Ok(JMapIter { iterator })
    }
}

crate::bind_java_type! {
    pub JMapEntry => "java.util.Map$Entry",
    methods = {
        /// Get the key of the map entry by calling the `getKey` method.
        ///
        /// # Throws
        ///
        /// May throw `IllegalStateException` if the entry has been removed from the map (depending on implementation)
        fn key {
            name = "getKey",
            sig = () -> JObject,
        },
        /// Get the value of the map entry by calling the `getValue` method.
        ///
        /// # Throws
        ///
        /// May throw `IllegalStateException` if the entry has been removed from the map (depending on implementation)
        fn value {
            name = "getValue",
            sig = () -> JObject,
        },
        /// Set the value of the map entry by calling the `setValue` method.
        ///
        /// # Throws
        ///
        /// - `UnsupportedOperationException` if the backing map does not support the put operation
        /// - `ClassCastException` if the value is not of a compatible type
        /// - `NullPointerException` if a null value is given and the backing map doesn't allow storing null values
        /// - `IllegalArgumentException` if the values has a property that prevents it from being stored by the backing map
        /// - May throw `IllegalStateException` if the entry has been removed from the map (depending on implementation)
        fn set_value(value: JObject) -> JObject,
    }
}

/// An iterator over the keys and values in a map. See [`JMap::iter`] for more
/// information.
///
/// This is implemented as a thin wrapper over [`JIterator`] and the only
/// difference is that [JMapIter::next] will yield [JMapEntry] values,
/// (avoiding the need for a runtime type check, compared to using
/// [JIterator::next] followed by [`JMapEntry::cast_local`]).
///
/// This derefs to [`JIterator`].
#[derive(Debug)]
pub struct JMapIter<'iter_local> {
    iterator: JIterator<'iter_local>,
}

impl<'local> Deref for JMapIter<'local> {
    type Target = JIterator<'local>;

    fn deref(&self) -> &Self::Target {
        &self.iterator
    }
}

impl<'local> JMapIter<'local> {
    /// Advances the iterator and returns the next key-value pair in the
    /// `java.util.Map`, or `None` if there are no more objects.
    ///
    /// See [`JMap::iter`] for more information.
    ///
    /// This method creates two new local references. To prevent excessive
    /// memory usage or overflow error, the local references should be deleted
    /// using [`Env::delete_local_ref`] or wrapped with
    /// [`crate::objects::IntoAuto::auto`] before the next loop iteration.
    /// Alternatively, if the map is known to have a small, predictable size,
    /// the loop could be wrapped in [`Env::with_local_frame`] to delete all of
    /// the local references at once.
    ///
    /// This method returns:
    ///
    /// * `Ok(Some(_))`: if there was another key-value pair in the map.
    /// * `Ok(None)`: if there are no more key-value pairs in the map.
    /// * `Err(_)`: if there was an error calling the Java method to get the
    ///   next key-value pair.
    ///
    /// This is like [`std::iter::Iterator::next`], but requires a parameter of
    /// type `&mut Env` in order to call into Java.
    pub fn next<'env_local>(
        &mut self,
        env: &mut Env<'env_local>,
    ) -> Result<Option<JMapEntry<'env_local>>> {
        self.iterator.next(env)?.map_or(Ok(None), |entry| {
            // SAFETY: we know that the entrySet iterator will yield Map.Entry values
            // so we can safely downcast without needing a runtime type check
            let entry = unsafe { JMapEntry::from_raw(env, entry.into_raw()) };
            Ok(Some(entry))
        })
    }
}
