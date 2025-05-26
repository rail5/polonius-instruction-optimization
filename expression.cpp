#include "expression.h"
#include <iostream>

Expression::Expression() = default;

Expression::Expression(uint8_t optimization_level) {
	this->optimization_level = optimization_level;
}


void Expression::set_optimization_level(uint8_t level) {
	optimization_level = level;

	if (!blocks.empty()) {
		// If we change the optimization level, we need to re-evaluate the expression
		// If we don't do this, the expression will not be optimized correctly when we add more terms
		// (And will become, in fact, an incorrect and potentially invalid expression)
		// E.g, if we've *been* optimizing according to -O1,
		// And we now set the optimization level to -O2,
		// We need to re-evaluate the expression to apply the new optimizations from the beginning
		re_evaluate();
	}
}

void Expression::re_evaluate() {
	// Re-evaluate the entire expression

	std::deque<Block> blocks_copy = blocks;
	blocks.clear();

	for (auto& block : blocks_copy) {
		switch (block.get_operator()) {
			case INSERT:
				insert(block);
				break;
			case REMOVE:
				remove(block);
				break;
			case REPLACE:
				replace(block);
				break;
		}
	}
}

void Expression::insert(Block block) {
	block.set_operator(INSERT);
	std::deque<Block> inserts;
	std::deque<Block> removes;
	std::deque<Block> replaces;
	uint64_t original_start = block.start();
	switch (optimization_level) {
		default:
			[[fallthrough]];
		case 3:
			[[fallthrough]];
		case 2:
			// Level 2 optimizations:
			// Apply theorem #3 (eliminating redundant insert/remove pairs)
			[[fallthrough]];
		case 1:
			// Level 1 optimizations:
			// Apply theorem #0 (combining insert instructions)
			// We don't need to actually strictly *combine* them,
			// We just need to put all the insert instructions next to each other
			// So that we can execute all of them on a single pass-through
			while (blocks.size() > 0) {
				switch (blocks[0].get_operator()) {
					case INSERT:
						inserts.push_back(blocks[0]);
						blocks.pop_front();
						break;
					case REMOVE:
						removes.push_back(blocks[0]);
						blocks.pop_front();
						if (removes.back().start() <= block.start()) {
							block.shift_right(removes.back().size());
						} else if (removes.back().start() > block.start()) {
							removes.back().shift_right(block.size());
						}
						break;
					case REPLACE:
						replaces.push_back(blocks[0]);
						blocks.pop_front();
						if (replaces.back().start() >= original_start) {
							replaces.back().shift_right(block.size());
						}
						break;
				}
			}
			for (const auto& b : inserts) {
				blocks.push_back(b);
			}
			[[fallthrough]];
		case 0:
			if (!block.empty()) {
				blocks.push_back(block);
			}
			break;
	}
	// Copy 'removes' and 'replaces' to the main blocks vector
	for (const auto& b : removes) {
		blocks.push_back(b);
	}
	for (const auto& b : replaces) {
		blocks.push_back(b);
	}
}

void Expression::remove(Block block) {
	block.set_operator(REMOVE);
	std::deque<Block> inserts;
	std::deque<Block> removes;
	std::deque<Block> replaces;
	switch (optimization_level) {
		default:
			[[fallthrough]];
		case 3:
			[[fallthrough]];
		case 2:
			// Level 2 optimizations:
			// Apply theorem #4 (eliminating redundant insert/remove pairs)
			[[fallthrough]];
		case 1:
			// Level 1 optimizations:
			// Apply theorem #1 (combining remove instructions)
			// Most of the leg-work is already handled by INSERTs (above)
			// Here, we only have to make sure we separate REMOVEs from REPLACEs
			while (blocks.size() > 0) {
				std::pair<uint64_t, uint64_t> overlap;
				switch (blocks[0].get_operator()) {
					case INSERT:
						inserts.push_back(blocks[0]);
						blocks.pop_front();
						break;
					case REMOVE:
						removes.push_back(blocks[0]);
						blocks.pop_front();
						break;
					case REPLACE:
						replaces.push_back(blocks[0]);
						blocks.pop_front();
						overlap = replaces.back().overlap(block);
						if (replaces.back().start() > block.end()) {
							replaces.back().shift_left(block.size());
						} else if (overlap != std::pair<uint64_t, uint64_t>(0, 0)) {
							// Remove the overlapping portion of the REPLACE instruction
							replaces.back().remove(overlap.first, overlap.second);
							// Remove the REPLACE instruction
							Block original_replace = replaces.back();
							replaces.pop_back();
							// If it's not empty:
							// It may be the case that the REMOVE instruction cut out the middle of this REPLACE (roughly speaking)
							// Or, it may be the case that the REMOVE instruction cut out the end of this REPLACE,
							// Or finally, it may be the case that the REMOVE instruction cut out the beginning of this REPLACE
							// The important point is this:
							// If there is any more to the REPLACE instruction *to the right* of the overlap,
							// That portion needs to be left-shifted by the size of the overlap between the two instructions
							// Therefore: split the REPLACE instruction into two REPLACEs
							// One to the left of the overlap, and one to the right of the overlap
							// And shift the right-hand side leftwards

							// If the original replace is now *empty* after our previous operations,
							// Neither of the two following branches will execute anyway,
							// And the REPLACE instruction will simply be removed from the expression
							// (As is proper)
							if (overlap.first > original_replace.start()) {
								Block left_replace = original_replace;
								left_replace.remove(overlap.first, original_replace.end());
								if (!left_replace.empty()) {
									replaces.push_back(left_replace);
								}
							}
							if (overlap.second < original_replace.end()) {
								Block right_replace = original_replace;
								right_replace.remove(original_replace.start(), overlap.second);
								right_replace.shift_left(overlap.second - overlap.first + 1);
								if (!right_replace.empty()) {
									replaces.push_back(right_replace);
								}
							}
						}
						break;
				}
			}
			for (const auto& b : inserts) {
				blocks.push_back(b);
			}
			for (const auto& b : removes) {
				blocks.push_back(b);
			}
			[[fallthrough]];
		case 0:
			if (!block.empty()) {
				blocks.push_back(block);
			}
			break;
	}
	// Copy 'replaces' to the main blocks vector
	for (const auto& b : replaces) {
		blocks.push_back(b);
	}
}

void Expression::replace(Block block) {
	block.set_operator(REPLACE);
	switch (optimization_level) {
		default:
			[[fallthrough]];
		case 3:
			[[fallthrough]];
		case 2:
			[[fallthrough]];
		case 1:
			[[fallthrough]];
		case 0:
			if (!block.empty()) {
				blocks.push_back(block);
			}
			break;
	}
}

Expression Expression::operator+(Block block) {
	insert(block);
	return *this;
}

Expression Expression::operator-(Block block) {
	remove(block);
	return *this;
}

Expression Expression::operator*(Block block) {
	replace(block);
	return *this;
}
