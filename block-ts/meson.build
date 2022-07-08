# SPDX-License-Identifier: Apache-2.0
# (c) Copyright 2019 - 2022 Xilinx, Inc. All rights reserved.
project('block-ts', 'c',
    version : '1.0.0',
)

te_path = get_option('te_path')
te_libdir = get_option('te_libdir')
add_project_arguments(get_option('te_cflags').split(), language: 'c')
add_project_arguments('-Werror', language : 'c')
add_project_link_arguments(get_option('te_ldflags').split(), language: 'c')

cc = meson.get_compiler('c')

te_tests_info_sh = find_program(join_paths(te_path, 'te_tests_info.sh'))

test_deps = [ dependency('threads') ]

dep_rpcs = dependency('libtirpc', required: false)
test_deps += [ dep_rpcs ]

te_libs = [
    'logger_core',
    'tapi_rpc',
    'rcfrpc',
    'tapi',
    'confapi',
    'conf_oid',
    'rcfapi',
    'rpcxdr',
    'rpc_types',
    'logger_ten',
    'ipc',
    'tools',
    'ndn',
    'asn',
    'tapi_job',
    'm',
    'tapi_gtest',
    'tapi_fio',
    'tapi_performance',
    'tapi_tool',
    'tapi_env',
    'tapi_bpf',
]

foreach lib : te_libs
    test_deps += cc.find_library(lib, dirs: te_libdir)
endforeach

tests = [ ]

package_dir = 'block-ts'
package_tests_c = []

includes = [ include_directories('lib') ]

subdir('lib')

test_deps += declare_dependency(include_directories: includes,
                                link_with: [ tslib ])

foreach test : tests
    test_exe = test
    test_c = test + '.c'
    package_tests_c += [ test_c ]
    executable(test_exe, test_c, install: true, install_dir: package_dir,
               dependencies: test_deps)
endforeach

install_data([ 'package.xml' ], install_dir: package_dir)

packages = [
    'rbd',
    'blk-proxy',
]

mydir = package_dir
foreach package : packages
    package_dir = join_paths(mydir, package)
    package_tests_c = []
    subdir(package)
endforeach
