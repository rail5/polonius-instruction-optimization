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
			// Level 2 optimizations:
			// Apply theorem #3 (eliminating redundant insert/remove pairs)
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
	std::deque<Block> inserts;
	std::deque<Block> removes;
	std::deque<Block> replaces;
	switch (optimization_level) {
		default:
		case 3:
		case 2:
			// Level 2 optimizations:
			// Apply theorem #4 (eliminating redundant insert/remove pairs)
		case 1:
			// Level 1 optimizations:
			// Apply theorem #1 (combining remove instructions)
			// Most of the leg-work is already handled by INSERTs (above)
			// Here, we only have to make sure we separate REMOVEs from REPLACEs
			while (blocks.size() > 0) {
				std::pair<uint64_t, uint64_t> overlap;
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
						break;
					case MULTIPLY:
						replaces.push_back(blocks[0]);
						blocks.pop_front();
						operators.pop_front();
						overlap = replaces.back().overlap(block);
						if (replaces.back().start() > block.end()) {
							replaces.back().shift_left(block.size());
						} else if (overlap != std::pair<uint64_t, uint64_t>(0, 0)) {
							// Remove the overlapping portion of the REPLACE instruction
							replaces.back().remove(overlap.first, overlap.second);
							if (replaces.back().empty()) {
								// If the REPLACE instruction is now empty, remove it
								replaces.pop_back();
							} else {
								// If it's not empty, shift it left to compensate for the removed portion
								// (Since this replace will be put *after* the REMOVE instruction)
								replaces.back().shift_left(overlap.second - overlap.first + 1);
							}
						}
						break;
				}
			}
			for (const auto& b : inserts) {
				blocks.push_back(b);
				operators.push_back(ADD);
			}
			for (const auto& b : removes) {
				blocks.push_back(b);
				operators.push_back(SUBTRACT);
			}
		case 0:
			blocks.push_back(block);
			operators.push_back(SUBTRACT);
			break;
	}
	// Copy 'replaces' to the main blocks vector
	for (const auto& b : replaces) {
		blocks.push_back(b);
		operators.push_back(MULTIPLY);
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
