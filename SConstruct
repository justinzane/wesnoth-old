# vi: syntax=python:et:ts=4
# SCons build description for the Wesnoth-NG project ---------------------------------------------
#
# Thoroughly bastardized from the original. Only tested on GNU/Linux with clang and gcc

import os
import sys
import shutil
import re
import commands
from glob import glob
from subprocess import Popen, PIPE, call
from os import access, F_OK

EnsureSConsVersion(0, 98, 3)

# Warn user of current set of build options.
AddOption('--option-cache',
          dest='option_cache',
          nargs=1, type='string',
          action='store',
          metavar='FILE',
          help='file with cached construction variables',
          default='.scons-option-cache')

if os.path.exists(GetOption("option_cache")):
    optfile = file(GetOption("option_cache"))
    print("Saved options:", optfile.read().replace("\n", ", ")[:-2])
    optfile.close()

# Get the Wesnoth version number --------------------------------------------------------------

config_h_re = re.compile(r"^.*#define\s*(\S*)\s*\"(\S*)\".*$", re.MULTILINE)
build_config = dict(config_h_re.findall(File("src/wesconfig.h").get_contents()))
try:
    version = build_config["VERSION"]
    print "Building Wesnoth version %s" % version
except KeyError:
    print "Couldn't determin the Wesnoth version number, bailing out!"
    sys.exit(1)

# Build-control options -----------------------------------------------------------------------

opts = Variables(GetOption("option_cache"))

def OptionalPath(key, val, env):
    if val:
        PathVariable.PathIsDir(key, val, env)

