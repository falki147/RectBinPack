#pragma once

namespace RBP {
	struct RectData {
		unsigned int m_x, m_y, m_w, m_h;
		bool m_flipped;
		unsigned int m_bin;
	};

	class Algorithm {
	public:
		virtual ~Algorithm() { }

		inline RectData* add(unsigned int w, unsigned int h) {
			auto ptr = new RectData;
			add(ptr, w, h);
			return ptr;
		}

		virtual void add(RectData* data, unsigned int w, unsigned int h) = 0;
		virtual unsigned int getNumBins() = 0;
		virtual bool pack() = 0;

	protected:
		static inline bool fits(unsigned int w, unsigned int h, unsigned int targetW, unsigned int targetH, bool canFlip) {
			return canFlip ? ((w <= targetW && h <= targetH) || (h <= targetW && w <= targetH)) : (w <= targetW && h <= targetH);
		}
	};
}
