#include "block.h"

std::vector<std::pair<uint64_t, char>> Block::get_data() const {
	return data;
}

void Block::set_operator(Operator op) {
	this->op = op;
}

Operator Block::get_operator() const {
	return op;
}

uint64_t Block::size() const {
	return data.size();
}

uint64_t Block::start() const {
	if (data.empty()) {
		return 0;
	}
	return data.front().first;
}

uint64_t Block::end() const {
	if (data.empty()) {
		return 0;
	}
	return data.back().first;
}

char Block::at(uint64_t index) const {
	if (data.empty()) {
		return 0;
	}

	if (index > end() || index < start()) {
		return 0;
	}

	for (const auto& pair : data) {
		if (pair.first == index) {
			return pair.second;
		}
	}

	return 0;
}

void Block::add(uint64_t start_position, const std::string& value) {
	data.clear();
	for (size_t i = 0; i < value.size(); ++i) {
		data.push_back(std::make_pair(start_position + i, value[i]));
	}
}

void Block::add(uint64_t start_position, uint64_t end_position) {
	for (uint64_t i = start_position; i <= end_position; ++i) {
		data.push_back(std::make_pair(i, 0)); // Using 0 to indicate a remove operation
											// This is janky and should be changed in production
	}
}

void Block::remove(uint64_t start_position, uint64_t end_position) {
	if (data.empty()) {
		return;
	}

	// Remove elements in the specified range
	data.erase(std::remove_if(data.begin(), data.end(),
		[start_position, end_position](const std::pair<uint64_t, char>& pair) {
			return pair.first >= start_position && pair.first <= end_position;
		}), data.end());
}

bool Block::shift_left(uint64_t shift_amount) {
	if (data.empty()) {
		return false;
	}

	// Reject if the shift would cause the start position to go negative
	if (shift_amount > start()) {
		return false;
	}

	// Shift the data
	for (auto& pair : data) {
		pair.first -= shift_amount;
	}
	
	return true;
}

bool Block::shift_right(uint64_t shift_amount) {
	if (data.empty()) {
		return false;
	}

	// Shift the data
	for (auto& pair : data) {
		pair.first += shift_amount;
	}
	
	return true;
}

bool Block::empty() const {
	return data.empty();
}

/**
 * @brief Calculates the overlap of two blocks.
 * 
 * For example:
 * Block A:
 * 		0	1	2	3	4
 * 		a	b	c	d	e
 * Block B:
 * 		2	3	4	5
 * 		x	y	z	a
 * They overlap between positions 2 and 4
 * 
 * The first element of the pair is the start of the overlap,
 * and the second element is the end of the overlap.
 * If there is no overlap, both elements will be 0.
 */
std::pair<uint64_t, uint64_t> Block::overlap(const Block& b) const {
	if (empty() || b.empty()) {
		return std::pair<uint64_t, uint64_t>(0, 0);
	}

	uint64_t start_overlap = 0;
	uint64_t end_overlap = 0;
	if (start() <= b.end() && end() >= b.start()) {
		start_overlap = std::max(start(), b.start());
		end_overlap = std::min(end(), b.end());
	}
	return std::pair<uint64_t, uint64_t>(start_overlap, end_overlap);
}
