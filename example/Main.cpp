#include <array>
#include <ctime>
#include <iostream>
#include <MaxRects.hpp>

const unsigned int numRects = 20;
const unsigned int size = 25;

inline unsigned int getSize() {
	return rand() % 3 + 4;
}

int main(int argc, char** argv) {
	srand((unsigned int) time(nullptr));
	
	// Prepare the rectangle bin packer
	RBP::MaxRects g({ RBP::MaxRects::RectBestAreaFit, size, size, 0, 1, true });
	
	RBP::RectData data[numRects];

	// Add rectangles and pack them into one bin
	for (unsigned int i = 0; i < numRects; ++i)
		g.add(&data[i], getSize(), getSize());
	
	if (!g.pack()) {
		std::cout << "Failed to pack" << std::endl;
		return 0;
	}

	// Output rectangles to console
	char map[size][size];

	for (unsigned int i = 0; i < size; ++i)
		for (unsigned int j = 0; j < size; ++j)
			map[j][i] = ' ';

	for (auto& rect : data) {
		const auto width  = rect.m_flipped ? rect.m_h : rect.m_w;
		const auto height = rect.m_flipped ? rect.m_w : rect.m_h;

		for (unsigned int i = 1; i < width - 1; ++i) map[rect.m_y][rect.m_x + i] = '-';
		for (unsigned int i = 1; i < width - 1; ++i) map[rect.m_y + height - 1][rect.m_x + i] = '-';

		for (unsigned int i = 1; i < height - 1; ++i) map[rect.m_y + i][rect.m_x] = '|';
		for (unsigned int i = 1; i < height - 1; ++i) map[rect.m_y + i][rect.m_x + width - 1] = '|';

		map[rect.m_y][rect.m_x] = '+';
		map[rect.m_y][rect.m_x + width - 1] = '+';
		map[rect.m_y + height - 1][rect.m_x] = '+';
		map[rect.m_y + height - 1][rect.m_x + width - 1] = '+';
	}

	for (unsigned int j = 0; j < size; ++j) {
		for (unsigned int i = 0; i < size; ++i)
			std::cout << map[j][i];

		std::cout << std::endl;
	}

	return 0;
}