opts.AddVariables(
    # Basic Build Options ---------------------------------------------------------------------
    ListVariable('default_targets',
                 'Targets that will be built if no target is specified in command line.',
                 "wesnoth,wesnothd,campaignd,cutter,exploder",
                 Split("wesnoth wesnothd campaignd cutter exploder test")),
    EnumVariable('build',
                 'Build variant: debug, release profile or base (no subdirectory)',
                 "release",
                 ["release", "debug", "profile", "base"]),
    BoolVariable('prereqs',
                 'abort if prerequisites cannot be detected',
                 True),
    ('jobs',
     'Set the number of parallel compilations',
     "11",
     lambda key, value, env: int(value),
     int),
    BoolVariable('distcc',
                 'Use distcc',
                 True),
    BoolVariable('ccache',
                 "Use ccache",
                 False),
    BoolVariable('SDL2',
                 'Enable SDL2 instead of SDL1.2.',
                 False),
    BoolVariable('openmp',
                 'Enable openmp use.',
                 False),
    BoolVariable('verbose',
                 'Emit progress messages during data installation.',
                 True),

    # Advanced Build Options ------------------------------------------------------------------
    ('host',
     'Cross-compile host.',
     ''),
    ('cxxtool',
     'Set c++ compiler command if not using standard compiler.',
     'clang++'),
    EnumVariable('cxxstd',
                 'The C++ standard to use. i.e. -std=c++11, etc.',
                 'c++11',
                 ['c++11', 'gnu++11', 'c++1y', 'gnu++1y']),
    EnumVariable('cstd',
                 'The C standard to use. i.e. -std=c11, etc.',
                 'c11',
                 ['c11', 'gnu11']),
    BoolVariable("fast",
                 "Make scons faster at cost of less precise dependency tracking.",
                 False),
    BoolVariable("lockfile",
                 "Use lockfile to prevent multiple instances of scons running at the same time.",
                 True),
    BoolVariable('strict',
                 'Set to strict compilation',
                 False),
    BoolVariable('static_test',
                 'Staticaly build against boost test (Not supported yet)',
                 False),

    # Compiler/Linker Flags -------------------------------------------------------------------
    ('extra_flags_config',
     'Extra compiler and linker flags to use for configuration and all builds',
     "-march=core-avx-i -lprofiler -ltcmalloc"),
    ('extra_flags_base',
     'Extra compiler and linker flags to use for base builds',
     ""),
    ('extra_flags_release',
     'Extra compiler and linker flags to use for release builds',
     "-O4 -fslp-vectorize -fslp-vectorize-aggressive"),
    ('extra_flags_debug',
     'Extra compiler and linker flags to use for debug builds',
     "-O0 -g"),
    ('extra_flags_profile',
     'Extra compiler and linker flags to use for profile builds',
     "-O0 -g"),

    # Optional Components ---------------------------------------------------------------------
    BoolVariable('fribidi',
                 'Clear to disable bidirectional-language support',
                 False),
    BoolVariable('desktop_entry',
                 'Clear to disable desktop-entry',
                 False),
    # TODO: Document what functions are disabled by this flag.
    BoolVariable('lowmem',
                 'Set to reduce memory usage by removing extra functionality',
                 False),
    BoolVariable('notifications',
                 'Enable support for desktop notifications',
                 False),
    BoolVariable('nls',
                 'enable compile/install of gettext message catalogs',
                 False),
    BoolVariable('raw_sockets',
                 'Set True to use raw sockets in the mp network layer instead of SDL_net',
                 False),
    BoolVariable('forum_user_handler',
                 'Enable forum user handler in wesnothd [Warning, links to mysql!]',
                 False),

    # Names -----------------------------------------------------------------------------------
    ('program_suffix',
     'suffix to append to names of installed programs',
     "$version_suffix"),
    ('version_suffix',
     'suffix that will be added to default values of prefsdir, program_suffix and datadirname',
     "-ng"),
    ('server_gid',
     'group id of the user who runs wesnothd',
     ""),
    ('server_uid',
     'user id of the user who runs wesnothd',
     ""),

    # Paths and Directory Locations -----------------------------------------------------------
    PathVariable('build_dir',
                 'Build all intermediate files(objects, test programs, etc) under this dir',
                 "build",
                 PathVariable.PathAccept),
    PathVariable('bindir',
                 'Where to install binaries',
                 "bin",
                 PathVariable.PathAccept),
    ('cachedir',
     'Directory that contains a cache of derived files.',
     ''),
    PathVariable('datadir',
                 'read-only architecture-independent game data',
                 "$datarootdir/$datadirname",
                 PathVariable.PathAccept),
    PathVariable('fifodir',
                 'directory for the wesnothd fifo socket file',
                 "/var/run/wesnothd",
                 PathVariable.PathAccept),
    BoolVariable('systemd',
                 'Install systemd unit file for wesnothd',
                 bool(WhereIs("systemd"))),
    PathVariable('datarootdir',
                 'sets the root of data directories to a non-default location',
                 "share",
                 PathVariable.PathAccept),
    PathVariable('datadirname',
                 'sets the name of data directory',
                 "wesnoth$version_suffix",
                 PathVariable.PathAccept),
    PathVariable('desktopdir',
                 'sets the desktop entry directory to a non-default location',
                 "$datarootdir/applications",
                 PathVariable.PathAccept),
    PathVariable('icondir',
                 'sets the icons directory to a non-default location',
                 "$datarootdir/icons",
                 PathVariable.PathAccept),
    BoolVariable('internal_data',
                 'Set to put data in Mac OS X application fork',
                 False),
    PathVariable('localedirname',
                 'sets the locale data directory to a non-default location',
                 "translations",
                 PathVariable.PathAccept),
    PathVariable('mandir',
                 'sets the man pages directory to a non-default location',
                 "$datarootdir/man",
                 PathVariable.PathAccept),
    PathVariable('docdir',
                 'sets the doc directory to a non-default location',
                 "$datarootdir/doc/wesnoth",
                 PathVariable.PathAccept),
    PathVariable('python_site_packages_dir',
                 'sets the directory where python modules are installed',
                 "lib/python/site-packages/wesnoth",
                 PathVariable.PathAccept),
    PathVariable('prefix',
                 'autotools-style installation prefix',
                 "/usr/local",
                 PathVariable.PathAccept),
    PathVariable('prefsdir',
                 'user preferences directory',
                 "",
                 PathVariable.PathAccept),
    PathVariable('default_prefs_file',
                 'default preferences file name',
                 "",
                 PathVariable.PathAccept),
    PathVariable('destdir',
                 'prefix to add to all installation paths.',
                 "/",
                 PathVariable.PathAccept),
    PathVariable('sdldir',
                 'Directory of SDL installation.',
                 "",
                 OptionalPath),
    PathVariable('boostdir',
                 'Directory of boost installation.',
                 "",
                 OptionalPath),
    PathVariable('boostlibdir',
                 'Directory where boost libraries are installed.',
                 "",
                 OptionalPath),
    ('boost_suffix',
     'Suffix of boost libraries.'),
    PathVariable('gettextdir',
                 'Root directory of Gettext\'s installation.',
                 "",
                 OptionalPath),
    PathVariable('gtkdir',
                 'Directory where GTK SDK is installed.',
                 "",
                 OptionalPath),
    PathVariable('luadir',
                 'Directory where Lua binary package is unpacked.',
                 "",
                 OptionalPath),
)

