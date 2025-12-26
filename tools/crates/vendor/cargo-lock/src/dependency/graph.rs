//! `petgraph` types used for modeling the `dependency::Tree`.

pub use petgraph::{EdgeDirection, graph::NodeIndex};

use crate::{Map, dependency::Dependency, package::Package};

/// Dependency graph (modeled using `petgraph`)
pub type Graph = petgraph::graph::Graph<Package, Dependency>;

/// Nodes in the dependency graph
pub type Nodes = Map<Dependency, NodeIndex>;
