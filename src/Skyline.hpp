#pragma once

#include "RectBinPack.hpp"
#include <vector>
#include <list>

namespace RBP {
	class Skyline: public Algorithm {
	public:
		enum {
			LevelBottomLeft,
			LevelMinWasteFit
		};

		struct Configuration {
			unsigned int m_levelHeuristic;
			unsigned int m_width;
			unsigned int m_height;
			int m_minBins;
			int m_maxBins;
			bool m_canFlip;
			bool m_wasteMap;
		};

		Skyline(const Configuration& config);
		~Skyline();

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
		struct Line {
			unsigned int m_x, m_y, m_w;
		};

		struct Rect {
			unsigned int m_x, m_y, m_w, m_h;
		};

		struct Bin {
			bool m_full;
			std::list<Line> m_skyline;
			std::list<Rect>* m_wasteMap;
		};

		typedef std::vector<Bin>::iterator BinIt;
		typedef std::list<Line>::iterator LineIt;
		typedef std::list<RectData*>::iterator RectDataIt;
		typedef std::list<Rect>::iterator RectIt;

		static inline unsigned long long combine(unsigned int x, unsigned int y) {
			return (unsigned long long) x << 32 | y;
		}

		static unsigned int computeWastedArea(LineIt line, unsigned int w, unsigned int h, unsigned int y);
		bool findBest(BinIt& outBin, LineIt& outLine, RectDataIt& outRect, unsigned int& outY, bool& flip);
		bool findBestWastemap(BinIt& outBin, RectIt& outFreeRect, RectDataIt& outRect, bool& flip);
		unsigned long long getScore(const Line& dest, unsigned int w, unsigned int h) const;
		static unsigned long long getScoreMinWaste(LineIt dest, unsigned int w, unsigned int h, unsigned int y);
		bool rectFits(LineIt line, unsigned int w, unsigned int h, unsigned int& y) const;
		
		std::list<RectData*> m_rectangles;
		std::vector<Bin> m_bins;

		Configuration m_config;
	};
}
