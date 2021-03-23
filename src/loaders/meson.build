subloader_dep = []

if get_option('loaders').contains('svg') == true
    subdir('svg')
    message('Enable SVG Loader')
endif

subdir('raw')
subdir('tvg')

loader_dep = declare_dependency(
   dependencies: subloader_dep,
   include_directories : include_directories('.'),
)
