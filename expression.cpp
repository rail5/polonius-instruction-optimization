#include "expression.h"

Expression::Expression() = default;

Expression Expression::operator+(Block block) {
	std::deque<Block> inserts;
	std::deque<Block> removes;
	std::deque<Block> replaces;
	switch (optimization_level) {
		default:
		case 3:
		case 2:
		case 1:
			// Level 1 optimizations:
			// Apply theorem #0 (combining insert instructions)
			// We don't need to actually strictly *combine* them,
			// We just need to put all the insert instructions next to each other
			// So that we can execute all of them on a single pass-through
			while (blocks.size() > 0) {
				switch (operators[0]) {
					case ADD:
						inserts.push_back(blocks[0]);
						blocks.pop_front();
						operators.pop_front();
						break;
					case SUBTRACT:
						removes.push_back(blocks[0]);
						blocks.pop_front();
						operators.pop_front();
						if (removes.back().start() <= block.start()) {
							block.shift_right(removes.back().size());
						} else if (removes.back().start() > block.start()) {
							removes.back().shift_right(block.size());
						}
						break;
					case MULTIPLY:
						replaces.push_back(blocks[0]);
						blocks.pop_front();
						operators.pop_front();
						if (replaces.back().start() >= block.start()) {
							replaces.back().shift_right(block.size());
						}
						break;
				}
			}
			for (const auto& b : inserts) {
					blocks.push_back(b);
					operators.push_back(ADD);
				}
		case 0:
			blocks.push_back(block);
			operators.push_back(ADD);
			break;
	}
	// Copy 'removes' and 'replaces' to the main blocks vector
	for (const auto& b : removes) {
		blocks.push_back(b);
		operators.push_back(SUBTRACT);
	}
	for (const auto& b : replaces) {
		blocks.push_back(b);
		operators.push_back(MULTIPLY);
	}
	return *this;
}

Expression Expression::operator-(Block block) {
	switch (optimization_level) {
		default:
		case 3:
		case 2:
		case 1:
		case 0:
			blocks.push_back(block);
			operators.push_back(SUBTRACT);
			break;
	}
	return *this;
}

Expression Expression::operator*(Block block) {
	switch (optimization_level) {
		default:
		case 3:
		case 2:
		case 1:
		case 0:
			blocks.push_back(block);
			operators.push_back(MULTIPLY);
			break;
	}
	return *this;
}
