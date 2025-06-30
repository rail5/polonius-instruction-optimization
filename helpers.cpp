#include "block.h"
#include "expression.h"
#include <iostream>
#include <sstream>
#include <vector>
#include <fstream>

std::vector<std::string> explode(
	const std::string& input,
	char delimiter,
	bool can_escape,
	int maximum_number_of_elements = 0,
	bool preserve_empty = false
) {
	std::vector<std::string> result;
	
	bool escaped = false;

	std::string current_element;
	for (auto& c : input) {
		if (c == '\\') {
			if (escaped) {
				// If we are already escaped or escaping is not allowed, treat it as a normal character
				current_element += '\\';
				current_element += c;
				escaped = false; // Reset the escaped state
				continue;
			}

			if (!can_escape) {
				current_element += c; // Add the backslash as a normal character
				escaped = false;
			} else {
				escaped = true;
			}
			continue;
		}

		if (c == delimiter) {
			if (maximum_number_of_elements > 0 && result.size() >= static_cast<size_t>(maximum_number_of_elements - 1)) {
				// If we have reached the maximum number of elements, append the rest of the string to the last element
				if (escaped) {
					current_element += '\\';
					escaped = false;
				}
				current_element += c; // Add the delimiter to the last element
				continue;
			}

			if (escaped) {
				current_element += c;
				escaped = false;
			} else {
				// If we are not escaped, treat it as a delimiter
				if (!current_element.empty() || preserve_empty) {
					result.push_back(current_element);
					current_element.clear();
				}
			}
			continue;
		}

		// If we reach here, it means we are not dealing with an escape character or a delimiter
		if (escaped) {
			current_element += '\\';
			escaped = false;
		}
		current_element += c;
	}
	if (!current_element.empty() || preserve_empty) {
		result.push_back(current_element);
	}
	return result;
}

static uint64_t step_counter = 0;
static std::string original_sequence = "";

bool parse_instruction(const std::string& instruction, Expression* expression) {
	extern uint64_t step_counter;
	extern std::string original_sequence;
	extern bool debug;
	std::vector<std::string> parts = explode(instruction, ' ', true, 3);
	step_counter++;

	if (parts.size() != 3) {
		std::cerr << "Invalid instruction: " << instruction << std::endl;
		return false;
	}
	std::string operation = parts[0];

	// Set operation to uppercase
	for (auto& c : operation) {
		c = toupper(c);
	}

	std::string position = parts[1];
	std::string value = parts[2];

	if (debug) {
		std::cout << "		ADDING INSTRUCTION: " << instruction << std::endl;
		original_sequence += instruction + "\n";
	}
	
	if (operation == "INSERT") {
		Block block;
		block.add(std::stoull(position), value);
		expression->insert(std::move(block));
	} else if (operation == "REMOVE") {
		Block block;
		block.add(std::stoull(position), std::stoull(value));
		expression->remove(std::move(block));
	} else if (operation == "REPLACE") {
		Block block;
		block.add(std::stoull(position), value);
		expression->replace(std::move(block));
	} else {
		std::cerr << "Unknown operation: " << operation << std::endl;
		return false;
	}
	if (debug) {
		std::cout << "		INSTRUCTION SEQUENCE AT STEP " << step_counter << ":"
			<< std::endl
			<< *expression
			<< std::endl;
		
		// Write the original sequence to a new file
		std::ofstream output_file("debug/original-" + std::to_string(step_counter) + ".txt", std::ios::trunc);
		if (!output_file) {
			std::cerr << "Failed to open output file for writing." << std::endl;
			return false;
		}
		output_file << original_sequence;
		output_file.close();

		// Write the current expression to a new file
		std::ofstream expression_file("debug/optimized-" + std::to_string(step_counter) + ".txt", std::ios::trunc);
		if (!expression_file) {
			std::cerr << "Failed to open expression file for writing." << std::endl;
			return false;
		}
		expression_file << expression->print_expression_as_instructions();
		expression_file.close();
	}
	
	return true;
}

std::string get_instruction_type(const std::string& instruction_line) {
	// Split the instruction line by spaces
	std::vector<std::string> parts = explode(instruction_line, ' ', true);
	if (parts.empty()) {
		return "";
	}
	return parts[0]; // Return the first part as the instruction type
}

bool parse_instruction_line(const std::string& instruction_line, Expression* expression) {
	if (instruction_line.empty()) {
		return true; // Empty line, nothing to parse
	}
	std::vector<std::string> parts = explode(instruction_line, ';', true);
	// Get instruction type
	std::string instruction_type = get_instruction_type(parts[0]);

	if (!parse_instruction(parts[0], expression)) {
		std::cerr << "Failed to parse instruction: " << parts[0] << std::endl;
		return false;
	}

	for (size_t i = 1; i < parts.size(); ++i) {
		if (parts[i].empty()) {
			continue; // Skip empty parts
		}
		std::string this_instruction = instruction_type + parts[i];
		if (!parse_instruction(this_instruction, expression)) {
			std::cerr << "Failed to parse instruction: " << parts[i] << std::endl;
			return false;
		}
	}
	return true;
}

bool parse_instruction_sequence(const std::string& instruction_sequence, Expression* expression) {
	std::vector<std::string> instructions = explode(instruction_sequence, '\n', true);
	for (const auto& instruction : instructions) {
		if (!parse_instruction_line(instruction, expression)) {
			return false;
		}
	}
	return true;
}
