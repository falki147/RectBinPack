#include "Guillotine.hpp"

#include <algorithm>
#include <stdexcept>

namespace RBP {
	Guillotine::Guillotine(const Configuration& config): m_config(config) { }

	void Guillotine::add(RectData* data, unsigned int w, unsigned int h) {
		if (!fits(w, h, m_config.m_width, m_config.m_height, m_config.m_canFlip))
			throw std::runtime_error("Rectangle doesn't fit");

		*data = { 0, 0, w, h, false, 0 };

		if (data->m_w != 0 && data->m_h != 0)
			m_rectangles.push_back(data);
	}

	void Guillotine::clear() {
		m_rectangles.clear();
		m_bins.clear();
	}

	bool Guillotine::findBest(BinIt& outBin, RectIt& outFreeRect, RectDataIt& outRect, bool& flip) {
		auto bestScore = std::numeric_limits<unsigned int>::max();

		for (auto binIt = m_bins.begin(); binIt != m_bins.end(); ++binIt) {
			auto& bin = *binIt;

			if (bin.m_full)
				continue;

			for (auto freeRect = bin.m_freeRects.begin(); freeRect != bin.m_freeRects.end(); ++freeRect) {
				for (auto rectIt = m_rectangles.begin(); rectIt != m_rectangles.end(); ++rectIt) {
					auto& rect = *rectIt;

					// Check if rect fits perfectly in freeRect
					if (rect->m_w == freeRect->m_w && rect->m_h == freeRect->m_h) {
						outBin      = binIt;
						outFreeRect = freeRect;
						outRect     = rectIt;
						flip        = false;

						return true;
					}

					if (m_config.m_canFlip && rect->m_h == freeRect->m_w && rect->m_w == freeRect->m_h) {
						outBin      = binIt;
						outFreeRect = freeRect;
						outRect     = rectIt;
						flip        = true;

						return true;
					}

					// Otherwise calculate score
					if (rect->m_w <= freeRect->m_w && rect->m_h <= freeRect->m_h) {
						auto score = getScore(*freeRect, rect->m_w, rect->m_h);

						if (score < bestScore) {
							outBin      = binIt;
							outFreeRect = freeRect;
							outRect     = rectIt;
							flip        = false;

							bestScore = score;
						}
					}

					if (m_config.m_canFlip && rect->m_h <= freeRect->m_w && rect->m_w <= freeRect->m_h) {
						auto score = getScore(*freeRect, rect->m_h, rect->m_w);

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

	unsigned int Guillotine::getNumBins() {
		return m_bins.size();
	}

	unsigned int Guillotine::getScore(const Rect& dest, unsigned int w, unsigned int h) const {
		switch (m_config.m_rectHeuristic) {
		case RectBestAreaFit:
			return dest.m_w * dest.m_h - w * h;
		case RectBestShortSideFit:
			return std::min(dest.m_w - w, dest.m_h - h);
		case RectBestLongSideFit:
			return std::max(dest.m_w - w, dest.m_h - h);
		case RectWorstAreaFit:
			return std::numeric_limits<unsigned int>::max() - (dest.m_w * dest.m_h - w * h);
		case RectWorstShortSideFit:
			return std::numeric_limits<unsigned int>::max() - std::min(dest.m_w - w, dest.m_h - h);
		case RectWorstLongSideFit:
			return std::numeric_limits<unsigned int>::max() - std::max(dest.m_w - w, dest.m_h - h);
		default:
			throw std::runtime_error("Invalid Rect Heuristic");
		}
	}

	bool Guillotine::merge(std::list<Rect>& list, const Rect& rect) {
		for (auto& other : list) {
			if (other.m_x == rect.m_x && other.m_w == rect.m_w) {
				if (other.m_y + other.m_h == rect.m_y) {
					other.m_h += rect.m_h;
					return true;
				}
				else if (rect.m_y + rect.m_h == other.m_y) {
					other.m_y -= rect.m_h;
					other.m_h += rect.m_h;
					return true;
				}
			}
			else if (other.m_y == rect.m_y && other.m_h == rect.m_h) {
				if (other.m_x + other.m_w == rect.m_x) {
					other.m_w += rect.m_w;
					return true;
				}
				else if (rect.m_x + rect.m_w == other.m_x) {
					other.m_x -= rect.m_x;
					other.m_w += rect.m_x;
					return true;
				}
			}
		}

		return false;
	}

	bool Guillotine::pack() {
		m_bins.clear();

		// Allocate bins (must be atleast 1)
		auto numBins = std::max(1, m_config.m_minBins);

		for (int i = 0; i < numBins; ++i)
			m_bins.push_back({ false , std::list<Rect>(1, { 0, 0, m_config.m_width, m_config.m_height }) });

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

				for (auto& bin : m_bins)
					bin.m_full = true;

				m_bins.push_back({ false , std::list<Rect>(1, { 0, 0, m_config.m_width, m_config.m_height }) });
				continue;
			}

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

					if (m_config.m_merge && merge(bin->m_freeRects, *freeRect))
						bin->m_freeRects.erase(freeRect);
				}
				else if (height == freeRect->m_h) {
					freeRect->m_x += width;
					freeRect->m_w -= width;

					if (m_config.m_merge && merge(bin->m_freeRects, *freeRect))
						bin->m_freeRects.erase(freeRect);
				}
				else {
					Rect bottom, right;

					split(*freeRect, width, height, bottom, right);

					if (m_config.m_merge && merge(bin->m_freeRects, bottom)) {
						if (m_config.m_merge && merge(bin->m_freeRects, right))
							bin->m_freeRects.erase(freeRect);
						else
							*freeRect = right;
					}
					else {
						*freeRect = bottom;

						if (!m_config.m_merge || !merge(bin->m_freeRects, right))
							bin->m_freeRects.push_back(right);
					}
				}
			}
			else
				bin->m_freeRects.erase(freeRect);

			// Remove rect
			m_rectangles.erase(rect);
		}

		return true;
	}

	void Guillotine::split(const Rect& freeRect, unsigned int width, unsigned int height, Rect& outBottom, Rect& outRight) const {
		const auto wdiff = freeRect.m_w - width;
		const auto hdiff = freeRect.m_h - height;

		auto splitHor = false;

		switch (m_config.m_splitHeuristic) {
		case SplitShorterLeftoverAxis:
			splitHor = wdiff <= hdiff;
			break;
		case SplitLongerLeftoverAxis:
			splitHor = wdiff > hdiff;
			break;
		case SplitMinimizeArea:
			splitHor = width * hdiff > wdiff * height;
			break;
		case SplitMaximizeArea:
			splitHor = width * hdiff <= wdiff * height;
			break;
		case SplitShorterAxis:
			splitHor = freeRect.m_w <= freeRect.m_h;
			break;
		case SplitLongerAxis:
			splitHor = freeRect.m_w > freeRect.m_h;
			break;
		default:
			throw std::runtime_error("Invalid Split Heuristic");
		}

		outBottom = { freeRect.m_x, freeRect.m_y + height, splitHor ? freeRect.m_w : width, freeRect.m_h - height };
		outRight  = { freeRect.m_x + width, freeRect.m_y, freeRect.m_w - width, splitHor ? height : freeRect.m_h };
	}
}
