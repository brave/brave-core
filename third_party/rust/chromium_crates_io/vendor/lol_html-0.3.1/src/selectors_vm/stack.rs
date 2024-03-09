use super::ast::NthChild;
use super::program::AddressRange;
use super::SelectorState;
use crate::html::{LocalName, Namespace, Tag};
use crate::memory::{LimitedVec, MemoryLimitExceededError, SharedMemoryLimiter};
// use hashbrown for raw entry, switch back to std once it stablizes there
use hashbrown::{hash_map::RawEntryMut, HashMap, HashSet};
use std::fmt::Debug;
use std::hash::{BuildHasher, Hash, Hasher};

#[inline]
fn is_void_element(local_name: &LocalName, enable_esi_tags: bool) -> bool {
    // NOTE: fast path for the most commonly used elements
    if tag_is_one_of!(*local_name, [Div, A, Span, Li]) {
        return false;
    }

    if tag_is_one_of!(
        *local_name,
        [
            Area, Base, Basefont, Bgsound, Br, Col, Embed, Hr, Img, Input, Keygen, Link, Meta,
            Param, Source, Track, Wbr
        ]
    ) {
        return true;
    }

    if enable_esi_tags {
        if let LocalName::Bytes(bytes) = local_name {
            // https://www.w3.org/TR/esi-lang/
            if &**bytes == b"esi:include" || &**bytes == b"esi:comment" {
                return true;
            }
        }
    }

    false
}

pub trait ElementData: Default + 'static {
    type MatchPayload: PartialEq + Eq + Copy + Debug + Hash + 'static;

    fn matched_payload_mut(&mut self) -> &mut HashSet<Self::MatchPayload>;
}

pub enum StackDirective {
    Push,
    PushIfNotSelfClosing,
    PopImmediately,
}

#[derive(Default)]
pub struct ChildCounter {
    cumulative: i32,
}

impl ChildCounter {
    #[inline]
    pub fn new_and_inc() -> Self {
        Self { cumulative: 1 }
    }

    #[inline]
    pub fn inc(&mut self) {
        self.cumulative += 1;
    }

    #[inline]
    pub fn is_nth(&self, nth: NthChild) -> bool {
        nth.has_index(self.cumulative)
    }
}

struct CounterItem {
    /// The counter at this index
    pub counter: ChildCounter,
    /// The index of this counter in the stack
    pub index: usize,
}

struct CounterList {
    items: Vec<CounterItem>,
    // we always have at least one item, an empty list shouldn't exist in the map
    current: CounterItem,
}

impl CounterList {
    pub fn new(start: usize) -> Self {
        Self {
            items: Vec::new(),
            current: CounterItem {
                counter: ChildCounter::new_and_inc(),
                index: start,
            },
        }
    }
}

#[derive(Default)]
/// A more efficient counter that only requires one owned local name to track counters across multiple stack frames
pub struct TypedChildCounterMap(HashMap<LocalName<'static>, CounterList>);

impl TypedChildCounterMap {
    fn hash_name(&self, name: &LocalName) -> u64 {
        let mut hasher = self.0.hasher().build_hasher();
        name.hash(&mut hasher);
        hasher.finish()
    }

    /// Adds a seen child to the map. The index is the level of the item
    pub fn add_child(&mut self, name: &LocalName, index: usize) {
        let hash = self.hash_name(name);
        let entry = self.0.raw_entry_mut().from_hash(hash, |n| name == n);
        match entry {
            RawEntryMut::Vacant(vacant) => {
                vacant.insert_hashed_nocheck(
                    hash,
                    name.clone().into_owned(), // the hash won't change just because we've got ownership
                    CounterList::new(index),
                );
            }
            RawEntryMut::Occupied(mut occupied) => {
                let CounterList { items, current } = occupied.get_mut();
                if current.index == index {
                    current.counter.inc();
                } else {
                    let counter = ChildCounter::new_and_inc();
                    let old = std::mem::replace(current, CounterItem { counter, index });
                    items.push(old);
                }
            }
        }
    }

    #[inline]
    pub fn pop_to(&mut self, index: usize) {
        self.0.retain(|_, v| {
            while v.current.index > index {
                match v.items.pop() {
                    Some(next) => {
                        v.current = next;
                    }
                    None => return false,
                }
            }
            true
        });
    }

    #[inline]
    pub fn get<'a, 'i>(&'a self, name: &LocalName<'i>, index: usize) -> Option<&'i ChildCounter>
    where
        'a: 'i,
    {
        match self.0.get(name) {
            Some(CounterList {
                current:
                    CounterItem {
                        counter,
                        index: current_index,
                    },
                ..
            }) if *current_index == index => Some(counter),
            _ => None,
        }
    }
}

pub struct StackItem<'i, E: ElementData> {
    pub local_name: LocalName<'i>,
    pub element_data: E,
    pub jumps: Vec<AddressRange>,
    pub hereditary_jumps: Vec<AddressRange>,
    pub child_counter: ChildCounter,
    pub has_ancestor_with_hereditary_jumps: bool,
    pub stack_directive: StackDirective,
}

