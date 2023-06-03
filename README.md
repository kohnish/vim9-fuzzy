Vim9-fuzzy
=========

A vim plugin for finding files with approximate string matching.
No vim language binding dependencies, use job-start API to avoid blocking vim

ToDo
----
 - Reduce hard coding
 - Remove the default rg executables dependency
 - Add backtrace
 - Add tests
 - Improve fuzzy algorithm
 - Allow custom keybinding
 - UTF-8 support
 - Test on Windows

Runtime requirements
--------------------
 - Vim with vim9 script support
 - Rg (Only for the default list command, can be overridden by Vim9fuzzy_user_list_func)

Usage
-----
Installation:
```sh
mkdir -p ~/.vim/pack/plugins/opt
git clone https://github.com/kohnish/vim9-fuzzy ~/.vim/pack/plugins/opt/vim9-fuzzy
add `packadd! vim9-fuzzy` in your vimrc

# For platforms with libuv-devel in the library path
:Vim9fuzzyMake

# For Linux, there is prebuilt binary
:Vim9fuzzyMake linux-download

# Or just compile static binary and install. (Check the build requirement for details)
:Vim9fuzzyMake online-build
```

Optional Configuration
```vim
# Vim9-fuzzy Keymap (No defaults)
# Search by only file name.
noremap <C-f> :Vim9FuzzyFile<CR>
# Search by full path
noremap <C-p> :Vim9FuzzyPath<CR>
# Search in last opened files via vim9-fuzzy.
noremap <C-e> :Vim9FuzzyMru<CR>
# Search in the directory of the current dir
noremap <C-k> :Vim9FuzzyPwdFile<CR>

# Path for keeping most recently used files (Default here)
g:vim9_fuzzy_mru_path = $HOME .. "/.vim/pack/plugins/opt/vim9-fuzzy/mru"

# For fuzzy yank history search
# Enable yank hook (Default false)
g:vim9_fuzzy_yank_enabled = true
noremap <C-y> :Vim9FuzzyYank<CR>
# The path for keeping yank histories (Defaulting here)
g:vim9_fuzzy_yank_path = $HOME .. "/.vim/pack/plugins/opt/vim9-fuzzy/yank"

# Root path to search from (Default to the current dir)
var proj_dir = getcwd()
var git_root = system("git rev-parse --show-toplevel | tr -d '\n'")
if len(git_root) > 0
    proj_dir = git_root
endif
g:vim9_fuzzy_proj_dir = proj_dir

# To set root of search path on every vim9-fuzzy window is opened in case vim current dir changes.
g:vim9fuzzy_get_proj_root_func = true
def g:Vim9FuzzyGetProjRootFunc(): string
    var root_dir = getcwd()
    var git_root_dir = trim(system(exepath("git") .. " -C " .. root_dir .. " rev-parse --show-toplevel 2>/dev/null"))
    if !empty(git_root_dir)
        root_dir = git_root_dir
    endif
    return root_dir
enddef

# Override defaut list cmd (defaults to rg)
g:vim9fuzzy_user_list_func = true
def g:Vim9fuzzy_user_list_func(root_dir: string, target_dir: string): dict<any>
    var git_exe = exepath("git")
    var dir = target_dir
    if dir == ""
        dir = root_dir
    endif
    var is_in_git_dir = system(git_exe .. " -C " .. dir .. " rev-parse --is-inside-work-tree")
    var in_git_dir = v:shell_error == 0
    var is_in_ignore_dir = system("git check-ignore " .. dir)
    var in_ignore_dir = v:shell_error == 0
    if in_git_dir && !in_ignore_dir
        return {
            "trim_target_dir": true,
            "cmd": git_exe .. " -C " .. dir .. " ls-files | egrep -v '^.*(\.png|\.jpg)$' && " .. git_exe .. " clean --dry-run -d | awk '{print $3}'"
        }
    endif
    return {
        "trim_target_dir": false,
        "cmd": "find " .. dir .. " -type f -maxdepth 3"
    }
enddef
```

Build requirements
------------------
 - CMake
 - pkg-config
 - C compiler (GCC / Clang)
 - Ninja (Only for make online-build target)
 - C++ compiler (Optional for test)  

Build (static build(needs internet access))
```shell
mkdir build
cd build
cmake -DBUILD_STATIC=ON ..
make -j`nproc`
make install/strip
```

Build (dynamic build(Fedora))
```shell
mkdir build
cd build
sudo dnf install -y libuv-devel
cmake ..
make -j`nproc`
make install/strip
```
