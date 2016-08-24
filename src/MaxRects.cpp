#include "MaxRects.hpp"

#include <algorithm>
#include <stdexcept>

namespace RBP {
	MaxRects::MaxRects(const Configuration& config): m_config(config) { }

	MaxRects::~MaxRects() {
		clear();
	}

	void MaxRects::add(RectData* data, unsigned int w, unsigned int h) {
		if (!fits(w, h, m_config.m_width, m_config.m_height, m_config.m_canFlip))
			throw std::runtime_error("Rectangle doesn't fit");

		*data = { 0, 0, w, h, false, 0 };

		if (data->m_w != 0 && data->m_h != 0)
			m_rectangles.push_back(data);
	}

	void MaxRects::clear() {
		for (auto& bin : m_bins)
			delete bin.m_usedRects;

		m_rectangles.clear();
		m_bins.clear();
	}

	bool MaxRects::findBest(BinIt& outBin, RectIt& outFreeRect, RectDataIt& outRect, bool& flip) {
		auto bestScore = std::numeric_limits<unsigned long long>::max();

		for (auto binIt = m_bins.begin(); binIt != m_bins.end(); ++binIt) {
			auto& bin = *binIt;

			if (bin.m_full)
				continue;

			for (auto freeRect = bin.m_freeRects.begin(); freeRect != bin.m_freeRects.end(); ++freeRect) {
				for (auto rectIt = m_rectangles.begin(); rectIt != m_rectangles.end(); ++rectIt) {
					auto& rect = *rectIt;

					if (rect->m_w <= freeRect->m_w && rect->m_h <= freeRect->m_h) {
						auto score = m_config.m_rectHeuristic == RectContactPointRule ?
							getScoreContactPoint(*bin.m_usedRects, freeRect->m_x, freeRect->m_y, rect->m_w, rect->m_h) :
							getScore(*freeRect, rect->m_w, rect->m_h);

						if (score < bestScore) {
							outBin      = binIt;
							outFreeRect = freeRect;
							outRect     = rectIt;
							flip        = false;

							bestScore = score;
						}
					}
				
					if (m_config.m_canFlip && rect->m_h <= freeRect->m_w && rect->m_w <= freeRect->m_h) {
						auto score = m_config.m_rectHeuristic == RectContactPointRule ?
							getScoreContactPoint(*bin.m_usedRects, freeRect->m_x, freeRect->m_y, rect->m_h, rect->m_w) :
							getScore(*freeRect, rect->m_h, rect->m_w);

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

		return bestScore != std::numeric_limits<unsigned long long>::max();
	}

	unsigned int MaxRects::getNumBins() {
		return m_bins.size();
	}

	unsigned long long MaxRects::getScore(const Rect& dest, unsigned int w, unsigned int h) const {
		switch (m_config.m_rectHeuristic) {
		case RectBestShortSideFit:
			return combine(std::min(dest.m_w - w, dest.m_h - h), std::max(dest.m_w - w, dest.m_h - h));
		case RectBestLongSideFit:
			return combine(std::max(dest.m_w - w, dest.m_h - h), std::min(dest.m_w - w, dest.m_h - h));
		case RectBestAreaFit:
			return combine(dest.m_w * dest.m_h - w * h, std::min(dest.m_w - w, dest.m_h - h));
		case RectBottomLeftRule:
			return combine(dest.m_y + h, dest.m_x);
		default:
			throw std::runtime_error("Invalid Rect Heuristic");
		}
	}

	unsigned long long MaxRects::getScoreContactPoint(const std::vector<RectData*>& usedRects, unsigned int x, unsigned int y, unsigned int w, unsigned int h) const {
		unsigned int score = 0;

		if (x == 0 || x + w == m_config.m_width)
			score += w;

		if (y == 0 || y + h == m_config.m_height)
			score += h;

		for (auto& rect : usedRects) {
			if (rect->m_x == x + w || rect->m_x + rect->m_w == x)
				score += std::min(rect->m_y + rect->m_h, y + h) - std::max(rect->m_y, y);

			if (rect->m_y == y + h || rect->m_y + rect->m_h == y)
				score += std::min(rect->m_x + rect->m_w, x + w) - std::max(rect->m_x, x);
		}

		return score;
	}

	bool MaxRects::pack() {
		for (auto& bin : m_bins)
			delete bin.m_usedRects;

		m_bins.clear();

		// Allocate bins (must be atleast 1)
		auto numBins = std::max(1, m_config.m_minBins);

		for (int i = 0; i < numBins; ++i)
			m_bins.push_back({
				false, std::list<Rect>(1, { 0, 0, m_config.m_width, m_config.m_height }),
				m_config.m_rectHeuristic == RectContactPointRule ? new std::vector<RectData*> : nullptr
			});

		while (!m_rectangles.empty()) {
			BinIt bin;
			RectIt freeRect;
			RectDataIt rect;
			bool flip;

			// If it couldn't find a free spot, add bin
			if (!findBest(bin, freeRect, rect, flip)) {
				// Can't add a new bin
				if (m_config.m_maxBins > 0 && m_bins.size() >= (unsigned int) m_config.m_maxBins) {
					for (auto& rect : m_rectangles)
						rect->m_bin = std::numeric_limits<unsigned int>::max();

					return false;
				}

				for (auto& bin: m_bins)
					bin.m_full = true;

				m_bins.push_back({
					false, std::list<Rect>(1, { 0, 0, m_config.m_width, m_config.m_height }), 
					m_config.m_rectHeuristic == RectContactPointRule ? new std::vector<RectData*> : nullptr
				});
				continue;
			}

			// Update rect data
			auto& rectRef = **rect;

			rectRef.m_x = freeRect->m_x;
			rectRef.m_y = freeRect->m_y;
			rectRef.m_flipped = flip;
			rectRef.m_bin = std::distance(m_bins.begin(), bin);

			// Split intersecting rectangles
			std::list<Rect> newRects;

			const auto width  = flip ? rectRef.m_h : rectRef.m_w;
			const auto height = flip ? rectRef.m_w : rectRef.m_h;

			bin->m_freeRects.remove_if([&newRects, &rectRef, &width, &height](Rect& freeRect) {
				if (rectRef.m_x >= freeRect.m_x + freeRect.m_w || rectRef.m_x + width <= freeRect.m_x ||
					rectRef.m_y >= freeRect.m_y + freeRect.m_h || rectRef.m_y + height <= freeRect.m_y)
					return false;

				if (rectRef.m_x < freeRect.m_x + freeRect.m_w && rectRef.m_x + width > freeRect.m_x) {
					if (rectRef.m_y > freeRect.m_y && rectRef.m_y < freeRect.m_y + freeRect.m_h)
						newRects.push_back({ freeRect.m_x, freeRect.m_y, freeRect.m_w, rectRef.m_y - freeRect.m_y });

					if (rectRef.m_y + height < freeRect.m_y + freeRect.m_h)
						newRects.push_back({ freeRect.m_x, rectRef.m_y + height, freeRect.m_w, freeRect.m_y + freeRect.m_h - (rectRef.m_y + height) });
				}

				if (rectRef.m_y < freeRect.m_y + freeRect.m_h && rectRef.m_y + height > freeRect.m_y) {
					if (rectRef.m_x > freeRect.m_x && rectRef.m_x < freeRect.m_x + freeRect.m_w)
						newRects.push_back({ freeRect.m_x, freeRect.m_y, rectRef.m_x - freeRect.m_x, freeRect.m_h });

					if (rectRef.m_x + width < freeRect.m_x + freeRect.m_w)
						newRects.push_back({ rectRef.m_x + width, freeRect.m_y, freeRect.m_x + freeRect.m_w - (rectRef.m_x + width), freeRect.m_h });
				}

				return true;
			});

			// Remove rectangle if inside another rectangle

			removePairwise(newRects, [](Rect& rect, Rect& other) {
				if (rect.m_x != other.m_x || rect.m_y != other.m_y || rect.m_w != other.m_w || rect.m_h != other.m_h)
					if (rect.m_x >= other.m_x && rect.m_y >= other.m_y &&
						rect.m_x + rect.m_w <= other.m_x + other.m_w &&
						rect.m_y + rect.m_h <= other.m_y + other.m_h)
						return true;

				return false;
			});

			newRects.remove_if([&bin](Rect& rect) {
				for (auto& other : bin->m_freeRects)
					if (rect.m_x >= other.m_x && rect.m_y >= other.m_y &&
						rect.m_x + rect.m_w <= other.m_x + other.m_w &&
						rect.m_y + rect.m_h <= other.m_y + other.m_h)
						return true;

				return false;
			});

			bin->m_freeRects.remove_if([&newRects](Rect& rect) {
				for (auto& other : newRects)
					if (rect.m_x >= other.m_x && rect.m_y >= other.m_y &&
						rect.m_x + rect.m_w <= other.m_x + other.m_w &&
						rect.m_y + rect.m_h <= other.m_y + other.m_h)
						return true;

				return false;
			});

			bin->m_freeRects.splice(bin->m_freeRects.end(), newRects);
		
			// Add rect to used vector
			if (m_config.m_rectHeuristic == RectContactPointRule)
				bin->m_usedRects->push_back(*rect);

			// Remove rect
			m_rectangles.erase(rect);
		}

		return true;
	}
}
