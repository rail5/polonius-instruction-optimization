#!/usr/bin/env bash

source_file=""
using_source_file=false

if [[ -n "$1" ]]; then
	source_file="$1"
	using_source_file=true
	if [[ ! -f "$source_file" ]]; then
		echo "Source file not found: $source_file"
		exit 1
	fi
else
	echo "You can also pass the source file as the first argument to this script."
	echo "Example: $0 /path/to/original/file.txt"
	echo "Passing a source file will mean that the instructions are executed on that file instead of a new one."
	echo ""
	source_file=$(mktemp)
fi

# The current directory contains a bunch of .txt files:
# original-#*.txt
# And optimized-#*.txt
# Each containing a set of instructions for Polonius to operate based on the source file
# For each of these pairs:
## 1. Copy the source file to a temporary file
## 2. Run the original instructions on the temporary file
## 3. Store the result as the "expected" / "correct" result
## 4. Remove the temporary file
## 5. Copy the source file to a new temporary file
## 6. Run the optimized instructions on the new temporary file
## 7. Compare the result with the expected result
# Once we find a mismatch, we print the number of the .txt file where the problem was introduced

# Run them IN ORDER, not necessarily in the order that shell globs put them in
## Find the highest number in the original-#*.txt files
highest_original=$(ls original-*.txt | sed 's/original-\([0-9]*\).txt/\1/' | sort -n | tail -n 1)

for i in $(seq 1 "$highest_original"); do
	original_file="original-$i.txt"
	optimized_file="optimized-$i.txt"

	if [ ! -f "$original_file" ] || [ ! -f "$optimized_file" ]; then
		echo "Missing files for test $i: $original_file or $optimized_file"
		continue
	fi

	# Create a temporary file for the source
	temp_source=$(mktemp)

	cp "$source_file" "$temp_source"

	# Run the original instructions
	if ! polonius-editor "$temp_source" -f "$original_file"; then
		if [[ $using_source_file == true ]]; then
			echo "Instructions not valid for source file: $source_file"
		else
			echo "Instructions not valid for a new file. Please provide a source file as the first argument to this script."
		fi
		rm "$temp_source"
		exit 1
	fi
	expected_result=$(cat "$temp_source")

	# Clean up the temporary file
	rm "$temp_source"

	# Create a new temporary file for the optimized instructions
	temp_optimized=$(mktemp)

	cp "$source_file" "$temp_optimized"

	# Run the optimized instructions
	if ! polonius-editor "$temp_optimized" -f "$optimized_file"; then
		echo "Optimizer generated invalid instructions at step $i"
	fi
	actual_result=$(cat "$temp_optimized")

	# Clean up the temporary file
	rm "$temp_optimized"

	# Compare results
	if [ "$expected_result" != "$actual_result" ]; then
		echo "Mismatch found in test $i: $original_file vs $optimized_file"

		# Display the diff
		diff --color -u <(printf "%s\n" "$expected_result") <(printf "%s\n" "$actual_result")
		exit 0
	fi

	echo "No mismatch found for test $i: $original_file vs $optimized_file"
done

if [[ $using_source_file != true ]]; then
	# Remove the temporary source file if it was created
	rm "$source_file"
fi
