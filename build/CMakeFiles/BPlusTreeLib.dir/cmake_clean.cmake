file(REMOVE_RECURSE
  "libBPlusTreeLib.a"
  "libBPlusTreeLib.pdb"
)

# Per-language clean rules from dependency scanning.
foreach(lang )
  include(CMakeFiles/BPlusTreeLib.dir/cmake_clean_${lang}.cmake OPTIONAL)
endforeach()