# Setup ---------------------------------------------------------------------------------------
sys.path.insert(0, "./scons")

env = Environment(tools=["tar",
                         "gettext",
                         "install",
                         "python_devel",
                         "scanreplace"], options=opts, toolpath=["scons"])

if env["lockfile"]:
    print "Creating lockfile"
    lockfile = os.path.abspath("scons.lock")
    open(lockfile, "wx").write(str(os.getpid()))
    import atexit
    atexit.register(os.remove, lockfile)

opts.Save(GetOption("option_cache"), env)
env.SConsignFile("$build_dir/sconsign.dblite")

# Make sure the user's environment is always available
env['ENV']['PATH'] = os.environ["PATH"]
env['ENV']['TERM'] = os.environ["TERM"]
try:
    env['ENV']['INCLUDE_SERVER_PORT'] = os.environ['INCLUDE_SERVER_PORT']
    env['ENV']['INCLUDE_SERVER_DIR'] = os.environ['INCLUDE_SERVER_DIR']
    env['ENV']['INCLUDE_SERVER_PID'] = os.environ['INCLUDE_SERVER_PID']
except KeyError as e:
    print(e.message)

if env["PLATFORM"] == "win32":
    env.Tool("mingw")
else:
    from cross_compile import *
    setup_cross_compile(env)

if env['cxxtool'] in ("clang++", "clang", "llvm"):
    env['CC'] = 'clang'
    env['CXX'] = "clang++"
    env.Tool('llvm')
    env.Append(LINKFLAGS="-Wl,-plugin,/usr/lib/LLVMgold.so")
elif env['cxxtool'] in (None, "", "g++"):
    env['CC'] = "gcc"
    env['CXX'] = "g++"

if 'HOME' in os.environ:
    env['ENV']['HOME'] = os.environ['HOME']

if env['jobs'] > 1:
    SetOption("num_jobs", env['jobs'])

if env['distcc'] or env['ccache']:
    env.Tool('ccwrappers')

Help("""Arguments may be a mixture of switches and targets in any order.
Switches apply to the entire build regardless of where they are in the order.
Important switches include:

    prefix=/usr     probably what you want for production tools
    build=base      build directly in the distribution root;
                    you'll need this if you're trying to use Emacs compile mode.
    build=release   build the release build variant with appropriate flags
                        in build/release and copy resulting binaries
                        into distribution/working copy root.
    build=debug     same for debug build variant, binaries will be copied with -debug suffix

With no arguments, the recipe builds wesnoth and wesnothd.  Available
build targets include the individual binaries:

    wesnoth wesnothd campaignd exploder cutter test

You can make the following special build targets:

    all = wesnoth exploder cutter wesnothd campaignd (*).
    TAGS = build tags for Emacs (*).
    wesnoth-deps.png = project dependency graph
    install = install all executables that currently exist, and any data needed
    install-wesnothd = install the Wesnoth multiplayer server.
    install-campaignd = install the Wesnoth campaign server.
    install-cutter = install the castle cutter
    install-exploder = install the castle exploder
    install-pytools = install all Python tools and modules
    uninstall = uninstall all executables, tools, modules, and servers.
    pot-update = generate gettext message catalog templates and merge them with localized message catalogs
    update-po = merge message catalog templates with localized message catalogs for particular lingua
    update-po4a = update translations of manual and manpages
    af bg ca ... = linguas for update-po
    dist = make distribution tarball as wesnoth.tar.bz2 (*).
    data-dist = make data tarball as wesnoth-data.tar.bz2 (*).
    binary-dist = make data tarball as wesnoth-binaries.tar.bz2 (*).
    wesnoth-bundle = make Mac OS application bundle from game (*)
    sanity-check = run a pre-release sanity check on the distribution.
    manual = regenerate English-language manual and, possibly, localized manuals if appropriate xmls exist.

Files made by targets marked with '(*)' are cleaned by 'scons -c all'

Options are cached in a file named .scons-option-cache and persist to later
invocations.  The file is editable.  Delete it to start fresh. You can also use a different file by
specifying --option-cache=FILE command line argument. Current option values can be listed with 'scons -h'.

If you set CXXFLAGS and/or LDFLAGS in the environment, the values will
be appended to the appropriate variables within scons.
""" + opts.GenerateHelpText(env, sort=cmp))

