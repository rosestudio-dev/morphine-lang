project(
    'app-morphine', 'c',
    default_options : ['c_std=c17']
)

version = get_option('version')
versioncode = get_option('versioncode')
installdir = get_option('installdir')
sanitize = get_option('sanitize')

morphine = subproject('morphine').get_variable('morphine_dep')
compiler = subproject('compiler').get_variable('morphinec_dep')

sources = run_command(
    'python',
    meson.source_root() / 'extra' / 'scripts' / 'walker.py',
    '-p',
    'src/**/*.c',
    check : true
).stdout().strip().split('\n')

include = include_directories('include')

warning_flags = [
    '-Wall',
    '-Wextra',
    '-Werror',
    '-Wno-error=conversion',
    '-Wno-error=strict-prototypes',
    '-Wdouble-promotion',
    '-Wshadow',
    '-Wformat',
    '-Wunused-parameter',
    '-Wconversion',
    '-Wredundant-decls',
    '-Wmissing-declarations',
    '-Wvla',
    '-Wmissing-prototypes',
    '-Wcast-qual',
    '-Wcast-align',
    '-Woverflow',
]

common_flags = [
    '-fno-common',
    '-fwrapv',
    '-flto'
]

sanitize_flags = [
    '-fsanitize=address',
    '-fsanitize=undefined'
]

compiler_args = warning_flags + common_flags

if sanitize.enabled()
    compiler_args += sanitize_flags
endif

executable(
    'morphine', sources,
    include_directories : include,
    dependencies : [morphine, compiler],
    c_args : compiler_args,

    install : installdir != 'undefined',
    install_dir : installdir
)

summary(
    {
        'version' : version,
        'version_code' : versioncode,
        'install' : installdir != 'undefined',
        'install-dir' : installdir,
        'sanitize' : sanitize,
        'flags' : compiler_args,
        'sources' : sources
    },
    section : 'Configuration',
    bool_yn : true,
    list_sep : ', '
)
