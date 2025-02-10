//! Dependency trees computed from `Cargo.lock` files.
//!
//! Uses the `petgraph` crate for modeling the dependency structure.

// Includes code from `cargo-tree`, Copyright (c) 2015-2016 Steven Fackler
// Licensed under the same terms as `cargo-audit` (i.e. Apache 2.0 + MIT)

use super::{
    graph::{EdgeDirection, Graph, NodeIndex, Nodes},
    Dependency,
};
use crate::{error::Error, lockfile::Lockfile, Map};
use std::{collections::BTreeSet as Set, io};

/// Dependency tree computed from a `Cargo.lock` file
#[derive(Clone, Debug)]
pub struct Tree {
    /// Dependency graph for a particular package
    graph: Graph,

    /// Package data associated with nodes in the graph
    nodes: Nodes,
}

impl Tree {
    /// Construct a new dependency tree for the given [`Lockfile`].
    pub fn new(lockfile: &Lockfile) -> Result<Self, Error> {
        let mut graph = Graph::new();
        let mut nodes = Map::new();

        // Populate all graph nodes in the first pass
        for package in &lockfile.packages {
            let node_index = graph.add_node(package.clone());
            nodes.insert(Dependency::from(package), node_index);
        }

        // Populate all graph edges in the second pass
        for package in &lockfile.packages {
            let parent_index = nodes[&Dependency::from(package)];

            for dependency in &package.dependencies {
                if let Some(node_index) = nodes.get(dependency) {
                    graph.add_edge(parent_index, *node_index, dependency.clone());
                } else {
                    return Err(Error::Resolution(format!(
                        "failed to find dependency: {dependency}"
                    )));
                }
            }
        }

        Ok(Tree { graph, nodes })
    }

    /// Render the dependency graph for the given [`NodeIndex`] using the
    /// default set of [`Symbols`].
    pub fn render(
        &self,
        w: &mut impl io::Write,
        node_index: NodeIndex,
        direction: EdgeDirection,
        exact: bool,
    ) -> io::Result<()> {
        self.render_with_symbols(w, node_index, direction, &Symbols::default(), exact)
    }

    /// Render the dependency graph for the given [`NodeIndex`] using the
    /// provided set of [`Symbols`].
    pub fn render_with_symbols(
        &self,
        w: &mut impl io::Write,
        node_index: NodeIndex,
        direction: EdgeDirection,
        symbols: &Symbols,
        exact: bool,
    ) -> io::Result<()> {
        Presenter::new(&self.graph, symbols).print_node(w, node_index, direction, exact)
    }

    /// Get the indexes of the root packages in the workspace
    /// (i.e. toplevel packages which are not used as dependencies)
    pub fn roots(&self) -> Vec<NodeIndex> {
        self.graph.externals(EdgeDirection::Incoming).collect()
    }

    /// Get the `petgraph` dependency graph.
    pub fn graph(&self) -> &Graph {
        &self.graph
    }

    /// Get the nodes of the `petgraph` dependency graph.
    pub fn nodes(&self) -> &Nodes {
        &self.nodes
    }
}

/// Symbols to use when printing the dependency tree
pub struct Symbols {
    down: &'static str,
    tee: &'static str,
    ell: &'static str,
    right: &'static str,
}

impl Default for Symbols {
    fn default() -> Symbols {
        Self {
            down: "│",
            tee: "├",
            ell: "└",
            right: "─",
        }
    }
}

/// Dependency tree presenter
struct Presenter<'g, 's> {
    /// Dependency graph being displayed
    graph: &'g Graph,

    /// Symbols to use to display graph
    symbols: &'s Symbols,

    /// Are there continuing levels?
    levels_continue: Vec<bool>,

    /// Dependencies we've already visited
    visited: Set<NodeIndex>,
}

impl<'g, 's> Presenter<'g, 's> {
    /// Create a new dependency tree `Presenter`.
    fn new(graph: &'g Graph, symbols: &'s Symbols) -> Self {
        Self {
            graph,
            symbols,
            levels_continue: vec![],
            visited: Set::new(),
        }
    }

    /// Print a node in the dependency tree.
    fn print_node(
        &mut self,
        w: &mut impl io::Write,
        node_index: NodeIndex,
        direction: EdgeDirection,
        exact: bool,
    ) -> io::Result<()> {
        let package = &self.graph[node_index];
        let new = self.visited.insert(node_index);

        if let Some((&last_continues, rest)) = self.levels_continue.split_last() {
            for &continues in rest {
                let c = if continues { self.symbols.down } else { " " };
                write!(w, "{c}   ")?;
            }

            let c = if last_continues {
                self.symbols.tee
            } else {
                self.symbols.ell
            };

            write!(w, "{0}{1}{1} ", c, self.symbols.right)?;
        }

        if exact {
            let spec = if let Some(checksum) = &package.checksum {
                format!("checksum:{checksum}")
            } else if let Some(src) = &package.source {
                src.to_string()
            } else {
                "inexact".to_string()
            };
            writeln!(w, "{} {} {}", &package.name, &package.version, spec)?;
        } else {
            writeln!(w, "{} {}", &package.name, &package.version)?;
        }

        if !new {
            return Ok(());
        }

        use petgraph::visit::EdgeRef;
        let dependencies = self
            .graph
            .edges_directed(node_index, direction)
            .map(|edge| match direction {
                EdgeDirection::Incoming => edge.source(),
                EdgeDirection::Outgoing => edge.target(),
            })
            .collect::<Vec<_>>();

        for (i, dependency) in dependencies.iter().enumerate() {
            self.levels_continue.push(i < (dependencies.len() - 1));
            self.print_node(w, *dependency, direction, exact)?;
            self.levels_continue.pop();
        }

        Ok(())
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn compute_tree_v3() {
        // TODO(tarcieri): test dependency tree is computed correctly
        let lockfile = Lockfile::load("tests/examples/Cargo.lock.v3").unwrap();
        Tree::new(&lockfile).unwrap();
    }

    #[test]
    fn compute_roots_v3() {
        let lockfile = Lockfile::load("tests/examples/Cargo.lock.v3").unwrap();
        let tree = Tree::new(&lockfile).unwrap();
        let roots = tree.roots();
        assert_eq!(roots.len(), 1);

        let root_package = &tree.graph[roots[0]];
        assert_eq!(root_package.name.as_str(), "cargo-lock");
    }

    #[test]
    fn compute_tree_git_ref() {
        let lockfile = Lockfile::load("tests/examples/Cargo.lock.git-ref").unwrap();
        Tree::new(&lockfile).unwrap();
    }
}
