#include "expression.h"

#include <memory>
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
	std::deque<Block> removes;
	std::deque<Block> replaces;
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

			// The instruction sequence at this moment will look like this:
			// {ALL OF THE INSERT INSTRUCTIONS}
			// {ALL OF THE REMOVE INSTRUCTIONS}
			// {ALL OF THE REPLACE INSTRUCTIONS}
			// {This INSERT instruction}
			// So let's move this one on up to the end of the INSERT instructions
			while (!blocks.empty() && blocks.back().get_operator() != INSERT) {
				std::unique_ptr<Block> last = std::make_unique<Block>(blocks.back());
				BlockOverlap overlap;
				switch (last->get_operator()) {
					case INSERT:
						// This case will never execute
						// It's only here to make the compiler happy
						throw std::runtime_error("Unexpected INSERT block in the middle of the expression.");
					case REMOVE:
						if (last->start() <= block.start()) {
							block.shift_right(last->size());
						} else {
							last->shift_right(block.size());
						}
						removes.push_front(*last);
						blocks.pop_back();
						break;
					case REPLACE:
						overlap = last->overlap(block);
						if (!overlap.empty) {
							Block pre_overlap = *last;
							Block post_overlap = *last;
							pre_overlap.remove(overlap.start, last->end());

							// In the event that the overlap starts at the beginning of the REPLACE block,
							// Calling remove(start, start - 1) can be disasterous
							// E.g, if the block starts at '0', this will underflow and clear the entire block
							// And even if the block doesn't start at 0, we'll end up removing some of what we want to keep
							// Of course, what we want to keep in post_overlap is the entire portion of the block from
							// where the overlap starts to the end
							if (overlap.start != last->start()) {
								post_overlap.remove(last->start(), overlap.start - 1);
							}
							blocks.pop_back();
							if (!pre_overlap.empty()) {
								replaces.push_front(pre_overlap);
							}
							if (!post_overlap.empty()) {
								post_overlap.shift_right(block.size());
								replaces.push_front(post_overlap);
							}
						} else if (last->start() >= block.start()) {
							last->shift_right(block.size());
							replaces.push_front(*last);
							blocks.pop_back();
						} else {
							replaces.push_front(*last);
							blocks.pop_back();
						}
						break;
				}
			}
			[[fallthrough]];
		case 0:
			if (!block.empty()) {
				blocks.push_back(block);
			}
			break;
	}
	// Re-add the removes and replaces
	for (auto& remove : removes) {
		blocks.push_back(remove);
	}
	for (auto& replace : replaces) {
		blocks.push_back(replace);
	}
}

void Expression::remove(Block block) {
	block.set_operator(REMOVE);
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

			// At this point, the instruction sequence will look like:
			// The instruction sequence at this moment will look like this:
			// {ALL OF THE INSERT INSTRUCTIONS}
			// {ALL OF THE REMOVE INSTRUCTIONS}
			// {ALL OF THE REPLACE INSTRUCTIONS}
			// {This remove instruction}
			// So let's move this one to just before the REPLACEs
			while (!blocks.empty() && blocks.back().get_operator() == REPLACE) {
				std::unique_ptr<Block> last = std::make_unique<Block>(blocks.back());
				BlockOverlap overlap = last->overlap(block);
				if (!overlap.empty) {
					Block pre_overlap = *last;
					Block overlapping_portion = *last;
					Block post_overlap = *last;
					pre_overlap.remove(overlap.start, last->end());
					overlapping_portion.remove(last->start(), overlap.start - 1);
					overlapping_portion.remove(overlap.end + 1, last->end());
					post_overlap.remove(last->start(), overlap.end);

					blocks.pop_back();

					if (!pre_overlap.empty()) {
						replaces.push_front(pre_overlap);
					}
					if (!post_overlap.empty()) {
						post_overlap.shift_left(block.size());
						replaces.push_front(post_overlap);
					}
					// Discard the overlapping portion
				} else if (last->start() >= block.start()) {
					last->shift_left(block.size());
					replaces.push_front(*last);
					blocks.pop_back();
				} else {
					replaces.push_front(*last);
					blocks.pop_back();
				}
			}
			[[fallthrough]];
		case 0:
			if (!block.empty()) {
				blocks.push_back(block);
			}
			break;
	}
	// Re-add the REPLACEs
	for (auto& b : replaces) {
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