if GetOption("help"):
    Return()

if env["cachedir"] and not env['ccache']:
    CacheDir(env["cachedir"])

if env["fast"]:
    env.Decider('MD5-timestamp')
    SetOption('max_drift', 1)
    SetOption('implicit_cache', 1)

if not os.path.isabs(env["prefix"]):
    print "Warning: prefix is set to relative dir. destdir setting will be ignored."
    env["destdir"] = ""

#
# work around long command line problem on windows
# see http://www.scons.org/wiki/LongCmdLinesOnWin32
#
if sys.platform == 'win32':
    try:
        import win32file
        import win32event
        import win32process
        import win32security
        import string

        def my_spawn(sh, escape, cmd, args, spawnenv):
            for var in spawnenv:
                spawnenv[var] = spawnenv[var].encode('ascii', 'replace')

            sAttrs = win32security.SECURITY_ATTRIBUTES()
            StartupInfo = win32process.STARTUPINFO()
            newargs = string.join(map(escape, args[1:]), ' ')
            cmdline = cmd + " " + newargs

            # check for any special operating system commands
            if cmd == 'del':
                for arg in args[1:]:
                    win32file.DeleteFile(arg)
                exit_code = 0
            else:
                # otherwise execute the command.
                hProcess, hThread, dwPid, dwTid = win32process.CreateProcess(None,
                                                                             cmdline,
                                                                             None,
                                                                             None,
                                                                             1,
                                                                             0,
                                                                             spawnenv,
                                                                             None,
                                                                             StartupInfo)
                win32event.WaitForSingleObject(hProcess, win32event.INFINITE)
                exit_code = win32process.GetExitCodeProcess(hProcess)
                win32file.CloseHandle(hProcess);
                win32file.CloseHandle(hThread);
            return exit_code

        env['SPAWN'] = my_spawn
    except ImportError:
        def subprocess_spawn(sh, escape, cmd, args, env):
            return call(' '.join(args))
        env['SPAWN'] = subprocess_spawn

#
# Check some preconditions
#

def Warning(message):
    print message
    return False

from metasconf import init_metasconf
configure_args = dict(custom_tests=init_metasconf(env,
                                                  ["cplusplus",
                                                   "llvm",
                                                   "python_devel",
                                                   "sdl2",
                                                   "boost",
                                                   "pango",
                                                   "opencv",
                                                   "gcrypt",
                                                   "msgpack",
                                                   "zmq",
                                                   "pkgconfig",
                                                   "gettext",
                                                   "lua"]),
                                                    config_h="config.h",
                                                    log_file="$build_dir/config.log",
                                                    conf_dir="$build_dir/sconf_temp")

env.MergeFlags(env["extra_flags_config"])

if env["SDL2"]:
    sdl_version = "2.0.0"
else:
    sdl_version = "1.2.7"

