# Tree-Bitmap: Fast lookup table for IPv4/IPv6 prefixes

> Forked from https://github.com/hroi/treebitmap

This crate provides a datastructure for fast IP address lookups.
It aims at fast lookup times, and a reasonable memory footprint.

The internal datastructure is based on the Tree-bitmap algorithm described by
W. Eatherton, Z. Dittia, G. Varghes.

## Documentation

Rustdoc: https://docs.rs/ip_network_table-deps-treebitmap/

## Illustration
An example illustration of a trie representing a routing table containing
```0.0.0.0/0``` (foo), ```10.0.0.0/8``` (bar), ```172.16.0.0/12``` (baz) and
```192.168.0.0/16``` (quux).

![rfc1918 trie illustration](https://hroi.github.io/rfc1918.svg)

## Internal trie datastructure basics
```Node ``` encodes result and child node pointers in a bitmap.

A trie node can encode up to 31 results when acting as an "end node", or 15
results and 16 children/subtrees when acting as a normal/internal node.

Each bit in the bitmap indicates a bit matching pattern:

| bit   | 0 |  1 |  2 |  3  |   4 |   5 |   6 |    7 |
|-------|---|----|----|-----|-----|-----|-----|------|
| match | * | 0* | 1* | 00* | 01* | 10* | 11* | 000* |

| bit   |    8 |    9 |   10 |   11 |   12 |   13 |   14 |          15 |
|-------|------|------|------|------|------|------|------|-------------|
| match | 001* | 010* | 011* | 100* | 101* | 110* | 111* | endnode-bit |

The last bit here does not indicate a pattern. It instead indicates if this
node is an "end node". End nodes carry double the amount of results but can't
encode any child pointers.

| bit   |    16 |    17 |    18 |    19 |    20 |    21 |    22 |    23 |
|-------|-------|-------|-------|-------|-------|-------|-------|-------|
| match | 0000* | 0001* | 0010* | 0011* | 0100* | 0101* | 0110* | 0111* |

| bit   |    24 |    25 |    26 |    27 |    28 |    29 |    30 |    31 |
|-------|-------|-------|-------|-------|-------|-------|-------|-------|
| match | 1000* | 1001* | 1010* | 1011* | 1100* | 1101* | 1110* | 1111* |

The location of the result value is computed by adding the ```result_ptr``` base
pointer and its position among set bits.

If the endnode bit is not set, the last 16 bits encodes pointers to child
nodes.
If bit N is set it means that a child node with segment value N is present.
The pointer to the child node is then computed by adding the ```child_ptr``` base
pointer and its position among set bits.

