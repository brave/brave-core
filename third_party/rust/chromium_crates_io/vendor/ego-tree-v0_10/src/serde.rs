//! Implement `serde::Serialize` and `serde::Deserialize` traits for Tree
//!
//! # Warning
//! Serialize and Deserialize implementations are recursive. They require an amount of stack memory
//! proportional to the depth of the tree.

use std::{fmt, marker::PhantomData};

use serde::{
    de::{self, MapAccess, Visitor},
    ser::{Serialize, SerializeStruct},
    Deserialize, Deserializer,
};

use crate::{NodeMut, NodeRef, Tree};

#[derive(Debug)]
struct SerNode<'a, T> {
    value: &'a T,
    children: Vec<SerNode<'a, T>>,
}

impl<'a, T> From<NodeRef<'a, T>> for SerNode<'a, T> {
    fn from(node: NodeRef<'a, T>) -> Self {
        let value = node.value();
        let children = node.children().map(SerNode::from).collect();
        Self { value, children }
    }
}

impl<T: Serialize> Serialize for SerNode<'_, T> {
    fn serialize<S>(&self, serializer: S) -> Result<S::Ok, S::Error>
    where
        S: serde::Serializer,
    {
        let mut state = serializer.serialize_struct("Node", 2)?;
        state.serialize_field("value", &self.value)?;
        state.serialize_field("children", &self.children)?;
        state.end()
    }
}

impl<T: Serialize> Serialize for Tree<T> {
    fn serialize<S>(&self, serializer: S) -> Result<S::Ok, S::Error>
    where
        S: serde::Serializer,
    {
        SerNode::from(self.root()).serialize(serializer)
    }
}

#[derive(Debug)]
struct DeserNode<T> {
    value: T,
    children: Vec<DeserNode<T>>,
}

impl<T> DeserNode<T> {
    fn into_tree_node(self, parent: &mut NodeMut<T>) {
        let mut node = parent.append(self.value);

        for child in self.children {
            child.into_tree_node(&mut node);
        }
    }
}

impl<T> From<DeserNode<T>> for Tree<T> {
    fn from(root: DeserNode<T>) -> Self {
        let mut tree: Tree<T> = Tree::new(root.value);
        let mut tree_root = tree.root_mut();

        for child in root.children {
            child.into_tree_node(&mut tree_root);
        }

        tree
    }
}

struct DeserNodeVisitor<T> {
    marker: PhantomData<fn() -> DeserNode<T>>,
}

impl<T> DeserNodeVisitor<T> {
    fn new() -> Self {
        DeserNodeVisitor {
            marker: PhantomData,
        }
    }
}

impl<'de, T> Visitor<'de> for DeserNodeVisitor<T>
where
    T: Deserialize<'de>,
{
    type Value = DeserNode<T>;

    fn expecting(&self, formatter: &mut fmt::Formatter) -> fmt::Result {
        formatter.write_str("struct Node")
    }

    fn visit_map<M>(self, mut map: M) -> Result<Self::Value, M::Error>
    where
        M: MapAccess<'de>,
    {
        let mut value = None;
        let mut children = None;

        while let Some(key) = map.next_key()? {
            match key {
                "value" => {
                    if value.is_some() {
                        return Err(de::Error::duplicate_field("value"));
                    }
                    value = Some(map.next_value()?);
                }
                "children" => {
                    if children.is_some() {
                        return Err(de::Error::duplicate_field("children"));
                    }
                    children = Some(map.next_value()?);
                }
                _ => {
                    return Err(de::Error::unknown_field(key, &["value", "children"]));
                }
            }
        }

        let value = value.ok_or_else(|| de::Error::missing_field("value"))?;
        let children = children.ok_or_else(|| de::Error::missing_field("children"))?;

        Ok(DeserNode { value, children })
    }
}

impl<'de, T> Deserialize<'de> for DeserNode<T>
where
    T: Deserialize<'de>,
{
    fn deserialize<D>(deserializer: D) -> Result<Self, D::Error>
    where
        D: Deserializer<'de>,
    {
        deserializer.deserialize_struct("Node", &["value", "children"], DeserNodeVisitor::new())
    }
}

impl<'de, T: Deserialize<'de>> Deserialize<'de> for Tree<T> {
    fn deserialize<D>(deserializer: D) -> Result<Self, D::Error>
    where
        D: serde::Deserializer<'de>,
    {
        let deser = DeserNode::<T>::deserialize(deserializer)?;
        Ok(deser.into())
    }
}
