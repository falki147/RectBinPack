#pragma once

#include "RectBinPack.hpp"
#include <vector>
#include <list>

namespace RBP {
	class MaxRects: public Algorithm {
	public:
		enum {
			RectBestShortSideFit,
			RectBestLongSideFit,
			RectBestAreaFit,
			RectBottomLeftRule,
			RectContactPointRule
		};

		struct Configuration {
			unsigned int m_rectHeuristic;
			unsigned int m_width;
			unsigned int m_height;
			int m_minBins;
			int m_maxBins;
			bool m_canFlip;
		};

		MaxRects(const Configuration& config);
		~MaxRects();
		
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
			std::vector<RectData*>* m_usedRects;
		};

		typedef std::vector<Bin>::iterator BinIt;
		typedef std::list<RectData*>::iterator RectDataIt;
		typedef std::list<Rect>::iterator RectIt;

		static inline unsigned long long combine(unsigned int x, unsigned int y) {
			return (long long) x << 32 | y;
		}

		template<class T, class Pred> static inline void removePairwise(std::list<T>& list, Pred pred) {
			for (auto rect = list.begin(); rect != list.end();) {
				auto next = std::next(rect);

				for (auto other = next; other != list.end();) {
					if (pred(*rect, *other)) {
						list.erase(rect);
						break;
					}

					if (pred(*other, *rect)) {
						if (other == next)
							next = other = list.erase(other);
						else
							other = list.erase(other);
					}
					else
						++other;
				}

				rect = next;
			}
		}

		bool findBest(BinIt& outBin, RectIt& outFreeRect, RectDataIt& outRect, bool& flip);
		unsigned long long getScore(const Rect& dest, unsigned int w, unsigned int h) const;
		unsigned long long getScoreContactPoint(const std::vector<RectData*>& usedRects, unsigned int x, unsigned int y, unsigned int w, unsigned int h) const;
		
		std::list<RectData*> m_rectangles;
		std::vector<Bin> m_bins;

		Configuration m_config;
	};
}
