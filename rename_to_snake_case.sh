#!/bin/bash

# This script renames .cpp, .h, and .hh files to snake_case convention
# and updates all references to these files in the codebase

# Function to convert a filename to snake_case
to_snake_case() {
  local filename=$(basename "$1")
  local dirname=$(dirname "$1")
  local ext="${filename##*.}"
  local base="${filename%.*}"
  
  # Convert camelCase/PascalCase to snake_case
  # Insert underscore before uppercase letters and convert to lowercase
  local snake=$(echo "$base" | sed -E 's/([A-Z])/_\L\1/g' | sed 's/^_//')
  echo "$dirname/$snake.$ext"
}

# Find all .cpp, .h, and .hh files and process them
find /media/pan/other/project/git_project/my_project/dawn -type f \( -name "*.cpp" -o -name "*.h" -o -name "*.hh" \) | while read file; do
  new_name=$(to_snake_case "$file")
  
  # Skip if already in snake_case
  if [ "$file" != "$new_name" ]; then
    echo "Renaming $file to $new_name"
    
    # Get the old and new base filenames for include updates
    old_base=$(basename "$file")
    new_base=$(basename "$new_name")
    
    # Update all include statements in the codebase
    find /media/pan/other/project/git_project/my_project/dawn -type f \( -name "*.cpp" -o -name "*.h" -o -name "*.hh" \) -exec sed -i "s/#include \"$old_base\"/#include \"$new_base\"/g" {} \;
    
    # Perform the actual rename
    git mv "$file" "$new_name" 2>/dev/null || mv "$file" "$new_name"
  fi
done

echo "All files have been renamed to snake_case convention and references updated."