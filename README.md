Rectangular Bin Packing Library
=============================
This library provides several approximations on how to pack rectangles into several bins. The project is based on the
[survey](http://clb.demon.fi/files/RectangleBinPack.pdf) of [Jukka Jylänki](https://github.com/juj) and the [code
used for it](https://github.com/juj/RectangleBinPack) Possible applications are e.g. Texture Atlas Generators.

Usage
-----
To pack rectangles you just need to add them with `add` and then call `pack`. Keep in mind that the callee is responsible
for the `RectData` pointer supplied to it. If you don't pass in a pointer it'll create a `RectData` struct on the heap.
`pack` returns false if it can't fit every rectangle and the ones left will be set to `std::numeric_limits<unsigned int>::max()`.
If a rectangle is too big or an invalid heuristic was passed through the configuration it'll throw a `std::runtime_error`.

Configuration Values
--------------------
These values are used in the `Configuration` struct of each algorithm. Some of them might not exist in every algorithm.

| Name           | Description                                             |
| -------------  | ------------------------------------------------------- |
| RectHeuristic  | Heuristic to choose position of rectangle               |
| LevelHeuristic | Heuristic to choose position of rectangle               |
| SplitHeuristic | Heuristic to determine which axis to split              |
| Width          | Width of the bin                                        |
| Height         | Height of the bin                                       |
| MinBins        | Minimal number of bins (defaults to 1 when less than 1) |
| MaxBins        | Maximal number of bins (ignored if less than 1)         |
| CanFlip        | Enables flipping                                        |
| Merge          | Merges free rectangles into one if possible             |
| WasteMap       | Enables storing and reusing "wasted" space              |

Building the Library
--------------------
The repository includes a [CMake](https://cmake.org/) file which can be used to compile it as a static library,
but since the source consists of only a few files you may also just copy them.
