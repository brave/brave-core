use crate::messages::MessageRingBuffer;

/// The top-level of the progress tree.
#[derive(Debug)]
pub struct Root {
    pub(crate) inner: parking_lot::Mutex<Item>,
}

/// A `Tree` represents an element of the progress tree.
///
/// It can be used to set progress and send messages.
/// ```rust
/// let tree = prodash::tree::Root::new();
/// let mut progress = tree.add_child("task 1");
///
/// progress.init(Some(10), Some("elements".into()));
/// for p in 0..10 {
///     progress.set(p);
/// }
/// progress.done("great success");
/// let mut  sub_progress = progress.add_child_with_id("sub-task 1", *b"TSK2");
/// sub_progress.init(None, None);
/// sub_progress.set(5);
/// sub_progress.fail("couldn't finish");
/// ```
pub struct Item {
    pub(crate) key: crate::progress::Key,
    pub(crate) value: crate::progress::StepShared,
    pub(crate) highest_child_id: crate::progress::key::Id,
    pub(crate) tree: std::sync::Arc<HashMap<crate::progress::Key, crate::progress::Task>>,
    pub(crate) messages: std::sync::Arc<parking_lot::Mutex<MessageRingBuffer>>,
}

#[cfg(feature = "dashmap")]
type HashMap<K, V> = dashmap::DashMap<K, V>;

#[cfg(not(feature = "dashmap"))]
type HashMap<K, V> = sync::HashMap<K, V>;

#[cfg(not(feature = "dashmap"))]
pub(crate) mod sync {
    pub struct HashMap<K, V>(parking_lot::Mutex<std::collections::HashMap<K, V>>);

    impl<K, V> HashMap<K, V>
    where
        K: Eq + std::hash::Hash,
    {
        pub fn with_capacity(cap: usize) -> Self {
            HashMap(parking_lot::Mutex::new(std::collections::HashMap::with_capacity(cap)))
        }
        pub fn extend_to(&self, out: &mut Vec<(K, V)>)
        where
            K: Clone,
            V: Clone,
        {
            let lock = self.0.lock();
            out.extend(lock.iter().map(|(k, v)| (k.clone(), v.clone())))
        }
        pub fn remove(&self, key: &K) -> Option<V> {
            self.0.lock().remove(key)
        }
        pub fn get<T>(&self, key: &K, cb: impl FnOnce(&V) -> T) -> Option<T> {
            self.0.lock().get(key).map(cb)
        }
        pub fn get_mut<T>(&self, key: &K, cb: impl FnOnce(&mut V) -> T) -> Option<T> {
            self.0.lock().get_mut(key).map(cb)
        }
        pub fn insert(&self, key: K, value: V) {
            self.0.lock().insert(key, value);
        }
        pub fn len(&self) -> usize {
            self.0.lock().len()
        }
        pub fn clone(&self) -> Self
        where
            K: Clone,
            V: Clone,
        {
            HashMap(parking_lot::Mutex::new(self.0.lock().clone()))
        }
    }
}

mod item;
///
pub mod root;

#[cfg(test)]
mod tests;
