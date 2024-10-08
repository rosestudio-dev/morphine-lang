
sources = run_command(
    python, script_walker, '-p', 'src/**/*.c',
    check : true
).stdout().strip().split('\n')

morphine_scripts_paths = run_command(
    python, script_walker, '-p', 'scripts/**/*.ms',
    check : true
).stdout().strip().split('\n')

morphine_scripts = custom_target(
    command : [
        python, script_data,
        '-s', meson.current_source_dir(),
        '-f', ':'.join(morphine_scripts_paths),
        '-p', 'scripts/',
        '-o', '@OUTPUT@'
    ],
    input : morphine_scripts_paths,
    output : 'scripts.h'
)

cc = meson.get_compiler('c')
readline_dep = cc.find_library('readline')

addition_args = []

if readline_dep.found()
    addition_args += ['-DUSE_READLINE']
endif

executable(
    'morphine', sources, morphine_scripts,
    include_directories : ['include'],
    dependencies : [
        morphine_dep,
        morphinec_dep,
        morphinel_dep,
        readline_dep
    ],
    c_args : c_args + addition_args,
    link_args : [link_args, '-flto'],
    build_by_default : true,
    install : true
)
