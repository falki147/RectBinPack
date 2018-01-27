Rectangular Bin Packing Library
=============================
This library provides algorithms on how to pack rectangles into several bins. It currently contains the MaxRects and Guillotine algorithms. The project is based on the [survey](http://clb.demon.fi/files/RectangleBinPack.pdf) of [Jukka Jylänki](https://github.com/juj) and the [reference implementation](https://github.com/juj/RectangleBinPack) Possible applications are e.g. Texture Atlas Generators.

Features
--------
* C++ 11 support
* Header only
* Custom types using template specialization

Example
-------
Full example can be found in `src/Example.cpp`

```C++
struct CustomRect { ... };

// Template specialization for converting CustomRect to RectBinPack::Rect
template<>
RectBinPack::Rect RectBinPack::toRect<CustomRect>(const CustomRect& value) { ... }

// Template specialization for converting RectBinPack::BinRect to CustomRect
template<>
void RectBinPack::fromBinRect<CustomRect>(CustomRect& value, RectBinPack::BinRect rect) { ... }

std::vector<CustomRect> data { ... };

// Initialize configuration (size x size, 1 bin, no flipping, BestAreaFit)
RectBinPack::MaxRectsConfiguration config {
	size, size, 1, 1, false, RectBinPack::MaxRectsHeuristic::BestAreaFit
};

// Pack rectangles
RectBinPack::packMaxRects(config, data);
```

Installation
------------
Just copy and paste the include folder into your project (or add it to your include path)
