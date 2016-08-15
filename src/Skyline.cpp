#include "Skyline.hpp"

#include <algorithm>
#include <stdexcept>

namespace RBP {
	Skyline::Skyline(const Configuration& config): m_config(config) { }

	Skyline::~Skyline() {
		clear();
	}

	void Skyline::add(RectData* data, unsigned int w, unsigned int h) {
		if (!fits(w, h, m_config.m_width, m_config.m_height, m_config.m_canFlip))
			throw std::runtime_error("Rectangle doesn't fit");

		*data = { 0, 0, w, h, false, 0 };

		if (data->m_w != 0 && data->m_h != 0)
			m_rectangles.push_back(data);
	}

	void Skyline::clear() {
		for (auto& bin : m_bins)
			delete bin.m_wasteMap;

		m_rectangles.clear();
		m_bins.clear();
	}

	unsigned int Skyline::computeWastedArea(LineIt line, unsigned int w, unsigned int h, unsigned int y) {
		unsigned int area = 0;

		const auto right = line->m_x + w;

		for (; line->m_x + line->m_w < right; ++line)
			area += line->m_w * (y - line->m_y);

		area += (right - line->m_x) * (y - line->m_y);

		return area;
	}

	bool Skyline::findBest(BinIt& outBin, LineIt& outLine, RectDataIt& outRect, unsigned int& outY, bool& flip) {
		auto bestScore = std::numeric_limits<unsigned long long>::max();

		for (auto binIt = m_bins.begin(); binIt != m_bins.end(); ++binIt) {
			auto& bin = *binIt;

			if (bin.m_full)
				continue;

			for (auto line = bin.m_skyline.begin(); line != bin.m_skyline.end(); ++line) {
				for (auto rectIt = m_rectangles.begin(); rectIt != m_rectangles.end(); ++rectIt) {
					auto& rect = *rectIt;

					unsigned int y;

					if (rectFits(line, rect->m_w, rect->m_h, y)) {
						auto score = m_config.m_levelHeuristic == LevelMinWasteFit ?
							getScoreMinWaste(line, rect->m_w, rect->m_h, y) :
							getScore(*line, rect->m_w, rect->m_h);

						if (score < bestScore) {
							outBin  = binIt;
							outLine = line;
							outRect = rectIt;
							outY    = y;
							flip    = false;

							bestScore = score;
						}
					}

					if (rectFits(line, rect->m_h, rect->m_w, y)) {
						auto score = m_config.m_levelHeuristic == LevelMinWasteFit ?
							getScoreMinWaste(line, rect->m_h, rect->m_w, y) :
							getScore(*line, rect->m_h, rect->m_w);

						if (score < bestScore) {
							outBin  = binIt;
							outLine = line;
							outRect = rectIt;
							outY    = y;
							flip    = true;

							bestScore = score;
						}
					}
				}
			}
		}

		return bestScore != std::numeric_limits<unsigned long long>::max();
	}

	bool Skyline::findBestWastemap(BinIt& outBin, RectIt& outFreeRect, RectDataIt& outRect, bool& flip) {
		auto bestScore = std::numeric_limits<unsigned int>::max();

		for (auto binIt = m_bins.begin(); binIt != m_bins.end(); ++binIt) {
			auto& bin = *binIt;

			if (bin.m_full)
				continue;

			for (auto freeRect = bin.m_wasteMap->begin(); freeRect != bin.m_wasteMap->end(); ++freeRect) {
				for (auto rectIt = m_rectangles.begin(); rectIt != m_rectangles.end(); ++rectIt) {
					auto& rect = *rectIt;

					if (rect->m_w <= freeRect->m_w && rect->m_h <= freeRect->m_h) {
						auto score = std::min(freeRect->m_w - rect->m_w, freeRect->m_h - rect->m_h);

						if (score < bestScore) {
							outBin      = binIt;
							outFreeRect = freeRect;
							outRect     = rectIt;
							flip        = false;

							bestScore = score;
						}
					}

					if (m_config.m_canFlip && rect->m_h <= freeRect->m_w && rect->m_w <= freeRect->m_h) {
						auto score = std::min(freeRect->m_w - rect->m_h, freeRect->m_h - rect->m_w);

						if (score < bestScore) {
							outBin      = binIt;
							outFreeRect = freeRect;
							outRect     = rectIt;
							flip        = true;

							bestScore = score;
						}
					}
				}
			}
		}

		return bestScore != std::numeric_limits<unsigned int>::max();
	}

	unsigned int Skyline::getNumBins() {
		return m_bins.size();
	}

	unsigned long long Skyline::getScore(const Line& dest, unsigned int w, unsigned int h) const {
		switch (m_config.m_levelHeuristic) {
		case LevelBottomLeft:
			return combine(dest.m_y + h, dest.m_w);
		default:
			throw std::runtime_error("Invalid Level Heuristic");
		}
	}

	unsigned long long Skyline::getScoreMinWaste(LineIt dest, unsigned int w, unsigned int h, unsigned int y) {
		return combine(computeWastedArea(dest, w, h, y), dest->m_y + h);
	}