impl<'i, E: ElementData> StackItem<'i, E> {
    #[inline]
    pub fn new(local_name: LocalName<'i>) -> Self {
        StackItem {
            local_name,
            element_data: E::default(),
            jumps: Vec::default(),
            hereditary_jumps: Vec::default(),
            child_counter: Default::default(),
            has_ancestor_with_hereditary_jumps: false,
            stack_directive: StackDirective::Push,
        }
    }

    #[inline]
    pub fn into_owned(self) -> StackItem<'static, E> {
        StackItem {
            local_name: self.local_name.into_owned(),
            element_data: self.element_data,
            jumps: self.jumps,
            hereditary_jumps: self.hereditary_jumps,
            child_counter: self.child_counter,
            has_ancestor_with_hereditary_jumps: self.has_ancestor_with_hereditary_jumps,
            stack_directive: self.stack_directive,
        }
    }
}

pub struct Stack<E: ElementData> {
    /// A counter for root elements
    root_child_counter: ChildCounter,
    /// A typed counter for all elements on all frames. This is optional to indicate if types are actually being counted.
    typed_child_counters: Option<TypedChildCounterMap>,
    items: LimitedVec<StackItem<'static, E>>,
}

impl<E: ElementData> Stack<E> {
    pub fn new(memory_limiter: SharedMemoryLimiter, enable_nth_of_type: bool) -> Self {
        Stack {
            root_child_counter: Default::default(),
            typed_child_counters: if enable_nth_of_type {
                Some(Default::default())
            } else {
                None
            },
            items: LimitedVec::new(memory_limiter),
        }
    }

