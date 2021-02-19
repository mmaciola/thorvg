examples_dep = dependency('elementary', required : true)

source_file = [
'Tvg.cpp',
]

foreach current_file : source_file
    name = current_file.split('.')[0]
    executable(name, current_file,
           include_directories : headers,
           link_with : thorvg_lib,
           dependencies : examples_dep)
endforeach
