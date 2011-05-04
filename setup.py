
from distutils.core import setup, Extension

module1 = Extension('dmultitouch',
                    define_macros = [('MAJOR_VERSION', '1'),
                                     ('MINOR_VERSION', '0')],
                    include_dirs = ['.', '/usr/include'],
                    libraries = ['cv'],
                    library_dirs = ['/usr/lib'],
                    sources = ['dmultitouchmodule.c'])

setup (name = 'DMultitouch',
       version = '1.0',
       description = 'Depth based multitouch library',
       author = 'Kenneth Bogert',
       author_email = 'kbogert@uga.edu',
       url = 'http://docs.python.org/extending/building',
       long_description = '''
A library for getting multitouch data out of depth maps.
''',
       ext_modules = [module1])