    /// Adds a child to child counters. Called before pushing the element to the stack.
    pub fn add_child<'i>(&mut self, name: &LocalName<'i>) {
        match self.items.last_mut() {
            Some(last) => &mut last.child_counter,
            None => &mut self.root_child_counter,
        }
        .inc();

        if let Some(counters) = &mut self.typed_child_counters {
            counters.add_child(name, self.items.len());
        }
    }

    pub fn build_state<'a, 'i>(&'a self, name: &LocalName<'i>) -> SelectorState<'i>
    where
        'a: 'i, // 'a outlives 'i, required to downcast 'a lifetimes into 'i
    {
        let cumulative = match self.items.last() {
            Some(last) => &last.child_counter,
            None => &self.root_child_counter,
        };
        SelectorState {
            cumulative,
            typed: self
                .typed_child_counters
                .as_ref()
                .and_then(|f| f.get(name, self.items.len())),
        }
    }

    #[inline]
    pub fn get_stack_directive(
        item: &StackItem<E>,
        ns: Namespace,
        enable_esi_tags: bool,
    ) -> StackDirective {
        if ns == Namespace::Html {
            if is_void_element(&item.local_name, enable_esi_tags) {
                StackDirective::PopImmediately
            } else {
                StackDirective::Push
            }
        } else {
            StackDirective::PushIfNotSelfClosing
        }
    }

    pub fn pop_up_to(&mut self, local_name: LocalName, popped_element_data_handler: impl FnMut(E)) {
        let pop_to_index = self
            .items
            .iter()
            .rposition(|item| item.local_name == local_name);
        if let Some(index) = pop_to_index {
            if let Some(c) = self.typed_child_counters.as_mut() {
                c.pop_to(index)
            }
            self.items
                .drain(index..)
                .map(|i| i.element_data)
                .for_each(popped_element_data_handler)
        }
    }

    #[inline]
    pub fn items(&self) -> &[StackItem<E>] {
        &self.items
    }

    #[inline]
    pub fn current_element_data_mut(&mut self) -> Option<&mut E> {
        self.items.last_mut().map(|i| &mut i.element_data)
    }

    #[inline]
    pub fn push_item(
        &mut self,
        mut item: StackItem<'static, E>,
    ) -> Result<(), MemoryLimitExceededError> {
        if let Some(last) = self.items.last() {
            if last.has_ancestor_with_hereditary_jumps || !last.hereditary_jumps.is_empty() {
                item.has_ancestor_with_hereditary_jumps = true;
            }
        }

        self.items.push(item)?;
        Ok(())
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::memory::MemoryLimiter;
    use encoding_rs::UTF_8;

    #[derive(Default)]
    struct TestElementData(usize);

    impl ElementData for TestElementData {
        type MatchPayload = ();

        fn matched_payload_mut(&mut self) -> &mut HashSet<()> {
            unreachable!();
        }
    }

    fn local_name(name: &'static str) -> LocalName<'static> {
        LocalName::from_str_without_replacements(name, UTF_8).unwrap()
    }

    fn item(name: &'static str, data: usize) -> StackItem<'static, TestElementData> {
        let mut item = StackItem::new(local_name(name));

        item.element_data = TestElementData(data);

        item
    }

    #[test]
    #[allow(clippy::reversed_empty_ranges)]
    fn hereditary_jumps_flag() {
        let mut stack = Stack::new(MemoryLimiter::new_shared(2048), false);

        stack.push_item(item("item1", 0)).unwrap();

        let mut item2 = item("item2", 1);
        item2.hereditary_jumps.push(0..0);
        stack.push_item(item2).unwrap();

        let mut item3 = item("item3", 2);
        item3.hereditary_jumps.push(0..0);
        stack.push_item(item3).unwrap();

        stack.push_item(item("item4", 3)).unwrap();

        assert_eq!(
            stack
                .items()
                .iter()
                .map(|i| i.has_ancestor_with_hereditary_jumps)
                .collect::<Vec<_>>(),
            [false, false, true, true]
        );
    }

    #[test]
    fn pop_up_to() {
        macro_rules! assert_pop_result {
            ($up_to:expr, $expected_unmatched:expr, $expected_items:expr) => {{
                let mut stack = Stack::new(MemoryLimiter::new_shared(2048), false);

                stack.push_item(item("html", 0)).unwrap();
                stack.push_item(item("body", 1)).unwrap();
                stack.push_item(item("div", 2)).unwrap();
                stack.push_item(item("div", 3)).unwrap();
                stack.push_item(item("span", 4)).unwrap();

                let mut unmatched = Vec::default();

                stack.pop_up_to(local_name($up_to), |d| {
                    unmatched.push(d.0);
                });

                assert_eq!(unmatched, $expected_unmatched);

                assert_eq!(
                    stack
                        .items()
                        .iter()
                        .map(|i| i.local_name.clone())
                        .collect::<Vec<_>>(),
                    $expected_items
                        .iter()
                        .map(|&i| local_name(i))
                        .collect::<Vec<_>>()
                );
            }};
        }

        assert_pop_result!("span", vec![4], ["html", "body", "div", "div"]);
        assert_pop_result!("div", vec![3, 4], ["html", "body", "div"]);
        assert_pop_result!("body", vec![1, 2, 3, 4], ["html"]);
        assert_pop_result!("html", vec![0, 1, 2, 3, 4], []);

        let empty: Vec<usize> = Vec::default();

        assert_pop_result!("table", empty, ["html", "body", "div", "div", "span"]);
    }

    #[test]
    fn pop_up_to_on_empty_stack() {
        let mut stack = Stack::new(MemoryLimiter::new_shared(2048), false);
        let mut handler_called = false;

        stack.pop_up_to(local_name("div"), |_: TestElementData| {
            handler_called = true;
        });

        assert!(!handler_called);
        assert_eq!(stack.items().len(), 0);
    }
}
