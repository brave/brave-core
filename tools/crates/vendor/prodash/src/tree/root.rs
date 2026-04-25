use std::{
    ops::Deref,
    sync::{atomic::AtomicUsize, Arc, Weak},
};

use parking_lot::Mutex;

use crate::{
    messages::{Message, MessageCopyState, MessageRingBuffer},
    progress::{Id, Key, Task},
    tree::{Item, Root},
};

impl Root {
    /// Create a new tree with default configuration.
    ///
    /// As opposed to [Item](./struct.Item.html) instances, this type can be closed and sent
    /// safely across threads.
    pub fn new() -> Arc<Root> {
        Options::default().into()
    }

    /// Returns the maximum amount of messages we can keep before overwriting older ones.
    pub fn messages_capacity(&self) -> usize {
        self.inner.lock().messages.lock().buf.capacity()
    }

    /// Returns the current amount of `Item`s stored in the tree.
    /// **Note** that this is at most a guess as tasks can be added and removed in parallel.
    pub fn num_tasks(&self) -> usize {
        #[cfg(feature = "progress-tree-hp-hashmap")]
        {
            self.inner.lock().tree.len()
        }
        #[cfg(not(feature = "progress-tree-hp-hashmap"))]
        {
            self.inner.lock().tree.len()
        }
    }

    /// Adds a new child `tree::Item`, whose parent is this instance, with the given `name`.
    ///
    /// This builds a hierarchy of `tree::Item`s, each having their own progress.
    /// Use this method to [track progress](./struct.Item.html) of your first tasks.
    pub fn add_child(&self, name: impl Into<String>) -> Item {
        self.inner.lock().add_child(name)
    }

    /// Adds a new child `tree::Item`, whose parent is this instance, with the given `name` and `id`.
    ///
    /// This builds a hierarchy of `tree::Item`s, each having their own progress.
    /// Use this method to [track progress](./struct.Item.html) of your first tasks.
    pub fn add_child_with_id(&self, name: impl Into<String>, id: Id) -> Item {
        self.inner.lock().add_child_with_id(name, id)
    }

    /// Copy the entire progress tree into the given `out` vector, so that
    /// it can be traversed from beginning to end in order of hierarchy.
    pub fn sorted_snapshot(&self, out: &mut Vec<(Key, Task)>) {
        out.clear();
        #[cfg(feature = "progress-tree-hp-hashmap")]
        out.extend(self.inner.lock().tree.iter().map(|r| (*r.key(), r.value().clone())));
        #[cfg(not(feature = "progress-tree-hp-hashmap"))]
        self.inner.lock().tree.extend_to(out);
        out.sort_by_key(|t| t.0);
    }

    /// Copy all messages from the internal ring buffer into the given `out`
    /// vector. Messages are ordered from oldest to newest.
    pub fn copy_messages(&self, out: &mut Vec<Message>) {
        self.inner.lock().messages.lock().copy_all(out);
    }

    /// Copy only new messages from the internal ring buffer into the given `out`
    /// vector. Messages are ordered from oldest to newest.
    pub fn copy_new_messages(&self, out: &mut Vec<Message>, prev: Option<MessageCopyState>) -> MessageCopyState {
        self.inner.lock().messages.lock().copy_new(out, prev)
    }

    /// Duplicate all content and return it.
    ///
    /// This is an expensive operation, whereas `clone()` is not as it is shallow.
    pub fn deep_clone(&self) -> Arc<Root> {
        Arc::new(Root {
            inner: Mutex::new(self.inner.lock().deep_clone()),
        })
    }
}

/// A way to configure new [`tree::Root`](./tree/struct.Root.html) instances
/// ```rust
/// let tree = prodash::tree::root::Options::default().create();
/// let tree2 = prodash::tree::root::Options { message_buffer_capacity: 100, ..Default::default() }.create();
/// ```
#[derive(Clone, Debug)]
pub struct Options {
    /// The amount of [items][Item] the tree can hold without being forced to allocate.
    pub initial_capacity: usize,
    /// The amount of messages we can hold before we start overwriting old ones.
    pub message_buffer_capacity: usize,
}

impl Options {
    /// Create a new [`Root`](./tree/struct.Root.html) instance from the
    /// configuration within.
    pub fn create(self) -> Root {
        self.into()
    }
}

impl Default for Options {
    fn default() -> Self {
        Options {
            initial_capacity: 100,
            message_buffer_capacity: 20,
        }
    }
}

impl From<Options> for Arc<Root> {
    fn from(opts: Options) -> Self {
        Arc::new(opts.into())
    }
}

impl From<Options> for Root {
    fn from(
        Options {
            initial_capacity,
            message_buffer_capacity,
        }: Options,
    ) -> Self {
        Root {
            inner: Mutex::new(Item {
                highest_child_id: 0,
                value: Arc::new(AtomicUsize::default()),
                key: Key::default(),
                tree: Arc::new(crate::tree::HashMap::with_capacity(initial_capacity)),
                messages: Arc::new(Mutex::new(MessageRingBuffer::with_capacity(message_buffer_capacity))),
            }),
        }
    }
}

impl crate::WeakRoot for Weak<Root> {
    type Root = Arc<Root>;

    fn upgrade(&self) -> Option<Self::Root> {
        Weak::upgrade(self)
    }
}

impl crate::Root for Arc<Root> {
    type WeakRoot = Weak<Root>;

    fn messages_capacity(&self) -> usize {
        self.deref().messages_capacity()
    }

    fn num_tasks(&self) -> usize {
        self.deref().num_tasks()
    }

    fn sorted_snapshot(&self, out: &mut Vec<(Key, Task)>) {
        self.deref().sorted_snapshot(out)
    }

    fn copy_messages(&self, out: &mut Vec<Message>) {
        self.deref().copy_messages(out)
    }

    fn copy_new_messages(&self, out: &mut Vec<Message>, prev: Option<MessageCopyState>) -> MessageCopyState {
        self.deref().copy_new_messages(out, prev)
    }

    fn downgrade(&self) -> Self::WeakRoot {
        Arc::downgrade(self)
    }
}