if env["prereqs"]:
    conf = env.Configure(**configure_args)

    if env["PLATFORM"] == "posix":
        conf.CheckCHeader("poll.h", "<>")
        conf.CheckCHeader("sys/poll.h", "<>")
        conf.CheckCHeader("sys/select.h", "<>")
        if conf.CheckCHeader("sys/sendfile.h", "<>"):
            conf.CheckFunc("sendfile")
    conf.CheckLib("m")
    conf.CheckFunc("round")

    def CheckAsio(conf):
        if env["PLATFORM"] == 'win32':
            conf.env.Append(LIBS=["libws2_32"])
            have_libpthread = True
        elif "clang++" in env['CXX']:
            have_libpthread = True  # Check always fails with clang++
        else:
            have_libpthread = conf.CheckLib("pthread")
        return have_libpthread and \
            conf.CheckBoost("system") and \
            conf.CheckBoost("asio", header_only=True)

    if env['host'] in ['x86_64-nacl', 'i686-nacl']:
        # libppapi_cpp has a reverse dependency on the following function
        env.Append(LINKFLAGS=['-Wl,--undefined=_ZN2pp12CreateModuleEv'])
        conf.CheckLib("ppapi")
        conf.CheckLib("ppapi_cpp")
        conf.CheckLib("nacl-mounts")
        # We are linking static libraries without libtool.
        # Enumerating all transitive dependencies.
        conf.CheckLib("pthread")
        conf.CheckLib("dl")
        conf.CheckLib("SDL", use_sdl2=env["SDL2"])
        conf.CheckLib("jpeg")
        conf.CheckLib("png")
        conf.CheckLib("tiff")
        conf.CheckLib("ogg")
        conf.CheckLib("expat")
        conf.CheckLib("pixman-1")
        conf.CheckLib("vorbisfile")
        conf.CheckLib("vorbis")
        conf.CheckLib("mikmod")

    have_server_prereqs = (conf.CheckGcrypt() and
                           conf.CheckZMQ(conf) and
                           conf.CheckMsgPack(conf) and
                           conf.CheckCPlusPlus(gcc_version="3.3") and
                           conf.CheckGettextLibintl() and
                           conf.CheckBoost("iostreams", require_version="1.34.1") and
                           conf.CheckBoostIostreamsGZip() and
                           conf.CheckBoostIostreamsBZip2() and
                           conf.CheckBoost("smart_ptr", header_only=True) and
                           conf.CheckSDL(require_version=sdl_version, use_sdl2=env["SDL2"]) and
                           conf.CheckSDL('SDL_net', use_sdl2=env["SDL2"]) and
                           conf.CheckPango("cairo", require_version="1.21.3") or
                           Warning("Base prerequisites are not met."))

    env = conf.Finish()
    client_env = env.Clone()
    conf = client_env.Configure(**configure_args)

    have_client_prereqs = have_server_prereqs and \
        CheckAsio(conf) and \
        conf.CheckOpenCV() and \
        conf.CheckPKG("fontconfig") and \
        conf.CheckBoost("program_options", require_version="1.35.0") and \
        conf.CheckBoost("regex", require_version="1.35.0") and \
        conf.CheckSDL("SDL_ttf", require_version="2.0.8", use_sdl2=env["SDL2"]) and \
        conf.CheckSDL("SDL_mixer", require_version='1.2.0', use_sdl2=env["SDL2"]) and \
        conf.CheckSDL("SDL_image", require_version='1.2.0', use_sdl2=env["SDL2"]) and \
        conf.CheckLib("vorbisfile") and \
        conf.CheckOgg(use_sdl2=env["SDL2"]) or \
        Warning("Client prerequisites not met. wesnoth, cutter and exploder cannot be built.")

    have_X = False
    if have_client_prereqs:
        if env["PLATFORM"] != "win32":
            have_X = conf.CheckLib('X11')

        env["notifications"] = env["notifications"] and conf.CheckPKG("dbus-1")
        if env["notifications"]:
            client_env.Append(CPPDEFINES=["HAVE_LIBDBUS"])

        if client_env['fribidi']:
            client_env['fribidi'] = (conf.CheckPKG('fribidi >= 0.10.9') or
                                     Warning("Can't find libfribidi, disabling freebidi support."))

    if env["forum_user_handler"]:
        env.ParseConfig("pkg_config --libs --cflags mysql")
        env.Append(CPPDEFINES=["HAVE_MYSQLPP"])

    client_env = conf.Finish()

    test_env = client_env.Clone()
    conf = test_env.Configure(**configure_args)

    have_test_prereqs = (have_client_prereqs and
                         have_server_prereqs and
                         conf.CheckBoost('unit_test_framework') or
                         Warning("Unit tests are disabled because their prerequisites are not met."))
    test_env = conf.Finish()
    if (not have_test_prereqs) and "test" in env["default_targets"]:
        env["default_targets"].remove("test")

    print env.subst("If any config checks fail, look in $build_dir/config.log for details")
    print "If a check fails spuriously due to caching, use --config=force to force its rerun"

else:
    have_client_prereqs = True
    have_X = False
    if env["PLATFORM"] != "win32":
        have_X = True
    have_server_prereqs = True
    have_test_prereqs = True
    test_env = env.Clone()
    client_env = env.Clone()


if env['host'] in ['x86_64-nacl', 'i686-nacl']:
  env['_LIBFLAGS'] = '-Wl,--start-group ' + env['_LIBFLAGS'] + ' -Wl,--end-group'
  client_env['_LIBFLAGS'] = '-Wl,--start-group ' + client_env['_LIBFLAGS'] + ' -Wl,--end-group'

have_msgfmt = env["MSGFMT"]
if not have_msgfmt:
     env["nls"] = False
