project(
    'morphine',
    default_options : [
        'version=0.0.1',
        'versioncode=1',
        'installdir=' + meson.source_root() / 'install',
        'morphine:sanitize=disabled',
        'compiler:sanitize=disabled',
        'app-morphine:sanitize=disabled',
    ],
    subproject_dir : 'components',
)

installdir = get_option('installdir')
message('Install dir:', installdir)

subproject('app')

if installdir != 'undefined'
    install_data(
        files('extra' / 'templates' / 'gitignore'),
        follow_symlinks : false,
        install_dir : installdir,
        rename : ['.gitignore'],
    )
endif
