#include "expression.h"

Expression::Expression() = default;

Expression::Expression(uint8_t optimization_level) {
	this->optimization_level = optimization_level;
}


void Expression::set_optimization_level(uint8_t level) {
	optimization_level = level;

	// If we change the optimization level, we need to re-evaluate the expression
	// If we don't do this, the expression will not be optimized correctly when we add more terms
	// (And will become, in fact, an incorrect and potentially invalid expression)
	// E.g, if we've *been* optimizing according to -O1,
	// And we now set the optimization level to -O2,
	// We need to re-evaluate the expression to apply the new optimizations from the beginning
	re_evaluate();
}

void Expression::re_evaluate() {
	// Re-evaluate the entire expression

	std::deque<Block> blocks_copy = std::move(blocks);
	blocks.clear();

	for (auto& block : blocks_copy) {
		switch (block.get_operator()) {
			case INSERT:
				insert(std::move(block));
				break;
			case REMOVE:
				remove(std::move(block));
				break;
			case REPLACE:
				replace(std::move(block));
				break;
		}
	}
}

void Expression::insert(Block&& block) {
	block.set_operator(INSERT);
	std::deque<Block> inserts_before_this_instruction;
	std::deque<Block> inserts_after_this_instruction;
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
						replaces.emplace_front(std::move(*last));
						blocks.pop_back();
						break;
					case REMOVE:
						if (last->start() < block.start() + left_shift) {
							left_shift += last->size(); // Track position shifts
						} else if (last->start() == block.start() + left_shift) {
							// Yes, they have to be EXACTLY EQUAL in order for there to be any redundancy.

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
									left_shift -= b.size(); // This will undo each position shift one-by-one in reverse order
											// Ie, left_shift == 0 at the end of this loop
								}
								blocks.emplace_back(std::move(b));
							}
							removes.clear();
							for (auto& b : replaces) {
								if (b.start() >= original_start + left_shift) {
									b.shift_right(overlap.end - overlap.start + 1);
								}
								blocks.emplace_back(std::move(b));
							}
							replaces.clear();
							
							// Add our replacement REPLACE block
							blocks.emplace_back(std::move(replace_block));
							
							// If the INSERT block is now empty, just return
							if (block.empty()) {
								return;
							}
							// Otherwise, fall through to the level 1 optimizations
							break;
						}
						removes.emplace_front(std::move(*last));
						blocks.pop_back();
						break;
				}
			}
			// Add back all the removes and replaces if we've made it this far
			for (auto& b : removes) {
				blocks.emplace_back(std::move(b));
			}
			removes.clear();
			for (auto& b : replaces) {
				blocks.emplace_back(std::move(b));
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
			// So let's move this one on up to the INSERT instructions
			while (!blocks.empty()) {
				Block* last = &blocks.back();
				BlockOverlap overlap;
				switch (last->get_operator()) {
					case INSERT:
						// Make sure the INSERT instructions are always sorted
						if (last->start() > block.start()) {
							last->shift_right(block.size());
							inserts_after_this_instruction.emplace_front(std::move(*last));
						} else {
							// If there's an overlap between these two blocks,
							// They must be combined into a single block in order to
							// be executed on a single pass of the file
							// E.g, 'insert 0 abc' followed by 'insert 1 x'
							// *must* become 'insert 0 axbc'
							Block combined = combine_inserts(*last, block);
							if (!combined.empty()) {
								block.clear();
								inserts_after_this_instruction.emplace_front(std::move(combined));
							} else {
								inserts_before_this_instruction.emplace_front(std::move(*last));
							}
						}
						blocks.pop_back();
						break;
					case REMOVE:
						if (last->start() <= block.start()) {
							block.shift_right(last->size());
						} else {
							last->shift_right(block.size());
						}
						removes.emplace_front(std::move(*last));
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
								replaces.emplace_front(std::move(pre_overlap));
							}
							if (!post_overlap.empty()) {
								post_overlap.shift_right(block.size());
								replaces.emplace_front(std::move(post_overlap));
							}
						} else if (last->start() >= block.start()) {
							last->shift_right(block.size());
							replaces.emplace_front(std::move(*last));
							blocks.pop_back();
						} else {
							replaces.emplace_front(std::move(*last));
							blocks.pop_back();
						}
						break;
				}
			}
			for (auto& insert : inserts_before_this_instruction) {
				blocks.emplace_back(std::move(insert));
			}
			[[fallthrough]];
		case 0:
			if (!block.empty()) {
				blocks.emplace_back(std::move(block));
			}
			break;
	}
	// Re-add the old inserts, removes, and replaces
	for (auto& insert : inserts_after_this_instruction) {
		blocks.emplace_back(std::move(insert));
	}
	for (auto& remove : removes) {
		blocks.emplace_back(std::move(remove));
	}
	for (auto& replace : replaces) {
		blocks.emplace_back(std::move(replace));
	}
}

void Expression::remove(Block&& block) {
	block.set_operator(REMOVE);
	std::deque<Block> removes_before_this_instruction;
	std::deque<Block> removes_after_this_instruction;
	std::deque<Block> replaces;
	BlockOverlap overlap;
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
			// So let's move this one to the REMOVE section
			while (!blocks.empty() && blocks.back().get_operator() != INSERT) {
				Block* last = &blocks.back();
				switch (last->get_operator()) {
					case INSERT:
						break; // This will never happen
					case REMOVE:
						// Ensure our REMOVE instructions are always sorted
						{
							Block combined = combine_removes(*last, block);
							if (!combined.empty()) {
								block = std::move(combined);
							} else {
								// Can't be combined, can be re-ordered
								if (last->start() < block.start()) {
									removes_before_this_instruction.emplace_front(std::move(*last));
								} else {
									last->shift_left(block.size());
									removes_after_this_instruction.emplace_front(std::move(*last));
								}
							}
						}
						blocks.pop_back();
						break;
					case REPLACE:
						overlap = last->overlap(block);
						if (!overlap.empty) {
							Block pre_overlap = *last;
							Block post_overlap = *last;
							pre_overlap.remove(overlap.start, last->end());
							post_overlap.remove(last->start(), overlap.end);

							blocks.pop_back();

							if (!pre_overlap.empty()) {
								replaces.emplace_front(std::move(pre_overlap));
							}
							if (!post_overlap.empty()) {
								post_overlap.shift_left(block.size());
								replaces.emplace_front(std::move(post_overlap));
							}
						} else if (last->start() >= block.start()) {
							last->shift_left(block.size());
							replaces.emplace_front(std::move(*last));
							blocks.pop_back();
						} else {
							replaces.emplace_front(std::move(*last));
							blocks.pop_back();
						}
						break;
				}
			}
			for (auto& b : removes_before_this_instruction) {
				blocks.emplace_back(std::move(b));
			}
			[[fallthrough]];
		case 0:
			if (!block.empty()) {
				blocks.emplace_back(std::move(block));
			}
			break;
	}
	// Re-add the REMOVEs and REPLACEs
	for (auto& b : removes_after_this_instruction) {
		blocks.emplace_back(std::move(b));
	}
	for (auto& b : replaces) {
		blocks.emplace_back(std::move(b));
	}
}

void Expression::replace(Block&& block) {
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
				blocks.emplace_back(std::move(block));
			}
			break;
	}
}

Expression Expression::operator+(Block&& block) {
	insert(std::move(block));
	return *this;
}

Expression Expression::operator-(Block&& block) {
	remove(std::move(block));
	return *this;
}

Expression Expression::operator*(Block&& block) {
	replace(std::move(block));
	return *this;
}
