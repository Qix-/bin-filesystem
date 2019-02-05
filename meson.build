project('filesystem', 'cpp', default_options: ['cpp_std=c++11'])

inc = include_directories('.')
dep = declare_dependency(include_directories: inc)

test_exe = executable('path_demo', 'path_demo.cpp', dependencies: [dep])
test('filesystem-test', test_exe, args: [])
