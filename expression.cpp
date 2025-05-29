#include "expression.h"

#include <memory>

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
	uint64_t left_shift = 0;
	switch (optimization_level) {
		default:
			[[fallthrough]];
		case 3:
			[[fallthrough]];
		case 2:
			// Level 2 optimizations:
			// Apply theorem #3 (eliminating redundant insert/remove pairs)
			
			// Iterate backwards through the blocks
			while (!blocks.empty() && blocks.back().get_operator() != INSERT) {
				Block* last = &blocks.back();
				switch (last->get_operator()) {
					case INSERT:
						throw std::runtime_error("Unexpected INSERT block in the middle of the expression.");
					case REPLACE:
						replaces.push_front(*last);
						blocks.pop_back();
						break;
					case REMOVE:
						// Yes, they have to be EXACTLY EQUAL in order for there to be any redundancy.
						if (last->start() == block.start() + left_shift) {
							// There is a redundancy between the remove and this insert
							// Ie, we're removing some characters and then inserting to the same position
							// This can be simplified to a single replace
							BlockOverlap overlap = last->overlap(block.start() + left_shift, block.end() + left_shift);

							// Simplify these two instructions to a single replace
							Block replace_block = block;
							replace_block.remove(overlap.end - left_shift + 1, block.end());
							replace_block.set_operator(REPLACE);

							// Store the block's original start position
							uint64_t original_start = block.start();

							// Remove the overlap from both instructions
							last->remove(overlap.start, overlap.end);
							block.remove(overlap.start - left_shift, overlap.end - left_shift);

							// After removing the overlap, either:
							// 1. There is *only* redundancy (nothing left over after the end of the overlap)
							// 2. Some of the INSERT is left over after the overlap
							// 3. Some of the REMOVE is left over
							if (last->empty()) {
								blocks.pop_back();
							}

							// Update all of the blocks in-between the redundant remove/insert pair
							for (auto& b : removes) {
								if (b.start() >= original_start + left_shift) {
									b.shift_right(overlap.end - overlap.start + 1);
								} else {
									left_shift -= b.size();
								}
								blocks.push_back(b);
							}
							removes.clear();
							for (auto& b : replaces) {
								if (b.start() >= original_start + left_shift) {
									b.shift_right(overlap.end - overlap.start + 1);
								}
								blocks.push_back(b);
							}
							replaces.clear();
							
							// Add our replacement REPLACE block
							blocks.push_back(replace_block);
							
							// If the INSERT block is now empty, just return
							if (block.empty()) {
								return;
							}
							// Otherwise, fall through to the level 1 optimizations
							break;
						} else if (last->start() < block.start() + left_shift) {
							left_shift += last->size();
						}
						removes.push_front(*last);
						blocks.pop_back();
						break;
				}
			}
			// Add back all the removes and replaces if we've made it this far
			for (auto& b : removes) {
				blocks.push_back(b);
			}
			removes.clear();
			for (auto& b : replaces) {
				blocks.push_back(b);
			}
			replaces.clear();
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
				Block* last = &blocks.back();
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
				Block* last = &blocks.back();
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