	bool Skyline::pack() {
		for (auto& bin : m_bins)
			delete bin.m_wasteMap;

		m_bins.clear();

		// Allocate bins (must be atleast 1)
		auto numBins = std::max(1, m_config.m_minBins);

		for (int i = 0; i < numBins; ++i)
			m_bins.push_back({ false, std::list<Line>(1, { 0, 0, m_config.m_width }),  m_config.m_wasteMap ? new std::list<Rect> : nullptr });

		while (!m_rectangles.empty()) {
			// Check wastemap
			if (m_config.m_wasteMap) {
				BinIt bin;
				RectIt freeRect;
				RectDataIt rect;
				bool flip;

				if (findBestWastemap(bin, freeRect, rect, flip)) {
					// Update rect data
					auto& rectRef = **rect;

					rectRef.m_x = freeRect->m_x;
					rectRef.m_y = freeRect->m_y;
					rectRef.m_flipped = flip;
					rectRef.m_bin = std::distance(m_bins.begin(), bin);

					// Split free rectangle
					const auto width  = flip ? rectRef.m_h : rectRef.m_w;
					const auto height = flip ? rectRef.m_w : rectRef.m_h;

					if (width != freeRect->m_w || height != freeRect->m_h) {
						if (width == freeRect->m_w) {
							freeRect->m_y += height;
							freeRect->m_h -= height;
						}
						else if (height == freeRect->m_h) {
							freeRect->m_x += width;
							freeRect->m_w -= width;
						}
						else {
							const unsigned int wdif = freeRect->m_w - width;
							const unsigned int hdif = freeRect->m_h - height;

							auto splitHor = width * hdif > wdif * height;

							bin->m_wasteMap->push_back({ freeRect->m_x, freeRect->m_y + height, splitHor ? freeRect->m_w : width, freeRect->m_h - height });
							bin->m_wasteMap->push_back({ freeRect->m_x + width, freeRect->m_y, freeRect->m_w - width, splitHor ? height : freeRect->m_h });
						}
					}
					else
						bin->m_wasteMap->erase(freeRect);

					// Remove rect
					m_rectangles.erase(rect);
					continue;
				}
			}

			BinIt bin;
			LineIt line;
			RectDataIt rect;
			unsigned int y;
			bool flip;

			// If it couldn't find a free spot, add bin
			if (!findBest(bin, line, rect, y, flip)) {
				// Can't add a new bin
				if (m_config.m_maxBins > 0 && m_bins.size() >= (unsigned int) m_config.m_maxBins) {
					for (auto& rect : m_rectangles)
						rect->m_bin = std::numeric_limits<unsigned int>::max();

					return false;
				}

				for (auto& bin : m_bins)
					bin.m_full = true;

				m_bins.push_back({ false, std::list<Line>(1, { 0, 0, m_config.m_width }),  m_config.m_wasteMap ? new std::list<Rect> : nullptr });
				continue;
			}

			// Update rect data
			auto& rectRef = **rect;

			rectRef.m_x = line->m_x;
			rectRef.m_y = y;
			rectRef.m_flipped = flip;
			rectRef.m_bin = std::distance(m_bins.begin(), bin);

			const auto width  = flip ? rectRef.m_h : rectRef.m_w;
			const auto height = flip ? rectRef.m_w : rectRef.m_h;

			// Add wasted areas to wastemap
			if (m_config.m_wasteMap) {
				auto curLine = line;

				const auto right = curLine->m_x + width;

				for (; curLine->m_x + curLine->m_w < right; ++curLine)
					bin->m_wasteMap->push_back({ curLine->m_x, curLine->m_y, curLine->m_w, y - curLine->m_y });

				bin->m_wasteMap->push_back({ curLine->m_x, curLine->m_y, right - curLine->m_x, y - curLine->m_y });
			}

			// Update skyline
			LineIt newLine, prev;

			if (line != bin->m_skyline.begin() && (prev = std::prev(line))->m_y == y + height) {
				prev->m_w += width;
				newLine = prev;
			}
			else
				newLine = bin->m_skyline.insert(line, { line->m_x, y + height, width });

			// Erase each line which is below the new one
			while (line->m_x + line->m_w < newLine->m_x + newLine->m_w)
				bin->m_skyline.erase(line++);

			if (line->m_y == newLine->m_y) {
				newLine->m_w = line->m_x + line->m_w - newLine->m_x;
				bin->m_skyline.erase(line);
			}
			else {
				auto diff = newLine->m_x + newLine->m_w - line->m_x;

				line->m_x += diff;
				line->m_w -= diff;
			}

			// Remove rect
			m_rectangles.erase(rect);
		}

		return true;
	}

	bool Skyline::rectFits(LineIt line, unsigned int w, unsigned int h, unsigned int& y) const {
		auto x = line->m_x;

		if (x + w > m_config.m_width)
			return false;

		y = line->m_y;

		unsigned int width = 0;

		do {
			y = std::max(y, line->m_y);

			if (y + h > m_config.m_height)
				return false;

			width += line++->m_w;
		}
		while (width < w);

		return true;
	}
}
