#include "block.h"

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