if not have_msgfmt:
     print "NLS tools are not present..."
if not env['nls']:
     print "NLS catalogue installation is disabled."

#
# Implement configuration switches
#

for env in [test_env, client_env, env]:
    env.Prepend(CPPPATH=["#/", "#/src"])

    env.Append(CPPDEFINES=["HAVE_CONFIG_H"])

    if "gcc" in env["TOOLS"]:
        env.AppendUnique(CCFLAGS=Split("-W -Wall"))
        env.AppendUnique(CFLAGS='-std=' + env["cstd"])
        env.AppendUnique(CXXFLAGS='-std=' + env['cxxstd'])

    # Todo add check for OpenMP capable compilers and versions
    if env['openmp']:
        env.AppendUnique(CXXFLAGS=["-fopenmp"], LIBS=["gomp"])

    if env['strict']:
        env.AppendUnique(CCFLAGS=Split("-Werror $(-Wno-unused-local-typedefs$)"))

    if env['lowmem']:
        env.Append(CPPDEFINES="LOW_MEM")

    if env['internal_data']:
        env.Append(CPPDEFINES="USE_INTERNAL_DATA")

    if have_X:
        env.Append(CPPDEFINES="_X11")

# Simulate autools-like behavior of prefix on various paths
    installdirs = Split("bindir datadir fifodir icondir desktopdir mandir docdir python_site_packages_dir")
    for d in installdirs:
        env[d] = os.path.join(env["prefix"], env[d])

    if env["PLATFORM"] == 'win32':
        env.Append(LIBS=["wsock32", "intl", "z"], CCFLAGS=["-mthreads"], LINKFLAGS=["-mthreads"], CPPDEFINES=["_WIN32_WINNT=0x0500"])
    if env["PLATFORM"] == 'darwin':  # Mac OS X
        env.Append(FRAMEWORKS="Carbon")  # Carbon GUI

if not env['static_test']:
    test_env.Append(CPPDEFINES="BOOST_TEST_DYN_LINK")

try:
    if call("utils/autorevision -t h > revision.h", shell=True) == 0:
        env["have_autorevision"] = True
except:
    pass

Export(Split("env client_env test_env have_client_prereqs have_server_prereqs have_test_prereqs"))

SConscript(dirs=Split("po doc packaging/windows packaging/systemd"))

binaries = Split("wesnoth wesnothd cutter exploder campaignd test")
builds = {
    "base"          : dict(CCFLAGS="$OPT_FLAGS"),  # Don't build in subdirectory
    "debug"         : dict(CCFLAGS=Split("$DEBUG_FLAGS")),
    "release"       : dict(CCFLAGS="$OPT_FLAGS"),
    "profile"       : dict(CCFLAGS="-pg", LINKFLAGS="-pg")
    }
build = env["build"]

for env in [test_env, client_env, env]:
    env.AppendUnique(**builds[build])
    env.Append(CXXFLAGS=Split(os.environ.get('CXXFLAGS', [])),
               LINKFLAGS=Split(os.environ.get('LDFLAGS', [])))
    env.MergeFlags(env["extra_flags_" + build])

print("--------------------------------------------------------------------------------------")
print("Current FLAGS")
print("--------------------------------------------------------------------------------------")
print("\tCXXFLAGS   = %s" % env['CXXFLAGS'])
print("\tCPPPATH    = %s" % env['CPPPATH'])
print("\tLINKFLAGS  = %s" % env['LINKFLAGS'])
print("--------------------------------------------------------------------------------------")

# Define build directory ----------------------------------------------------------------------
if build == "base":
    build_dir = ""
else:
    build_dir = os.path.join("$build_dir", build)

# Define object file names --------------------------------------------------------------------
if build == "release" :
    build_suffix = ""
else                  :
    build_suffix = "-" + build
Export("build_suffix")

# Call scr/SConscript -------------------------------------------------------------------------
env.SConscript("src/SConscript", variant_dir=build_dir, duplicate=False)

Import(binaries + ["sources"])
binary_nodes = map(eval, binaries)
all = env.Alias("all", map(Alias, binaries))
env.Default(map(Alias, env["default_targets"]))

if have_client_prereqs and env["nls"]:
    env.Requires("wesnoth", Dir("translations"))

