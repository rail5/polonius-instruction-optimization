#include "block.h"
#include "expression.h"

#include <iostream>
#include <string>
#include <getopt.h>

#include <sstream>
#include <vector>

#include "helpers.cpp"

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
