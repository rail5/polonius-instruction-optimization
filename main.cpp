#include "block.h"
#include "expression.h"

#include <iostream>
#include <string>
#include <getopt.h>

#include <sstream>
#include <vector>

std::vector<std::string> explode(std::string const &input, char delimiter, bool can_escape = false, int maximum_number_of_elements = 0) {
	/***
	vector<string> explode(string &input, char delimiter, bool can_escape, int maximum_number_of_elements):
		Similar to PHP's "explode" function
		
		Splits the input string into separate strings, using the provided delimiter
			Example:
				string input = "Test,one,two,three";
				explode(input, ",");
			Would return the following vector:
				{"Test", "one", "two", "three"}
		
		If can_escape == true, then we do NOT split where the delimiter is backslash-escaped.
			Example:
				string input = "Test,one\,two,three";
				explode(input, ",", true);
			Would return the following vector:
				{"Test", "one,two", "three"}
				
		The "maximum_number_of_elements" parameter, if set higher than zero, limits how many elements we can split apart
			Example:
				string input = "Test,one,two,three";
				explode(input, ",", 3);
			Would return the following vector:
				{"Test", "one", "two,three"}
	***/
	// Create the vector
	std::vector<std::string> output_vector;
	
	// Create a stringstream from the input
	std::istringstream input_string_stream(input);

	std::string buffer = "";
	
	// Cycle through the stringstream and push back the substrings
	for (std::string token; std::getline(input_string_stream, token, delimiter); ) {
		if (can_escape) {
			// Need to process escaped delimiters
			if (token[token.length()-1] == '\\') {
				// Escaped delimiter, move to buffer until we hit one that isn't escaped or we run out of text
				buffer += token.replace(token.length()-1, 1, 1, delimiter); // Remove the backslash, re-insert the delimiter
				token = "";
			} else {
				// Real delimiter (not escaped), push a concatenated string of 'the buffer' + 'token' into our vector, and then clear the buffer
				std::string concatenated = buffer + std::move(token);
				output_vector.push_back(std::move(concatenated));
				buffer = "";
			}
		} else {
			// Don't care about escaped chars, just push the token into the vector
			output_vector.push_back(std::move(token));
		}
	}
	// We've run out of text to process. Is the buffer empty?
	if (buffer != "") {
		// If not, we need to push it into the vector as well
		output_vector.push_back(std::move(buffer));
	}
	
	// If maximum_number_of_elements is set, we want to recombine any splits after the maximum
	if (maximum_number_of_elements > 0) {
		// Set the highest index (counting from zero) we want the vector to have
		int last_permissible_element = maximum_number_of_elements - 1;
		
		// Store the current actual highest index of the vector
		int last_element = output_vector.size() - 1;
		
		// Cycle through the vector and combine into the last_permissible_element
		// (re-inserting the delimiter character between previously split elements)
		// Then delete the higher elements
		for (int i = maximum_number_of_elements; i <= last_element; i++) {
			output_vector[last_permissible_element] = output_vector[last_permissible_element] + delimiter + output_vector[last_permissible_element + 1];
			output_vector.erase(output_vector.begin() + (last_permissible_element + 1));
		}
		
		// If the input string originally ended with the delimiter char,
		// Let's put that back in place at the end
		if (input[input.length()-1] == delimiter) {
			output_vector[last_permissible_element] = output_vector[last_permissible_element] + delimiter;
		}
	}
	
	return output_vector;
}

bool parse_instruction(const std::string& instruction, Expression* expression) {
	std::vector<std::string> parts = explode(instruction, ' ', true, 3);

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

	if (operation == "INSERT") {
		Block block;
		block.add(std::stoull(position), value);
		*expression = *expression + block;
	} else if (operation == "REMOVE") {
		Block block;
		block.add(std::stoull(position), std::stoull(value));
		*expression = *expression - block;
	} else if (operation == "REPLACE") {
		Block block;
		block.add(std::stoull(position), value);
		*expression = *expression * block;
	} else {
		std::cerr << "Unknown operation: " << operation << std::endl;
		return false;
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

int main(int argc, char* argv[]) {
	Expression expression;
	// getopt
	int opt;
	while ((opt = getopt(argc, argv, "O:s:")) != -1) {
		switch (opt) {
			case 'O':
				expression.optimization_level = static_cast<uint8_t>(std::stoi(optarg));
				break;
			case 's':
				// Interpret instruction sequence
				if (!parse_instruction_sequence(optarg, &expression)) {
					std::cerr << "Failed to parse instruction sequence: " << optarg << std::endl;
					return 1;
				}
				break;
			default:
				return 1;
		}
	}

	/**
	 * INSERT 0 hello world
	 * REMOVE 0 4
	 * INSERT 0 goodbye
	 * REPLACE 8 abcde
	 * REPLACE 8 buddy
	 * 
	 * result should be 'goodbye buddy'
	 */ 
	

	/**
	 * Expected results:
	 * 		-O0:
	 * 			INSERT 0 hello world
	 * 			REMOVE 0 4
	 * 			INSERT 0 goodbye
	 * 			REPLACE 8 abcde
	 * 			REPLACE 8 buddy
	 * 		-O1:
	 * 			INSERT 0 hello world
	 * 			INSERT 0 goodbye
	 * 			REMOVE 7 11
	 * 			REPLACE 8 abcde
	 * 			REPLACE 8 buddy
	 * 		-O2:
	 * 			INSERT 0  world
	 * 			INSERT 0 goodbye
	 * 			REPLACE 8 abcde
	 * 			REPLACE 8 buddy
	 * 		-O3:
	 * 			INSERT 0  world
	 * 			INSERT 0 goodbye
	 * 			REPLACE 8 buddy
	 */

	std::cout << expression << std::endl;
}
