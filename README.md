# Polonius Instruction Optimization

This repo is for in-development work on instruction optimization for [Polonius](https://github.com/rail5/polonius)

When satisfactory, this will be merged into the main Polonius repo.

## Overview

The goal is to optimize instructions given to Polonius by applying the theorems outlined in [this page](https://github.com/rail5/polonius/wiki/Instruction-Optimization)

### Optimization Levels

The current idea is to add a `-O` flag to polonius that will allow the user to specify the optimization level.

Each optimization level will also apply all of the optimizations of the levels below it.

The levels are as follows:

- `-O0`: No optimization. This may be worth it for very small files, but I doubt it.
- `-O1`: Apply theorems 0 and 1 (combination of `INSERT` instructions, and of `REMOVE` instructions)
- `-O2`: Apply theorems 3 and 4 (elimination of redundant `INSERT`/`REMOVE` pairs)
- `-O3`: Apply theorems 2, 5 and 6 (combination of `REPLACE` instructions, elimination of redundant `REPLACE` instructions)

`-O3` may not be implemented after all. I find it very hard to believe that the time it would take to reduce the number of `REPLACE` instructions could possibly be any faster than the time it would take to simply run them as given. Our primary concern is reducing the number of times that Polonius needs to traverse a file -- and when it comes to the kinds of files that Polonius is designed to work with (that is, extremely large files), the difference between the time it takes to execute a single `REPLACE` and the time it takes to traverse the file is something like the difference between the diameter of a grain of sand and the diameter of the solar system. Practically speaking, `REPLACE` instructions can be said to execute in zero time.

### Temporary Test Suite

This repository also contains a test suite written in [Bash++](https://bpp.sh) which will be used to verify that we don't screw anything up while implementing the optimizations. It will be expanded as we go along.


### Milestones

 - **2025-05-28**: -O1 optimizations fully implemented
   - Theorems 0 and 1 have been fully implemented.

 - **2025-05-29**: The original sample from the Instruction Optimization wiki page has been successfully optimized to the expected result.
   - The sequence `REMOVE 0 2`, `INSERT 0 abc` now automatically optimizes to `REPLACE 0 abc`
   - Theorems 0, 1, and 3 have been fully implemented.

 - **2025-06-29**: -O2 optimizations fully implemented
   - Theorems 0, 1, 3, and 4 have been fully implemented.
