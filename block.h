#ifndef BLOCK_H
#define BLOCK_H

#include <string>
#include <vector>
#include <algorithm>

enum InstructionType {
	INSERT,
	REMOVE,
	REPLACE
};

struct BlockOverlap {
	uint64_t start = 0;
	uint64_t end = 0;
	bool empty = true;
};

/**
 * @class Block
 * @brief Represents a block of data with a start and end position.
 * 
 * The concept of 'blocks' is explained in https://github.com/rail5/polonius/wiki/Instruction-Optimization
 * 
 * A block can be a representation of a file, a section of a file, or several disjoint sections of a file.
 * The data contained in the block does not need to be contiguous.
 * 
 * For example, suppose we have a file with the following contents:
 * 		abc def ghi
 * 
 * A block representing the entire file would look like this:
 * 		0	1	2	3	4	5	6	7	8	9	10
 * 		a	b	c	 	d	e	f	 	g	h	i
 * 				(Call this "Block A")
 * 
 * We could also have a block representing just the first three characters:
 * 		0	1	2
 * 		a	b	c
 * 				(Call this "Block B")
 * 
 * Or a block representing the first three and last three characters:
 * 		0	1	2	8	9	10		-- note the jump from '2' to '8', this is allowed
 * 		a	b	c	g	h	i
 * 
 * One thing we can do, for example, is do math with multiple blocks.
 * For example, if we subtract Block B from Block A, we get:
 * 		A - B =
 * 		0	1	2	3	4	5	6	7
 * 		 	d	e	f	 	g	h	i
 * 
 * Note that this is equivalent to just deleting the initial 'abc' from the file.
 * The rules for operations on blocks are explained in the above-linked document.
 * The gist of it is:
 * 		Addition = Insert instructions
 * 		Subtraction = Remove instructions
 * 		Multiplication = Replace instructions
 * 
 * TODO(@rail5): Masive performance improvements if we can safely assume that all Blocks are contiguous
 */
class Block {
	private:
		std::vector<std::pair<uint64_t, char>> data;
		InstructionType op;

	public:
		Block() = default;
		Block(const Block& other); // Copy constructor
		Block(Block&& other) noexcept; // Move constructor
		std::vector<std::pair<uint64_t, char>> get_data() const;

		void set_operator(InstructionType op);
		InstructionType get_operator() const;

		uint64_t size() const;
		uint64_t start() const;
		uint64_t end() const;

		char at(uint64_t index) const;

		void add(uint64_t start_position, const std::string& value);
		void add(uint64_t start_position, uint64_t end_position);

		void remove(uint64_t start_position, uint64_t end_position);
		void remove_and_shift(uint64_t start_position, uint64_t end_position);

		bool shift_left(uint64_t shift_amount);
		bool shift_right(uint64_t shift_amount);

		bool empty() const;
		
		BlockOverlap overlap(const Block& b) const;
		BlockOverlap overlap(uint64_t start_position, uint64_t end_position) const;
};

#endif // BLOCK_H