# Utility productions (Unix-like systems only) ------------------------------------------------
# Make a tags file for Emacs: Exuberant Ctags doesn't understand the -l c++ flag
# so if the etags fails try the ctags version
env.Command("TAGS", sources,
            'etags -l c++ $SOURCES.srcpath || (ctags --tag-relative=yes -f src/tags $SOURCES.srcpath)')
env.Clean(all, 'TAGS')

# Unix installation productions ---------------------------------------------------------------
# These will not be portable to Windows or Mac. They assume a Unix-like
# directory structure and FreeDesktop standard locations for icon, app, and doc files.
for d in installdirs:
    locals()[d] = os.path.join(env["destdir"], env[d].lstrip("/"))
installable_subs = Split('data fonts images sounds')
if os.path.isabs(env["localedirname"]):
    env["localedir"] = env["localedirname"]
else:
    env["localedir"] = "$datadir/$localedirname"

pythontools = Split("wmlscope wmllint wmlindent wesnoth_addon_manager")
pythonmodules = Split("wmltools.py wmlparser.py wmldata.py wmliterator.py campaignserver_client.py __init__.py")

def CopyFilter(fn):
    "Filter out data-tree things that shouldn't be installed."
    return not ".git" in str(fn) and not "Makefile" in str(fn)

env["copy_filter"] = CopyFilter

linguas = Split(open("po/LINGUAS").read())

def InstallManpages(env, component):
    env.InstallData("mandir", component, os.path.join("doc", "man", component + ".6"), "man6")
    for lingua in linguas:
        manpage = os.path.join("doc", "man", lingua, component + ".6")
        env.InstallData("mandir", component, manpage, os.path.join(lingua, "man6"))

# Now the actual installation productions -----------------------------------------------------
# Install game and associated resources
env.InstallBinary(wesnoth)  # Game
env.InstallData("datadir",  # data
                "wesnoth",
                map(Dir, installable_subs))
env.InstallData("docdir",  # docs
                "wesnoth",
                [Glob("doc/manual/*.html"),
                 Dir("doc/manual/styles"),
                 Dir("doc/manual/images")])
InstallManpages(env, "wesnoth")  # manpages
env.InstallBinary(campaignd)  # Wesnoth campaign server
env.InstallBinary(cutter)  # And the artists' tools
env.InstallBinary(exploder)  # And the artists' tools

# Install translations if requested
if env["nls"]:
    env.InstallData("localedir", "wesnoth", Dir("translations"))
    env.InstallData("datadir", "wesnoth", "l10n-track")

# Install desktop niceties if requested
if have_client_prereqs and have_X and env["desktop_entry"]:
     if sys.platform == "darwin":
         env.InstallData("icondir", "wesnoth", "icons/wesnoth-icon-Mac.png")
     else:
         env.InstallData("icondir", "wesnoth", "icons/wesnoth-icon.png")
     env.InstallData("desktopdir", "wesnoth", "icons/wesnoth.desktop")

# Python tools
env.InstallData("bindir",
                "pytools",
                map(lambda tool: os.path.join("data", "tools", tool), pythontools))
env.InstallData("python_site_packages_dir",
                "pytools",
                map(lambda module: os.path.join("data", "tools", "wesnoth", module), pythonmodules))

# Wesnoth MP server install
env.InstallBinary(wesnothd)
InstallManpages(env, "wesnothd")
if not access(fifodir, F_OK):
    fifodir = env.Command(fifodir, [], [
        Mkdir(fifodir),
        Chmod(fifodir, 0700),
        Action("chown %s:%s %s" %
               (env["server_uid"], env["server_gid"], fifodir)),
        ])
    AlwaysBuild(fifodir)
    env.Alias("install-wesnothd", fifodir)

# Handle systemd MP Server install
if env["systemd"]:
    env.InstallData("prefix",
                    "wesnothd",
                    "#packaging/systemd/wesnothd.service",
                    "lib/systemd/system")
    env.InstallData("prefix",
                    "wesnothd",
                    "#packaging/systemd/wesnothd.conf",
                    "lib/tmpfiles.d")


# Compute things for default install based on which targets have been created. ----------------
install = env.Alias('install', [])
for installable in ('wesnoth', 'wesnothd', 'campaignd', 'exploder', 'cutter'):
    if (os.path.exists(installable + build_suffix) or
        installable in COMMAND_LINE_TARGETS or
        "all" in COMMAND_LINE_TARGETS
        ):
        env.Alias('install', env.Alias('install-' + installable))

