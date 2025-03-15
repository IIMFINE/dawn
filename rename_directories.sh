#!/bin/bash

# This script renames directories with uppercase letters to snake_case

# Function to convert a directory name to snake_case
to_snake_case() {
  local dirname="$1"
  
  # Convert camelCase/PascalCase to snake_case
  # Insert underscore before uppercase letters and convert to lowercase
  local snake=$(echo "$dirname" | sed -E 's/([A-Z])/_\L\1/g' | sed 's/^_//')
  echo "$snake"
}

# Find all directories with uppercase letters
find /media/pan/other/project/git_project/my_project/dawn -type d | grep -P "[A-Z]" | sort -r | while read dir; do
  dir_name=$(basename "$dir")
  parent_dir=$(dirname "$dir")
  new_dir_name=$(to_snake_case "$dir_name")
  
  if [ "$dir_name" != "$new_dir_name" ]; then
    echo "Renaming directory: $dir to $parent_dir/$new_dir_name"
    
    # Perform the actual rename
    git mv "$dir" "$parent_dir/$new_dir_name" 2>/dev/null || mv "$dir" "$parent_dir/$new_dir_name"
    
    # Update includes and file references
    find /media/pan/other/project/git_project/my_project/dawn -type f -name "*.cpp" -o -name "*.h" -o -name "*.hh" -o -name "CMakeLists.txt" | xargs grep -l "$dir_name" | while read file; do
      sed -i "s|$dir_name|$new_dir_name|g" "$file"
    done
  fi
done

echo "All directories have been renamed to snake_case convention."