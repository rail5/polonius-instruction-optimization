#ifndef EXPRESSION_H_
#define EXPRESSION_H_

#include <deque>
#include <ostream>
#include <string>
#include <cstdint>

#include "block.h"

/**
 * @class Expression
 * @brief Represents a mathematical expression with Blocks as its operands.
 * 
 * The mathematics of Blocks is explained in https://github.com/rail5/polonius/wiki/Instruction-Optimization
 * 
 * By chaining Blocks together in expressions, we can represent Polonius instruction sequences in a way that
 * allows for efficient optimization according to the theorems presented in the above-linked document.
 * 
 * The expression abstractly takes the form:
 * 		<block> <operator> <block> <operator> <block> ...
 * Where <operator> is one of:
 * 		+ (addition) [insert operations]
 * 		- (subtraction) [remove operations]
 * 		* (multiplication) [replace operations]
 * 
 * The expression is evaluated left-to-right, with the first operator being applied to the first two blocks,
 * and the result being used as the first operand for the next operator.
 * There is no operator precedence, so all operators are evaluated in the order they appear.
 * 
 * When appending a new block (and operator) to the expression, the expression is evaluated immediately
 * 	according to the rules of the operator.
 */
class Expression {
	private:
		std::deque<Block> blocks;
		uint8_t optimization_level = 0;

		void re_evaluate();
	public:
		Expression();
		explicit Expression(uint8_t optimization_level);

		// Operator overloads
		Expression operator+(Block block);
		Expression operator-(Block block);
		Expression operator*(Block block);

		void set_optimization_level(uint8_t level);

		std::string print_expression_as_instructions() const {
			std::string result;
			for (auto& block : blocks) {
				bool is_remove = false;
				switch (block.get_operator()) {
					case INSERT:
						result += "INSERT ";
						break;
					case REMOVE:
						result += "REMOVE ";
						is_remove = true;
						break;
					case REPLACE:
						result += "REPLACE ";
						break;
				}
				if (is_remove) {
					result += std::to_string(block.start()) + " " + std::to_string(block.end());
				} else {
					result += std::to_string(block.start()) + " ";
					for (const auto& [position, value] : block.get_data()) {
						if (value == 0) {
							result += "?";
						} else {
							result += value;
						}
					}
				}
				result += "\n";
			}
			return result;
		}

		friend std::ostream& operator<<(std::ostream& os, const Expression& expression) {
			os << expression.print_expression_as_instructions();
			return os;
		}
};

#endif // EXPRESSION_H_
