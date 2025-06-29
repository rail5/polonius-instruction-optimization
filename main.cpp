#include "block.h"
#include "expression.h"

#include <iostream>
#include <fstream>
#include <string>
#include <getopt.h>

static bool debug = false;

#include "helpers.cpp"

int main(int argc, char* argv[]) {
	Expression expression;
	// getopt
	int opt;
	while ((opt = getopt(argc, argv, "O:s:df:")) != -1) {
		switch (opt) {
			case 'O':
				expression.set_optimization_level(static_cast<uint8_t>(std::stoi(optarg)));
				break;
			case 's':
				// Interpret instruction sequence
				if (!parse_instruction_sequence(optarg, &expression)) {
					std::cerr << "Failed to parse instruction sequence: " << optarg << std::endl;
					return 1;
				}
				break;
			case 'd':
				debug = true;
				// Remove all *.txt files under the 'debug' directory
				if (system("rm -f debug/*.txt") != 0) {
					std::cerr << "Failed to remove files in 'steps' directory." << std::endl;
					return 1;
				}
				break;
			case 'f':
				// Read instructions from a file
				{
					std::ifstream file(optarg);
					if (!file) {
						std::cerr << "Failed to open file: " << optarg << std::endl;
						return 1;
					}
					// Read the entire file into a single string
					std::string instructions((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
					file.close();
					// Parse the instructions
					if (!parse_instruction_sequence(instructions, &expression)) {
						std::cerr << "Failed to parse instructions from file: " << optarg << std::endl;
						return 1;
					}
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

	if (!debug) {
		std::cout << expression << std::endl;
	}
}
