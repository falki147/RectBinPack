#pragma once

#include "RectBinPack.hpp"
#include <vector>
#include <list>

namespace RBP {
	class Guillotine: public Algorithm {
	public:
		enum {
			RectBestAreaFit,
			RectBestShortSideFit,
			RectBestLongSideFit,
			RectWorstAreaFit,
			RectWorstShortSideFit,
			RectWorstLongSideFit
		};

		enum {
			SplitShorterLeftoverAxis,
			SplitLongerLeftoverAxis,
			SplitMinimizeArea,
			SplitMaximizeArea,
			SplitShorterAxis,
			SplitLongerAxis
		};

		struct Configuration {
			unsigned int m_rectHeuristic;
			unsigned int m_splitHeuristic;
			unsigned int m_width;
			unsigned int m_height;
			int m_minBins;
			int m_maxBins;
			bool m_canFlip;
			bool m_merge;
		};

		Guillotine(const Configuration& config);
		~Guillotine() { }

		inline void configure(const Configuration& config) {
			m_config = config;
			clear();
		}

		inline const Configuration& getConfiguration() const {
			return m_config;
		}

		void add(RectData* data, unsigned int w, unsigned int h);
		void clear();
		unsigned int getNumBins();
		bool pack();

	private:
		struct Rect {
			unsigned int m_x, m_y, m_w, m_h;
		};

		struct Bin {
			bool m_full;
			std::list<Rect> m_freeRects;
		};

		typedef std::vector<Bin>::iterator BinIt;
		typedef std::list<RectData*>::iterator RectDataIt;
		typedef std::list<Rect>::iterator RectIt;

		bool findBest(BinIt& outBin, RectIt& outFreeRect, RectDataIt& outRect, bool& flip);
		unsigned int getScore(const Rect& dest, unsigned int w, unsigned int h) const;
		static bool merge(std::list<Rect>& list, const Rect& rect);
		void split(const Rect& freeRect, unsigned int width, unsigned int height, Rect& outBottom, Rect& outRight) const;

		std::list<RectData*> m_rectangles;
		std::vector<Bin> m_bins;

		Configuration m_config;
	};
}