# Un-installation -----------------------------------------------------------------------------
def Uninstall(nodes):
    deletes = []
    for node in nodes:
        if node.__class__ == install[0].__class__:
            deletes.append(Uninstall(node.sources))
        else:
            deletes.append(Delete(str(node)))
    return deletes
uninstall = env.Command('uninstall', '', Flatten(Uninstall(Alias("install"))) or "")
env.AlwaysBuild(uninstall)
env.Precious(uninstall)

# Making a distribution tarball. --------------------------------------------------------------
env["version"] = build_config.get("VERSION")
if 'dist' in COMMAND_LINE_TARGETS:  # Speedup, the manifest is expensive
    def dist_manifest():
        "Get an argument list suitable for passing to a distribution archiver."
        # Start by getting a list of all files under version control
        lst = commands.getoutput("git ls-files | grep -v 'data\/test\/.*' | awk '/^[^?]/ {print $4;}'").split()
        lst = filter(os.path.isfile, lst)
        return lst
    dist_tarball = env.Tar('wesnoth-${version}.tar.bz2', [])
    open("dist.manifest", "w").write("\n".join(dist_manifest() + ["src/revision.hpp"]))
    env.Append(TARFLAGS='-j -T dist.manifest --transform "s,^,wesnoth-$version/,"',
               TARCOMSTR="Making distribution tarball...")
    env.AlwaysBuild(dist_tarball)
    env.Clean(all, 'wesnoth.tar.bz2')
    env.Alias('dist', dist_tarball)


# Make binary distribution (from installed client side) ---------------------------------------
bin_tar_env = env.Clone()
bin_tarball = bin_tar_env.Tar('wesnoth-binary.tar.bz2',
                              os.path.join(bindir, "wesnoth"))
bin_tar_env.Append(TARFLAGS='-j', TARCOMSTR="Making binary tarball...")
env.Clean(all, 'wesnoth-binary.tar.bz2')
env.Alias('binary-dist', bin_tarball)

# Make data distribution (from installed client side) -----------------------------------------
data_tar_env = env.Clone()
data_tarball = data_tar_env.Tar('wesnoth-data.tar.bz2', datadir)
data_tar_env.Append(TARFLAGS='-j', TARCOMSTR="Making data tarball...")
env.Clean(all, 'wesnoth-data.tar.bz2')
env.Alias('data-dist', data_tarball)

# Windows installer ---------------------------------------------------------------------------
text_builder = Builder(action=Copy("$TARGET", "$SOURCE"), single_source=True, suffix=".txt")
env.WindowsInstaller([
    wesnoth, wesnothd,
    Dir(installable_subs), env["nls"] and Dir("translations") or [],
    glob("*.dll"),
    text_builder(env, source=Split("README copyright COPYING changelog players_changelog"))
    ])

# Making Mac OS X application bundles ---------------------------------------------------------
env.Alias("wesnoth-bundle",
          env.Command("Battle For Wesnoth.app", "wesnoth", [
              Mkdir("${TARGET}/Contents"),
              Mkdir("${TARGET}/Contents/MacOS"),
              Mkdir("${TARGET}/Contents/Resources"),
              Action('echo "APPL????" > "${TARGET}/Contents/PkgInfo"'),
              Copy("${TARGET}/Contents/MacOS/wesnoth", "wesnoth"),
              ]))
env.Clean(all, "Battle For Wesnoth.app")

# Sanity checking -----------------------------------------------------------------------------
# @note that this is a "#!/bin/sh" script
sanity_check = env.Command('sanity-check',
                           '',
                           [Action("cd utils; ./sanity_check"),
                            Action("cd data/tools; make sanity-check"), ])
env.AlwaysBuild(sanity_check)
env.Precious(sanity_check)

# Make the project dependency graph (requires graph-includes). --------------------------------
# @todo Fix this to use env['CPPPATH']
env.Command("wesnoth-deps.dot",
            [],
            "graph-includes -verbose --class wesnoth \
            -sysI /usr/include/c++/4.0 -sysI /usr/include -sysI /usr/include/SDL \
            --prefixstrip src/ -I src src > ${TARGET}")
env.Command("wesnoth-deps.png", "wesnoth-deps.dot",
            "dot -Tpng -o ${TARGET} ${SOURCE}")
env.Clean(all, ["wesnoth-deps.dot", "wesnoth-deps.png"])

# Local variables:
# mode: python
# end:
