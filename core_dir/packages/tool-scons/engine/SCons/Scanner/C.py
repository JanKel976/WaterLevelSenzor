"""SCons.Scanner.C

This module implements the dependency scanner for C/C++ code.

"""

#
# __COPYRIGHT__
#
# Permission is hereby granted, free of charge, to any person obtaining
# a copy of this software and associated documentation files (the
# "Software"), to deal in the Software without restriction, including
# without limitation the rights to use, copy, modify, merge, publish,
# distribute, sublicense, and/or sell copies of the Software, and to
# permit persons to whom the Software is furnished to do so, subject to
# the following conditions:
#
# The above copyright notice and this permission notice shall be included
# in all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY
# KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
# WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
# NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
# LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
# OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
# WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
#

__revision__ = "__FILE__ __REVISION__ __DATE__ __DEVELOPER__"

import SCons.Node.FS
import SCons.Scanner
import SCons.Util

import SCons.cpp

class SConsCPPScanner(SCons.cpp.PreProcessor):
    """
    SCons-specific subclass of the cpp.py module's processing.

    We subclass this so that: 1) we can deal with files represented
    by Nodes, not strings; 2) we can keep track of the files that are
    missing.
    """
    def __init__(self, *args, **kw):
        SCons.cpp.PreProcessor.__init__(self, *args, **kw)
        self.missing = []
        self._known_paths = []

    def initialize_result(self, fname):
        self.result = SCons.Util.UniqueList([fname])

    def find_include_file(self, t):
        keyword, quote, fname = t
        paths = tuple(self._known_paths) + self.searchpath[quote]
        if quote == '"':
            paths = (self.current_file.dir, ) + paths
        result = SCons.Node.FS.find_file(fname, paths)
        if result:
            result_path = result.get_abspath()
            for p in self.searchpath[quote]:
                if result_path.startswith(p.get_abspath()):
                    self._known_paths.append(p)
                    break
        else:
            self.missing.append((fname, self.current_file))
        return result

    def read_file(self, file):
        try:
            with open(str(file.rfile())) as fp:
                return fp.read()
        except EnvironmentError as e:
            self.missing.append((file, self.current_file))
            return ''

def dictify_CPPDEFINES(env):
    cppdefines = env.get('CPPDEFINES', {})
    if cppdefines is None:
        return {}
    if SCons.Util.is_Sequence(cppdefines):
        result = {}
        for c in cppdefines:
            if SCons.Util.is_Sequence(c):
                # handle tuple with 1 item (e.g. tuple("DEFINE", ))
                if len(c) > 1:
                    result[c[0]] = c[1]
                else:
                    result[c[0]] = None
            else:
                result[c] = None
        return result
    if not SCons.Util.is_Dict(cppdefines):
        return {cppdefines : None}
    return cppdefines

class SConsCPPScannerWrapper(object):
    """
    The SCons wrapper around a cpp.py scanner.

    This is the actual glue between the calling conventions of generic
    SCons scanners, and the (subclass of) cpp.py class that knows how
    to look for #include lines with reasonably real C-preprocessor-like
    evaluation of #if/#ifdef/#else/#elif lines.
    """
    def __init__(self, name, variable):
        self.name = name
        self.path = SCons.Scanner.FindPathDirs(variable)
    def __call__(self, node, env, path = (), depth=-1):
        cpp = SConsCPPScanner(current = node.get_dir(),
                              cpppath = path,
                              dict = dictify_CPPDEFINES(env),
                              depth = depth)
        result = cpp(node)
        for included, includer in cpp.missing:
            fmt = "No dependency generated for file: %s (included from: %s) -- file not found"
            SCons.Warnings.warn(SCons.Warnings.DependencyWarning,
                                fmt % (included, includer))
        return result

    def recurse_nodes(self, nodes):
        return nodes
    def select(self, node):
        return self


def CConditionalScanner():
    """
    Return an advanced conditional Scanner instance for scanning source files

    Interprets C/C++ Preprocessor conditional syntax
    (#ifdef, #if, defined, #else, #elif, etc.).
    """
    return SConsCPPScannerWrapper("CScanner", "CPPPATH")


def CScanner():
    """Return a simplified classic Scanner instance for scanning source files

    Takes into account the type of bracketing used to include the file, and
    uses classic CPP rules for searching for the files based on the bracketing.
    """
    return SCons.Scanner.ClassicCPP(
        "CScanner", "$CPPSUFFIXES", "CPPPATH",
        '^[ \t]*#[ \t]*(?:include|import)[ \t]*(<|")([^>"]+)(>|")')


# Local Variables:
# tab-width:4
# indent-tabs-mode:nil
# End:
# vim: set expandtab tabstop=4 shiftwidth=4:
