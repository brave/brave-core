//! `petgraph` types used for modeling the `dependency::Tree`.

pub use petgraph::{graph::NodeIndex, EdgeDirection};

use crate::{dependency::Dependency, package::Package, Map};

/// Dependency graph (modeled using `petgraph`)
pub type Graph = petgraph::graph::Graph<Package, Dependency>;

/// Nodes in the dependency graph
pub type Nodes = Map<Dependency, NodeIndex>;
